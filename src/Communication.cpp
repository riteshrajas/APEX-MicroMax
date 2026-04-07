#include "Communication.h"

Communication::Communication(IO_Manager& ioManager) : _ioManager(ioManager) {
    inputBuffer.reserve(256);
}

void Communication::begin(long baudRate) {
    Serial.begin(baudRate);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB
    }
}

void Communication::update() {
    while (Serial.available() > 0) {
        char inChar = (char)Serial.read();
        if (inChar == '\n') {
            processMessage(inputBuffer);
            inputBuffer = "";
        } else {
            inputBuffer += inChar;
        }
    }
}

void Communication::processMessage(const String& message) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        sendError("Failed to parse JSON");
        return;
    }

    _ioManager.configWatchdog(0);

    if (doc["query"].is<String>()) {
        handleQuery(doc);
    } else if (doc["action"].is<String>() || doc["command"].is<String>()) {
        handleAction(doc);
    } else {
         // Some other format. We just use this as heartbeat.
    }
}

void Communication::handleQuery(JsonDocument& doc) {
    String query = doc["query"].as<String>();

    if (query == "WHO_ARE_YOU") {
        sendIdentity();
    }
}

void Communication::handleAction(JsonDocument& doc) {
    String action;
    if (doc["action"].is<String>()) {
        action = doc["action"].as<String>();
    } else if (doc["command"].is<String>()) {
        action = doc["command"].as<String>();
    }

    if (action == "WRITE") {
        String target = doc["target"].as<String>();
        int value = doc["value"].as<int>();
        _ioManager.writePin(target, value);

        JsonDocument response;
        response["status"] = "success";
        response["action"] = "WRITE";
        response["target"] = target;
        response["value"] = value;
        sendResponse(response);
    }
    else if (action == "READ") {
        String target = doc["target"].as<String>();
        int value = _ioManager.readPin(target);

        JsonDocument response;
        response["status"] = "success";
        response["action"] = "READ";
        response["target"] = target;
        response["value"] = value;
        sendResponse(response);
    }
    else if (action == "CONFIG_WATCHDOG") {
        unsigned long timeout = doc["timeout"].as<unsigned long>();
        _ioManager.configWatchdog(timeout);

        JsonDocument response;
        response["status"] = "success";
        response["action"] = "CONFIG_WATCHDOG";
        response["timeout"] = timeout;
        sendResponse(response);
    }
    else if (action == "ping") { // From Python tests
        JsonDocument response;
        response["status"] = "ready";
#ifdef ARDUINO_ARCH_RP2040
        response["id"] = "micromax_rp2040_01";
#else
        response["id"] = "micromax_uno_01";
#endif
        response["capabilities"] = "Relay,Sensor,Watchdog";
        sendResponse(response);
    }
    else if (action == "set_role") { // From Python tests
        String role = doc["role"].as<String>();
        JsonDocument response;
        response["status"] = "success";
        response["role"] = role;
        sendResponse(response);
    }
    else if (action == "trigger") { // From Python tests
        if (doc["pin"].is<int>()) {
             int pin = doc["pin"].as<int>();
             String stateStr = doc["state"].as<String>();
             int state = (stateStr == "HIGH") ? HIGH : LOW;
             _ioManager.writePin("D" + String(pin), state);
        } else if (doc["target"].is<String>() && doc["target"].as<String>() == "neopixel") {
            // Mock neopixel trigger
        }

        JsonDocument response;
        response["status"] = "success";
        sendResponse(response);
    }
}

void Communication::sendIdentity() {
    JsonDocument response;
#ifdef ARDUINO_ARCH_RP2040
    response["identity"] = "MICRO_MAX_01";
    response["hardware"] = "RP2040";
#else
    response["identity"] = "MICRO_MAX_01";
    response["hardware"] = "Atmega328P";
#endif
    response["version"] = "1.0.0";
    sendResponse(response);
}

void Communication::sendEvent(const char* source, const char* type) {
    JsonDocument event;
    event["event"] = "TRIGGER";
    event["source"] = source;
    event["type"] = type;
    sendResponse(event);
}

void Communication::sendResponse(const JsonDocument& response) {
    serializeJson(response, Serial);
    Serial.println();
}

void Communication::sendError(const char* message) {
    JsonDocument error;
    error["error"] = message;
    sendResponse(error);
}
