#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class Communication;

enum class NodeRole {
    Reflex,
    Sentry,
    Action
};

enum class CoreState {
    Idle,
    Busy,
    Alert,
    Disconnected
};

class IO_Manager {
public:
    IO_Manager();

    void begin();
    void update();
    void setCommunication(Communication* comm);

    void configWatchdog(unsigned long timeoutMs);
    void noteHostContact();
    void handleInterrupt(int pin);

    bool writePin(const String& target, int value);
    int readPin(const String& target);

    bool setRole(const String& roleName);
    bool setCoreState(const String& stateName);
    bool setRgb(uint8_t r, uint8_t g, uint8_t b, unsigned long durationMs);
    bool setRelay(uint8_t index, bool enabled);
    bool setServoAngle(uint8_t index, int angle);
    bool buzz(unsigned int frequency, unsigned long durationMs);
    bool setPresenceOverride(bool present);

    void populateIdentity(JsonDocument& response) const;
    void populateCapabilities(JsonObject response) const;
    void populateStatus(JsonDocument& response) const;
    void populateTelemetry(JsonDocument& response) const;

    const char* getNodeId() const;
    const char* getRoleName() const;
    const char* getCoreStateName() const;
    bool isFailSafe() const;

private:
    unsigned long _lastHeartbeat;
    unsigned long _watchdogTimeout;
    unsigned long _lastTelemetryMs;
    unsigned long _rgbOverrideUntilMs;
    unsigned long _buzzerUntilMs;
    bool _isFailSafe;
    bool _presenceOverrideEnabled;
    bool _presenceOverrideValue;
    NodeRole _role;
    CoreState _coreState;
    Communication* _comm;

    uint8_t _rgbValue[3];
    uint8_t _relayState[2];
    int _servoAngle[1];

    void enterFailSafe();
    void exitFailSafe();
    void emitTelemetryIfDue();
    void updateIndicators();
    void updateBuzzer();
    void applyRgb(uint8_t r, uint8_t g, uint8_t b);
    void applyBuiltInIndicator(bool on);
    void feedHardwareWatchdog();
    void configureHardwareWatchdog(unsigned long timeoutMs);
    const char* readHardwareName() const;
    float readTemperatureEstimate() const;
    float readHumidityEstimate() const;
    int readLuxEstimate() const;
    bool readPresence() const;
    int getPinNumber(const String& target) const;
    bool isAnalogTarget(const String& target) const;
};

#endif
