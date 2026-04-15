#include "Communication.h"

Communication::Communication(IO_Manager& ioManager) : _ioManager(ioManager) {
    inputBuffer.reserve(384);
}

void Communication::begin(long baudRate) {
    Serial.begin(baudRate);
    while (!Serial) {
    }
}

void Communication::update() {
    while (Serial.available() > 0) {
        const char inChar = static_cast<char>(Serial.read());
        if (inChar == '\r') {
            continue;
        }

        if (inChar == '\n') {
            if (inputBuffer.length() > 0) {
                processMessage(inputBuffer);
                inputBuffer = "";
            }
        } else {
            inputBuffer += inChar;
        }
    }
}

void Communication::processMessage(const String& message) {
    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, message);
    if (error) {
        sendError("Failed to parse JSON");
        return;
    }

    _ioManager.noteHostContact();

    if (doc["query"].is<String>()) {
        handleQuery(doc);
        return;
    }

    if (doc["action"].is<String>() || doc["command"].is<String>()) {
        handleAction(doc);
        return;
    }

    sendError("Unsupported message");
}

void Communication::handleQuery(JsonDocument& doc) {
    const String query = doc["query"].as<String>();

    if (query == "WHO_ARE_YOU") {
        sendIdentity();
        return;
    }
    if (query == "GET_STATUS") {
        sendStatus();
        return;
    }
    if (query == "GET_TELEMETRY") {
        sendTelemetry();
        return;
    }
    if (query == "GET_CAPABILITIES") {
        JsonDocument response;
        response["status"] = "READY";
        response["node_id"] = _ioManager.getNodeId();
        _ioManager.populateCapabilities(response["capabilities"].to<JsonObject>());
        sendResponse(response);
        return;
    }

    sendError("Unknown query");
}

void Communication::handleAction(JsonDocument& doc) {
    const String action = doc["action"].is<String>()
        ? doc["action"].as<String>()
        : doc["command"].as<String>();

    if (doc["command"].is<String>()) {
        handleLegacyCommand(action, doc);
        return;
    }

    const String target = doc["target"] | "";

    if (action == "WRITE") {
        const int value = doc["value"] | 0;
        if (!_ioManager.writePin(target, value)) {
            sendError("Unknown write target");
            return;
        }

        JsonDocument response;
        response["status"] = "success";
        response["action"] = action;
        response["target"] = target;
        response["value"] = value;
        sendResponse(response);
        return;
    }

    if (action == "READ") {
        JsonDocument response;
        response["status"] = "success";
        response["action"] = action;
        response["target"] = target;
        response["value"] = _ioManager.readPin(target);
        sendResponse(response);
        return;
    }

    if (action == "CONFIG_WATCHDOG") {
        unsigned long timeout = parseDuration(doc["timeout"], 0);
        if (timeout == 0) {
            timeout = parseDuration(doc["value"], 5000UL);
        }
        _ioManager.configWatchdog(timeout);

        JsonDocument response;
        response["status"] = "success";
        response["action"] = action;
        response["timeout"] = timeout;
        sendResponse(response);
        return;
    }

    if (action == "SET_STATE") {
        const String stateValue = doc["value"] | "";
        if (!_ioManager.setCoreState(stateValue)) {
            sendError("Unknown core state");
            return;
        }

        sendAck("SET_STATE", stateValue);
        return;
    }

    if (action == "SET_ROLE") {
        const String roleValue = doc["value"] | "";
        if (!_ioManager.setRole(roleValue)) {
            sendError("Unknown role");
            return;
        }

        sendAck("SET_ROLE", roleValue);
        return;
    }

    if (action == "SET_RGB") {
        const unsigned long duration = parseDuration(doc["duration"], 0);
        JsonArrayConst values = doc["value"].as<JsonArrayConst>();
        if (!values.isNull() && values.size() >= 3) {
            _ioManager.setRgb(values[0] | 0, values[1] | 0, values[2] | 0, duration);
        } else {
            _ioManager.setRgb(doc["r"] | 0, doc["g"] | 0, doc["b"] | 0, duration);
        }

        sendAck("SET_RGB", target);
        return;
    }

    if (action == "SET_RELAY") {
        const bool enabled = parseBoolValue(doc["value"], false);
        if (target == "RELAY_01") {
            _ioManager.setRelay(0, enabled);
        } else if (target == "RELAY_02") {
            _ioManager.setRelay(1, enabled);
        } else {
            sendError("Unknown relay target");
            return;
        }

        sendAck("SET_RELAY", target);
        return;
    }

    if (action == "SET_SERVO") {
        const int angle = doc["value"] | 90;
        if (!_ioManager.setServoAngle(0, angle)) {
            sendError("Servo unavailable");
            return;
        }

        sendAck("SET_SERVO", target);
        return;
    }

    if (action == "ALERT" || action == "BUZZ") {
        const unsigned int frequency = doc["frequency"] | 2200;
        const unsigned long duration = parseDuration(doc["duration"], 150);
        _ioManager.buzz(frequency, duration);
        sendAck(action.c_str(), target);
        return;
    }

    if (action == "EMIT_TELEMETRY") {
        sendTelemetry();
        return;
    }

    if (action == "GET_STATUS") {
        sendStatus();
        return;
    }

    sendError("Unknown action");
}

void Communication::handleLegacyCommand(const String& command, JsonDocument& doc) {
    if (command == "ping") {
        JsonDocument response;
        response["status"] = "ready";
#if defined(ARDUINO_ARCH_RP2040)
        response["id"] = "micromax_rp2040_01";
#else
        response["id"] = "micromax_uno_01";
#endif
        response["capabilities"] = "Relay,Sensor,Watchdog,Telemetry";
        response["node_id"] = _ioManager.getNodeId();
        sendResponse(response);
        return;
    }

    if (command == "set_role") {
        const String role = doc["role"] | "reflex";
        if (!_ioManager.setRole(role)) {
            sendError("Unknown role");
            return;
        }

        JsonDocument response;
        response["status"] = "success";
        response["role"] = role;
        sendResponse(response);
        return;
    }

    if (command == "trigger") {
        if (doc["pin"].is<int>()) {
            const String pinTarget = "D" + String(doc["pin"].as<int>());
            const String stateString = doc["state"] | "LOW";
            _ioManager.writePin(pinTarget, stateString == "HIGH" ? 1 : 0);
        } else if ((doc["target"] | "") == "neopixel") {
            _ioManager.setRgb(doc["r"] | 0, doc["g"] | 0, doc["b"] | 0, 1200);
        }

        JsonDocument response;
        response["status"] = "success";
        sendResponse(response);
        return;
    }

    if (command == "telemetry_now") {
        sendTelemetry();
        return;
    }

    sendError("Unknown legacy command");
}

bool Communication::parseBoolValue(JsonVariantConst source, bool defaultValue) const {
    if (source.is<bool>()) {
        return source.as<bool>();
    }
    if (source.is<int>()) {
        return source.as<int>() != 0;
    }
    if (source.is<const char*>()) {
        const String text = source.as<const char*>();
        return text == "1" || text == "true" || text == "HIGH" || text == "ON";
    }
    return defaultValue;
}

unsigned long Communication::parseDuration(JsonVariantConst source, unsigned long defaultValue) const {
    if (source.is<unsigned long>()) {
        return source.as<unsigned long>();
    }
    if (source.is<int>()) {
        return static_cast<unsigned long>(source.as<int>());
    }
    return defaultValue;
}

void Communication::sendAck(const char* action, const String& target) {
    JsonDocument response;
    response["status"] = "success";
    response["action"] = action;
    if (target.length() > 0) {
        response["target"] = target;
    }
    sendResponse(response);
}

void Communication::sendIdentity() {
    JsonDocument response;
    _ioManager.populateIdentity(response);
    sendResponse(response);
}

void Communication::sendEvent(const char* source, const char* type) {
    JsonDocument event;
    event["event"] = "TRIGGER";
    event["node_id"] = _ioManager.getNodeId();
    event["source"] = source;
    event["type"] = type;
    event["presence"] = _ioManager.readPin("PRESENCE_01") == 1;
    sendResponse(event);
}

void Communication::sendTelemetry() {
    JsonDocument response;
    _ioManager.populateTelemetry(response);
    sendResponse(response);
}

void Communication::sendStatus() {
    JsonDocument response;
    _ioManager.populateStatus(response);
    sendResponse(response);
}

void Communication::sendResponse(const JsonDocument& response) {
    serializeJson(response, Serial);
    Serial.println();
}

void Communication::sendError(const char* message) {
    JsonDocument response;
    response["status"] = "error";
    response["error"] = message;
    response["node_id"] = _ioManager.getNodeId();
    sendResponse(response);
}
