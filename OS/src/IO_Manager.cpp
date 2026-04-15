#include "IO_Manager.h"
#include "Communication.h"

#if defined(ARDUINO_ARCH_AVR)
#include <avr/wdt.h>
#endif

#if defined(ARDUINO_ARCH_RP2040)
#include <hardware/watchdog.h>
#endif

#include <Servo.h>

namespace {

constexpr unsigned long kDefaultTelemetryMs = 1000;
constexpr unsigned long kDefaultWatchdogMs = 5000;
constexpr unsigned long kDefaultBuzzHz = 2200;

#if defined(ARDUINO_ARCH_RP2040)
constexpr uint8_t kBuiltInLedPin = LED_BUILTIN;
#else
constexpr uint8_t kBuiltInLedPin = LED_BUILTIN;
#endif

constexpr uint8_t kPresencePin = 2;
constexpr uint8_t kAuxInterruptPin = 3;
constexpr uint8_t kBuzzerPin = 5;
constexpr uint8_t kServoPin = 6;
constexpr uint8_t kRelayPins[2] = {4, 7};
constexpr uint8_t kRgbPins[3] = {9, 10, 11};
constexpr uint8_t kLuxPin = A0;
constexpr uint8_t kTempPin = A1;
constexpr uint8_t kHumidityPin = A2;

Servo g_servo;
bool g_servoAttached = false;
IO_Manager* g_ioManager = nullptr;

void isrPresence() {
    if (g_ioManager != nullptr) {
        g_ioManager->handleInterrupt(kPresencePin);
    }
}

void isrAux() {
    if (g_ioManager != nullptr) {
        g_ioManager->handleInterrupt(kAuxInterruptPin);
    }
}

}  // namespace

IO_Manager::IO_Manager()
    : _lastHeartbeat(0),
      _watchdogTimeout(0),
      _lastTelemetryMs(0),
      _rgbOverrideUntilMs(0),
      _buzzerUntilMs(0),
      _isFailSafe(false),
      _presenceOverrideEnabled(false),
      _presenceOverrideValue(false),
      _role(NodeRole::Reflex),
      _coreState(CoreState::Idle),
      _comm(nullptr),
      _rgbValue{0, 0, 0},
      _relayState{0, 0},
      _servoAngle{90} {
    g_ioManager = this;
}

void IO_Manager::setCommunication(Communication* comm) {
    _comm = comm;
}

void IO_Manager::begin() {
    pinMode(kBuiltInLedPin, OUTPUT);
    applyBuiltInIndicator(false);

    pinMode(kPresencePin, INPUT_PULLUP);
    pinMode(kAuxInterruptPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(kPresencePin), isrPresence, CHANGE);
    attachInterrupt(digitalPinToInterrupt(kAuxInterruptPin), isrAux, CHANGE);

    pinMode(kBuzzerPin, OUTPUT);
    noTone(kBuzzerPin);

    for (uint8_t pin : kRelayPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    for (uint8_t pin : kRgbPins) {
        pinMode(pin, OUTPUT);
        analogWrite(pin, 0);
    }

    if (!g_servoAttached) {
        g_servo.attach(kServoPin);
        g_servoAttached = true;
    }
    g_servo.write(_servoAngle[0]);

    noteHostContact();
    configWatchdog(kDefaultWatchdogMs);
}

void IO_Manager::update() {
    const unsigned long now = millis();

    if (_watchdogTimeout > 0 && (now - _lastHeartbeat) > _watchdogTimeout) {
        enterFailSafe();
    } else if (_isFailSafe && _coreState != CoreState::Disconnected) {
        exitFailSafe();
    }

    if (!_isFailSafe) {
        feedHardwareWatchdog();
    }

    emitTelemetryIfDue();
    updateIndicators();
    updateBuzzer();
}

void IO_Manager::configWatchdog(unsigned long timeoutMs) {
    _watchdogTimeout = timeoutMs;
    noteHostContact();
    configureHardwareWatchdog(timeoutMs);
}

void IO_Manager::noteHostContact() {
    _lastHeartbeat = millis();
    if (_coreState == CoreState::Disconnected) {
        _coreState = CoreState::Idle;
    }
    exitFailSafe();
}

void IO_Manager::handleInterrupt(int pin) {
    if (_comm == nullptr) {
        return;
    }

    char pinName[12];
    snprintf(pinName, sizeof(pinName), "D%d", pin);
    _comm->sendEvent(pinName, "INTERRUPT");
}

bool IO_Manager::writePin(const String& target, int value) {
    if (target == "RELAY_01") {
        return setRelay(0, value != 0);
    }
    if (target == "RELAY_02") {
        return setRelay(1, value != 0);
    }
    if (target == "SERVO_01") {
        return setServoAngle(0, value);
    }
    if (target == "PRESENCE_OVERRIDE") {
        return setPresenceOverride(value != 0);
    }

    const int pin = getPinNumber(target);
    if (pin < 0) {
        return false;
    }

    noteHostContact();
    if (isAnalogTarget(target)) {
        analogWrite(pin, constrain(value, 0, 255));
    } else {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, value != 0 ? HIGH : LOW);
    }
    return true;
}

int IO_Manager::readPin(const String& target) {
    if (target == "PRESENCE_01" || target == "PRESENCE") {
        return readPresence() ? 1 : 0;
    }
    if (target == "LUX_01" || target == "LIGHT_01") {
        return readLuxEstimate();
    }
    if (target == "TEMP_01") {
        return static_cast<int>(readTemperatureEstimate() * 10.0f);
    }
    if (target == "HUMIDITY_01") {
        return static_cast<int>(readHumidityEstimate() * 10.0f);
    }

    const int pin = getPinNumber(target);
    if (pin < 0) {
        return 0;
    }

    noteHostContact();
    if (isAnalogTarget(target)) {
        return analogRead(pin);
    }

    pinMode(pin, INPUT_PULLUP);
    return digitalRead(pin);
}

bool IO_Manager::setRole(const String& roleName) {
    if (roleName == "reflex") {
        _role = NodeRole::Reflex;
    } else if (roleName == "sentry") {
        _role = NodeRole::Sentry;
    } else if (roleName == "action") {
        _role = NodeRole::Action;
    } else {
        return false;
    }

    noteHostContact();
    return true;
}

bool IO_Manager::setCoreState(const String& stateName) {
    noteHostContact();

    if (stateName == "IDLE" || stateName == "idle") {
        _coreState = CoreState::Idle;
    } else if (stateName == "BUSY" || stateName == "PROCESSING" || stateName == "busy") {
        _coreState = CoreState::Busy;
    } else if (stateName == "ALERT" || stateName == "HIGH_PRIORITY" || stateName == "alert") {
        _coreState = CoreState::Alert;
    } else if (stateName == "DISCONNECTED" || stateName == "disconnected") {
        _coreState = CoreState::Disconnected;
    } else {
        return false;
    }
    return true;
}

bool IO_Manager::setRgb(uint8_t r, uint8_t g, uint8_t b, unsigned long durationMs) {
    _rgbValue[0] = r;
    _rgbValue[1] = g;
    _rgbValue[2] = b;
    _rgbOverrideUntilMs = durationMs > 0 ? millis() + durationMs : 0;
    noteHostContact();
    applyRgb(r, g, b);
    return true;
}

bool IO_Manager::setRelay(uint8_t index, bool enabled) {
    if (index >= 2) {
        return false;
    }

    _relayState[index] = enabled ? HIGH : LOW;
    digitalWrite(kRelayPins[index], _relayState[index]);
    noteHostContact();
    return true;
}

bool IO_Manager::setServoAngle(uint8_t index, int angle) {
    if (index >= 1) {
        return false;
    }

    const int bounded = constrain(angle, 0, 180);
    _servoAngle[index] = bounded;
    if (g_servoAttached) {
        g_servo.write(bounded);
    }
    noteHostContact();
    return true;
}

bool IO_Manager::buzz(unsigned int frequency, unsigned long durationMs) {
    noteHostContact();
    tone(kBuzzerPin, frequency == 0 ? kDefaultBuzzHz : frequency);
    _buzzerUntilMs = millis() + (durationMs == 0 ? 150 : durationMs);
    return true;
}

bool IO_Manager::setPresenceOverride(bool present) {
    _presenceOverrideEnabled = true;
    _presenceOverrideValue = present;
    noteHostContact();
    return true;
}

void IO_Manager::populateIdentity(JsonDocument& response) const {
    response["identity"] = getNodeId();
    response["node_id"] = getNodeId();
    response["hardware"] = readHardwareName();
    response["version"] = "2.0.0";
    response["tier"] = 1;
    response["transport"] = "usb-serial";
}

void IO_Manager::populateCapabilities(JsonObject response) const {
    JsonArray outputs = response["outputs"].to<JsonArray>();
    outputs.add("status_led");
    outputs.add("rgb_led");
    outputs.add("buzzer");
    outputs.add("relay_bank");
    outputs.add("servo");

    JsonArray inputs = response["inputs"].to<JsonArray>();
    inputs.add("presence");
    inputs.add("lux");
    inputs.add("temp_estimate");
    inputs.add("humidity_estimate");
    inputs.add("interrupt_d2");
    inputs.add("interrupt_d3");

    response["watchdog"] = true;
    response["telemetry_interval_ms"] = kDefaultTelemetryMs;
}

void IO_Manager::populateStatus(JsonDocument& response) const {
    response["node_id"] = getNodeId();
    response["status"] = _isFailSafe ? "FAIL_SAFE" : "READY";
    response["role"] = getRoleName();
    response["core_state"] = getCoreStateName();
    response["watchdog_ms"] = _watchdogTimeout;
    response["uptime_ms"] = millis();
    response["failsafe"] = _isFailSafe;

    JsonArray relays = response["relays"].to<JsonArray>();
    relays.add(_relayState[0] == HIGH);
    relays.add(_relayState[1] == HIGH);

    response["servo_angle"] = _servoAngle[0];
}

void IO_Manager::populateTelemetry(JsonDocument& response) const {
    response["node_id"] = getNodeId();
    response["status"] = _isFailSafe ? "FAIL_SAFE" : "READY";
    response["role"] = getRoleName();
    response["core_state"] = getCoreStateName();
    response["uptime_ms"] = millis();

    JsonObject sensors = response["sensors"].to<JsonObject>();
    sensors["temp"] = readTemperatureEstimate();
    sensors["humidity"] = readHumidityEstimate();
    sensors["lux"] = readLuxEstimate();
    sensors["presence"] = readPresence();
    sensors["temp_raw"] = analogRead(kTempPin);
    sensors["humidity_raw"] = analogRead(kHumidityPin);
    sensors["lux_raw"] = analogRead(kLuxPin);
}

const char* IO_Manager::getNodeId() const {
#if defined(ARDUINO_ARCH_RP2040)
    return "MMX-RP2040-01";
#else
    return "MMX-UNO-01";
#endif
}

const char* IO_Manager::getRoleName() const {
    switch (_role) {
        case NodeRole::Reflex:
            return "reflex";
        case NodeRole::Sentry:
            return "sentry";
        case NodeRole::Action:
            return "action";
        default:
            return "reflex";
    }
}

const char* IO_Manager::getCoreStateName() const {
    switch (_coreState) {
        case CoreState::Idle:
            return "IDLE";
        case CoreState::Busy:
            return "BUSY";
        case CoreState::Alert:
            return "ALERT";
        case CoreState::Disconnected:
            return "DISCONNECTED";
        default:
            return "IDLE";
    }
}

bool IO_Manager::isFailSafe() const {
    return _isFailSafe;
}

void IO_Manager::enterFailSafe() {
    _isFailSafe = true;
    _coreState = CoreState::Disconnected;

    for (uint8_t i = 0; i < 2; ++i) {
        _relayState[i] = LOW;
        digitalWrite(kRelayPins[i], LOW);
    }

    if (g_servoAttached) {
        _servoAngle[0] = 90;
        g_servo.write(_servoAngle[0]);
    }

    noTone(kBuzzerPin);
    _buzzerUntilMs = 0;
}

void IO_Manager::exitFailSafe() {
    _isFailSafe = false;
}

void IO_Manager::emitTelemetryIfDue() {
    const unsigned long now = millis();
    if (_comm == nullptr || (now - _lastTelemetryMs) < kDefaultTelemetryMs) {
        return;
    }

    _lastTelemetryMs = now;
    _comm->sendTelemetry();
}

void IO_Manager::updateIndicators() {
    const unsigned long now = millis();

    if (_rgbOverrideUntilMs > 0 && now < _rgbOverrideUntilMs) {
        applyBuiltInIndicator((now / 120) % 2 == 0);
        return;
    }

    if (_rgbOverrideUntilMs > 0 && now >= _rgbOverrideUntilMs) {
        _rgbOverrideUntilMs = 0;
    }

    switch (_coreState) {
        case CoreState::Idle:
            applyRgb(0, 0, 64);
            applyBuiltInIndicator((now / 1500) % 2 == 0);
            break;
        case CoreState::Busy:
            applyRgb(64, 0, 0);
            applyBuiltInIndicator((now / 220) % 2 == 0);
            break;
        case CoreState::Alert:
            applyRgb(255, 0, 0);
            applyBuiltInIndicator((now / 120) % 2 == 0);
            break;
        case CoreState::Disconnected:
            applyRgb(255, 0, 0);
            applyBuiltInIndicator((now / 250) % 2 == 0);
            break;
    }
}

void IO_Manager::updateBuzzer() {
    if (_buzzerUntilMs > 0 && millis() >= _buzzerUntilMs) {
        noTone(kBuzzerPin);
        _buzzerUntilMs = 0;
    }
}

void IO_Manager::applyRgb(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(kRgbPins[0], r);
    analogWrite(kRgbPins[1], g);
    analogWrite(kRgbPins[2], b);
}

void IO_Manager::applyBuiltInIndicator(bool on) {
    digitalWrite(kBuiltInLedPin, on ? HIGH : LOW);
}

void IO_Manager::feedHardwareWatchdog() {
#if defined(ARDUINO_ARCH_AVR)
    if (_watchdogTimeout > 0) {
        wdt_reset();
    }
#elif defined(ARDUINO_ARCH_RP2040)
    if (_watchdogTimeout > 0) {
        watchdog_update();
    }
#endif
}

void IO_Manager::configureHardwareWatchdog(unsigned long timeoutMs) {
#if defined(ARDUINO_ARCH_AVR)
    if (timeoutMs == 0) {
        wdt_disable();
        return;
    }

    if (timeoutMs <= 15) {
        wdt_enable(WDTO_15MS);
    } else if (timeoutMs <= 30) {
        wdt_enable(WDTO_30MS);
    } else if (timeoutMs <= 60) {
        wdt_enable(WDTO_60MS);
    } else if (timeoutMs <= 120) {
        wdt_enable(WDTO_120MS);
    } else if (timeoutMs <= 250) {
        wdt_enable(WDTO_250MS);
    } else if (timeoutMs <= 500) {
        wdt_enable(WDTO_500MS);
    } else if (timeoutMs <= 1000) {
        wdt_enable(WDTO_1S);
    } else if (timeoutMs <= 2000) {
        wdt_enable(WDTO_2S);
    } else if (timeoutMs <= 4000) {
        wdt_enable(WDTO_4S);
    } else {
        wdt_enable(WDTO_8S);
    }
#elif defined(ARDUINO_ARCH_RP2040)
    if (timeoutMs == 0) {
        return;
    }
    watchdog_enable(timeoutMs, true);
#else
    (void)timeoutMs;
#endif
}

const char* IO_Manager::readHardwareName() const {
#if defined(ARDUINO_ARCH_RP2040)
    return "RP2040";
#else
    return "ATmega328P";
#endif
}

float IO_Manager::readTemperatureEstimate() const {
    const int raw = analogRead(kTempPin);
#if defined(ARDUINO_ARCH_RP2040)
    const float voltage = raw * (3.3f / 1023.0f);
    return 27.0f - ((voltage - 0.706f) / 0.001721f);
#else
    return 18.0f + (raw / 1023.0f) * 17.0f;
#endif
}

float IO_Manager::readHumidityEstimate() const {
    const int raw = analogRead(kHumidityPin);
    return 25.0f + (raw / 1023.0f) * 55.0f;
}

int IO_Manager::readLuxEstimate() const {
    const int raw = analogRead(kLuxPin);
    return map(raw, 0, 1023, 0, 1000);
}

bool IO_Manager::readPresence() const {
    if (_presenceOverrideEnabled) {
        return _presenceOverrideValue;
    }

    return digitalRead(kPresencePin) == LOW;
}

int IO_Manager::getPinNumber(const String& target) const {
    if (target.length() < 2) {
        return -1;
    }

    const char prefix = target.charAt(0);
    const int pin = target.substring(1).toInt();
    if (prefix == 'D' || prefix == 'd') {
        return pin;
    }

    if (prefix == 'A' || prefix == 'a') {
        switch (pin) {
            case 0:
                return A0;
            case 1:
                return A1;
            case 2:
                return A2;
            case 3:
                return A3;
#if defined(A4)
            case 4:
                return A4;
#endif
#if defined(A5)
            case 5:
                return A5;
#endif
            default:
                return -1;
        }
    }

    return -1;
}

bool IO_Manager::isAnalogTarget(const String& target) const {
    if (target.length() < 2) {
        return false;
    }

    const char prefix = target.charAt(0);
    return prefix == 'A' || prefix == 'a';
}
