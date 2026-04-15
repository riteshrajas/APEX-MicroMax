#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "IO_Manager.h"

class Communication {
public:
    Communication(IO_Manager& ioManager);
    void begin(long baudRate);
    void update();

    void sendIdentity();
    void sendEvent(const char* source, const char* type);
    void sendTelemetry();
    void sendStatus();
    void sendResponse(const JsonDocument& response);
    void sendError(const char* message);

private:
    IO_Manager& _ioManager;
    String inputBuffer;

    void processMessage(const String& message);
    void handleAction(JsonDocument& doc);
    void handleQuery(JsonDocument& doc);
    void handleLegacyCommand(const String& command, JsonDocument& doc);
    bool parseBoolValue(JsonVariantConst source, bool defaultValue) const;
    unsigned long parseDuration(JsonVariantConst source, unsigned long defaultValue) const;
    void sendAck(const char* action, const String& target);
};

#endif
