#pragma once

#include "IO_Manager.h"
#include "FailSafeStateMachine.h"
#include <ArduinoJson.h>

class Communication {
private:
    IO_Manager& ioManager;
    FailSafeStateMachine stateMachine;

    void processMessage(const char* jsonString);

public:
    Communication(IO_Manager& io);

    void begin(long baudRate);
    void update();
    void sendIdentity();
    void sendStatus();

    void handlePing(JsonObject doc);
    void handleQueryTelemetry(JsonObject doc);
    void handleSetActuator(JsonObject doc);
    void handleSetRole(JsonObject doc);
    void handleReboot(JsonObject doc);

    void sendError(const char* msgId, const char* errorMsg);
};
