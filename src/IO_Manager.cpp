#include "IO_Manager.h"
#include "Communication.h"

// For interrupts
IO_Manager* global_io_manager = nullptr;

#ifdef ARDUINO_ARCH_RP2040
    #define LED_PIN 25 // Default LED pin for Pico
#else
    #define LED_PIN 13 // Default LED pin for Uno
#endif

void isr_D2() {
    if (global_io_manager) {
        global_io_manager->handleInterrupt(2);
    }
}

void isr_D3() {
    if (global_io_manager) {
        global_io_manager->handleInterrupt(3);
    }
}

IO_Manager::IO_Manager() : _lastHeartbeat(0), _watchdogTimeout(0), _isFailSafe(false), _comm(nullptr) {
    global_io_manager = this;
}

void IO_Manager::setCommunication(Communication* comm) {
    _comm = comm;
}

void IO_Manager::begin() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize relays D2-D9 as OUTPUT and LOW
    for (int i = 2; i <= 9; ++i) {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
    }

    // Configure interrupts on D2 and D3
    // Note: If D2/D3 are relays, maybe interrupts on other pins make more sense?
    // The spec says:
    // Implement an interrupt-driven system for specific digital pins (D2, D3).
    // Let's set them as INPUT_PULLUP for interrupts.
    pinMode(2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(2), isr_D2, CHANGE);

    pinMode(3, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(3), isr_D3, CHANGE);
}

void IO_Manager::configWatchdog(unsigned long timeout) {
    _watchdogTimeout = timeout;
    _lastHeartbeat = millis();
    _isFailSafe = false;
    digitalWrite(LED_PIN, LOW); // Turn off fail-safe LED if it was on
}

void IO_Manager::update() {
    // Check Watchdog
    if (_watchdogTimeout > 0 && !_isFailSafe) {
        if (millis() - _lastHeartbeat > _watchdogTimeout) {
            enterFailSafe();
        }
    }

    // Fail-safe LED pulse (2Hz = 500ms period = 250ms ON / 250ms OFF)
    if (_isFailSafe) {
        if ((millis() % 500) < 250) {
            digitalWrite(LED_PIN, HIGH);
        } else {
            digitalWrite(LED_PIN, LOW);
        }
    }
}

void IO_Manager::enterFailSafe() {
    _isFailSafe = true;
    // Set all Relay pins (D2-D9) to LOW (Normally Open)
    // Careful: D2 and D3 are interrupts according to spec, but also "Relay pins (D2-D9)".
    // We'll set 4-9 LOW, and if 2/3 are relays we'd set them too, but they are inputs.
    // Spec: "immediately set all Relay pins (D2-D9) to LOW"
    // Spec: "Implement an interrupt-driven system for specific digital pins (D2, D3)"
    // We will ensure 4-9 are LOW, and try to write 2-3 LOW (which may disable pullup).
    for (int i = 2; i <= 9; ++i) {
        pinMode(i, OUTPUT);
        digitalWrite(i, LOW);
    }
}

void IO_Manager::writePin(const String& target, int value) {
    int pin = getPinNumber(target);
    if (pin != -1) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, value);
    }
    // Any valid action resets the heartbeat
    _lastHeartbeat = millis();
    _isFailSafe = false;
}

int IO_Manager::readPin(const String& target) {
    int pin = getPinNumber(target);
    int value = 0;
    if (pin != -1) {
        if (isAnalog(target)) {
            value = analogRead(pin);
        } else {
            pinMode(pin, INPUT); // or INPUT_PULLUP? Spec doesn't specify
            value = digitalRead(pin);
        }
    }
    // Any valid action resets the heartbeat
    _lastHeartbeat = millis();
    _isFailSafe = false;
    return value;
}

void IO_Manager::handleInterrupt(int pin) {
    if (_comm) {
        char pinName[4];
        snprintf(pinName, sizeof(pinName), "D%d", pin);
        _comm->sendEvent(pinName, "Interrupt");
    }
}

int IO_Manager::getPinNumber(const String& target) {
    if (target.length() < 2) return -1;
    char type = target.charAt(0);
    String numStr = target.substring(1);
    int pin = numStr.toInt();

    // In Arduino, A0 etc. can be treated as analog or digital.
    // D13 -> 13
    // A0 -> A0 macro
    if (type == 'A' || type == 'a') {
        switch (pin) {
            case 0: return A0;
            case 1: return A1;
            case 2: return A2;
            case 3: return A3;
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_UNO) || defined(A4)
            case 4: return A4;
            case 5: return A5;
#endif
            default: return -1;
        }
    } else if (type == 'D' || type == 'd') {
        return pin;
    }
    return -1;
}

bool IO_Manager::isAnalog(const String& target) {
    if (target.length() < 2) return false;
    char type = target.charAt(0);
    return (type == 'A' || type == 'a');
}
