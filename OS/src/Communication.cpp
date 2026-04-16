#include "../include/Communication.h"
#include "../include/IHardware.h"
#include <string.h>

Communication::Communication(IO_Manager& io)
    : ioManager(io), stateMachine(getHardware()) {}

void Communication::begin(long baudRate) {
    getHardware()->serialBegin(baudRate);
    stateMachine.begin();
}

void Communication::update() {
    stateMachine.update();

    if (stateMachine.getState() == NodeState::FAIL_SAFE) {
        ioManager.enterFailSafe();
    }

    if (getHardware()->serialAvailable() > 0) {
        char buffer[256];
        getHardware()->serialReadStringUntil('\n', buffer, sizeof(buffer));
        processMessage(buffer);
    }
}

void Communication::processMessage(const char* jsonString) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        sendError("UNKNOWN", "Malformed JSON");
        return;
    }

    const char* cmd = doc["cmd"];
    const char* msgId = doc["msgId"] | "UNKNOWN";

    if (!cmd) {
        sendError(msgId, "Missing 'cmd' field");
        return;
    }

    // Heartbeat logic
    // We only update heartbeat for valid commands to prevent garbage from keeping us alive
    stateMachine.heartbeatReceived();

    if (strcmp(cmd, "PING") == 0) {
        handlePing(doc.as<JsonObject>());
    } else if (strcmp(cmd, "QUERY_TELEMETRY") == 0) {
        handleQueryTelemetry(doc.as<JsonObject>());
    } else if (strcmp(cmd, "SET_ACTUATOR") == 0) {
        handleSetActuator(doc.as<JsonObject>());
    } else if (strcmp(cmd, "SET_ROLE") == 0) {
        handleSetRole(doc.as<JsonObject>());
    } else if (strcmp(cmd, "REBOOT") == 0) {
        handleReboot(doc.as<JsonObject>());
    } else {
        sendError(msgId, "Unknown command");
    }
}

void Communication::handlePing(JsonObject doc) {
    const char* msgId = doc["msgId"] | "UNKNOWN";

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleQueryTelemetry(JsonObject doc) {
    const char* msgId = doc["msgId"] | "UNKNOWN";


    StaticJsonDocument<256> response;
    response["msgId"] = msgId;
    response["status"] = "OK";
    JsonObject data = response.createNestedObject("data");
    data["state"] = (int)stateMachine.getState();

    char buffer[256];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleSetActuator(JsonObject doc) {
    const char* msgId = doc["msgId"] | "UNKNOWN";

    if (!doc.containsKey("pin") || !doc.containsKey("value")) {
        sendError(msgId, "Missing pin or value");
        return;
    }

    uint8_t pin = doc["pin"];
    uint8_t value = doc["value"];

    ioManager.setActuator(pin, value);

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleSetRole(JsonObject doc) {
    const char* msgId = doc["msgId"] | "UNKNOWN";

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleReboot(JsonObject doc) {
    const char* msgId = doc["msgId"] | "UNKNOWN";

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();

    getHardware()->reset();
}

void Communication::sendError(const char* msgId, const char* errorMsg) {
    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "ERROR";
    response["error"] = errorMsg;

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::sendIdentity() {
    StaticJsonDocument<128> response;
    response["msgId"] = "INIT";
    response["status"] = "OK";
    response["type"] = "MicroMax L1";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::sendStatus() {
    StaticJsonDocument<128> response;
    response["msgId"] = "STATUS";
    response["status"] = "OK";
    response["state"] = (int)stateMachine.getState();

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}
