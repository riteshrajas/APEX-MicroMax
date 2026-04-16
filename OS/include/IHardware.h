#pragma once

#include <stdint.h>
#include <stddef.h>
#include <ArduinoJson.h>

class IHardware {
public:
    virtual ~IHardware() = default;

    // Time
    virtual unsigned long getMillis() = 0;

    // GPIO
    virtual void setPinMode(uint8_t pin, uint8_t mode) = 0;
    virtual void writeDigital(uint8_t pin, uint8_t val) = 0;
    virtual int readDigital(uint8_t pin) = 0;
    virtual int readAnalog(uint8_t pin) = 0;

    // System
    virtual void reset() = 0;

    // Serial Communication
    virtual void serialBegin(long baudRate) = 0;
    virtual int serialAvailable() = 0;
    virtual void serialReadStringUntil(char terminator, char* buffer, size_t maxLength) = 0;
    virtual void serialPrint(const char* str) = 0;
    virtual void serialPrintln() = 0;
};

// Global HAL instance provider
IHardware* getHardware();
