#ifdef HAL_RP2040

#include "../../include/IHardware.h"
#include <Arduino.h>

class RP2040Hardware : public IHardware {
public:
    unsigned long getMillis() override {
        return millis();
    }

    void setPinMode(uint8_t pin, uint8_t mode) override {
        pinMode(pin, mode);
    }

    void writeDigital(uint8_t pin, uint8_t val) override {
        digitalWrite(pin, val);
    }

    int readDigital(uint8_t pin) override {
        return digitalRead(pin);
    }

    int readAnalog(uint8_t pin) override {
        return analogRead(pin);
    }

    void reset() override {
        NVIC_SystemReset();
    }

    void serialBegin(long baudRate) override {
        Serial.begin(baudRate);
    }

    int serialAvailable() override {
        return Serial.available();
    }

    void serialReadStringUntil(char terminator, char* buffer, size_t maxLength) override {
        String str = Serial.readStringUntil(terminator);
        str.toCharArray(buffer, maxLength);
    }

    void serialPrint(const char* str) override {
        Serial.print(str);
    }

    void serialPrintln() override {
        Serial.println();
    }
};

IHardware* getHardware() {
    static RP2040Hardware hw;
    return &hw;
}

#endif
