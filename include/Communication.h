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
    void sendResponse(const JsonDocument& response);
    void sendError(const char* message);

private:
    IO_Manager& _ioManager;
    String inputBuffer;

    void processMessage(const String& message);
    void handleAction(JsonDocument& doc);
    void handleQuery(JsonDocument& doc);
};

#endif // COMMUNICATION_H
