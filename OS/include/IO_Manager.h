#pragma once

#include "IHardware.h"

class Communication;

#ifndef MAX_PINS_COUNT
#define MAX_PINS_COUNT 20
#endif

class IO_Manager {
private:
    IHardware* hw;
    Communication* comm;

    static const uint8_t MAX_PINS = MAX_PINS_COUNT;
    uint8_t safeStates[MAX_PINS];
    uint8_t currentStates[MAX_PINS];
    bool isPinConfigured[MAX_PINS];

public:
    IO_Manager() : hw(getHardware()), comm(nullptr) {
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            safeStates[i] = 0;
            currentStates[i] = 0;
            isPinConfigured[i] = false;
        }
    }

    void setCommunication(Communication* c) { comm = c; }

    void begin() {}

    void update() {}

    void setActuator(uint8_t pin, uint8_t value) {
        if (pin < MAX_PINS) {
            hw->writeDigital(pin, value);
            currentStates[pin] = value;
            isPinConfigured[pin] = true;
        }
    }

    void setSafePosition(uint8_t pin, uint8_t safeValue) {
        if (pin < MAX_PINS) {
            safeStates[pin] = safeValue;
            isPinConfigured[pin] = true;
        }
    }

    void enterFailSafe() {
        for (uint8_t i = 0; i < MAX_PINS; i++) {
            if (isPinConfigured[i]) {
                setActuator(i, safeStates[i]);
            }
        }
    }

    bool isConfigured(uint8_t pin) const {
        return pin < MAX_PINS && isPinConfigured[pin];
    }

    uint8_t getPinState(uint8_t pin) const {
        return (pin < MAX_PINS) ? currentStates[pin] : 0;
    }

    uint8_t getMaxPins() const {
        return MAX_PINS;
    }
};
