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
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        sendError("UNKNOWN", "Malformed JSON");
        return;
    }

    // Support multiple field names for the command/action
    const char* cmd = doc["cmd"] | doc["action"] | doc["query"] | doc["command"];
    const char* msgId = doc["msgId"] | doc["id"] | "UNKNOWN";

    if (!cmd) {
        sendError(msgId, "Missing command field (cmd, action, or query)");
        return;
    }

    // Heartbeat logic
    stateMachine.heartbeatReceived();

    // Normalize command to uppercase for easier matching
    String cmdStr = String(cmd);
    cmdStr.toUpperCase();

    if (cmdStr == "PING") {
        handlePing(doc.as<JsonObject>());
    } else if (cmdStr == "QUERY_TELEMETRY" || cmdStr == "GET_TELEMETRY" || cmdStr == "TELEMETRY") {
        handleQueryTelemetry(doc.as<JsonObject>());
    } else if (cmdStr == "SET_ACTUATOR" || cmdStr == "WRITE" || cmdStr == "TOGGLE_RELAY" || cmdStr == "SET_SERVO") {
        handleSetActuator(doc.as<JsonObject>());
    } else if (cmdStr == "SET_ROLE") {
        handleSetRole(doc.as<JsonObject>());
    } else if (cmdStr == "REBOOT") {
        handleReboot(doc.as<JsonObject>());
    } else if (cmdStr == "WHO_ARE_YOU") {
        sendIdentity();
    } else if (cmdStr == "GET_STATUS" || cmdStr == "STATUS") {
        sendStatus();
    } else {
        sendError(msgId, "Unknown command");
    }
}

void Communication::handlePing(JsonObject doc) {
    const char* msgId = doc["msgId"] | doc["id"] | "UNKNOWN";

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";
    response["v"] = "2.0";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleQueryTelemetry(JsonObject doc) {
    const char* msgId = doc["msgId"] | doc["id"] | "UNKNOWN";

    StaticJsonDocument<512> response;
    response["msgId"] = msgId;
    response["status"] = "OK";
    response["v"] = "2.0";
    JsonObject data = response.createNestedObject("data");
    data["state"] = (int)stateMachine.getState();
    data["uptime"] = getHardware()->getMillis() / 1000;

    JsonObject pins = data.createNestedObject("pins");
    for (uint8_t i = 0; i < ioManager.getMaxPins(); i++) {
        if (ioManager.isConfigured(i)) {
            char pinName[4];
            snprintf(pinName, sizeof(pinName), "D%d", i);
            pins[pinName] = ioManager.getPinState(i);
        }
    }

    char buffer[512];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleSetActuator(JsonObject doc) {
    const char* msgId = doc["msgId"] | doc["id"] | "UNKNOWN";

    uint8_t pin = 255;
    uint8_t value = 0;

    // Resolve Pin
    if (doc.containsKey("pin")) {
        pin = doc["pin"];
    } else if (doc.containsKey("target")) {
        const char* target = doc["target"];
        if (target[0] == 'D') {
            pin = atoi(target + 1);
        } else {
            pin = atoi(target);
        }
    } else if (doc.containsKey("index")) {
        // Default relay mapping for MicroMax
        uint8_t index = doc["index"];
        if (index == 1) pin = 2;
        else if (index == 2) pin = 3;
    }

    if (pin == 255) {
        sendError(msgId, "Missing or invalid pin/target/index");
        return;
    }

    // Resolve Value
    if (doc.containsKey("value")) {
        value = doc["value"];
    } else if (doc.containsKey("angle")) {
        value = doc["angle"];
    } else {
        value = 1; // Default to HIGH/ON
    }

    ioManager.setActuator(pin, value);

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";
    response["v"] = "2.0";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleSetRole(JsonObject doc) {
    const char* msgId = doc["msgId"] | doc["id"] | "UNKNOWN";

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";
    response["v"] = "2.0";

    char buffer[128];
    serializeJson(response, buffer);
    getHardware()->serialPrint(buffer);
    getHardware()->serialPrintln();
}

void Communication::handleReboot(JsonObject doc) {
    const char* msgId = doc["msgId"] | doc["id"] | "UNKNOWN";

    StaticJsonDocument<128> response;
    response["msgId"] = msgId;
    response["status"] = "OK";
    response["v"] = "2.0";

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
