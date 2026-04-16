#ifndef HAL_AVR
#ifndef HAL_RP2040
#ifndef HAL_ESP32

#include "../../include/IHardware.h"
#include <string>
#include <queue>
#include <cstring>

// Extended interface internally for tests
class INativeHardware : public IHardware {
public:
    virtual void mockAdvanceTime(unsigned long ms) = 0;
    virtual void mockSerialPush(const char* msg) = 0;
    virtual std::string mockSerialPopOutput() = 0;
    virtual void resetMockMillis() = 0;
};

// Mock implementation for native testing
class NativeHardware : public INativeHardware {
public:
    unsigned long mockMillis = 0;
    std::queue<std::string> serialInputBuffer;
    std::string serialOutputBuffer;

    void mockAdvanceTime(unsigned long ms) override {
        mockMillis += ms;
    }

    void mockSerialPush(const char* msg) override {
        serialInputBuffer.push(std::string(msg));
    }

    std::string mockSerialPopOutput() override {
        std::string ret = serialOutputBuffer;
        serialOutputBuffer.clear();
        return ret;
    }

    void resetMockMillis() override {
        mockMillis = 0;
    }

    unsigned long getMillis() override {
        return mockMillis;
    }

    void setPinMode(uint8_t pin, uint8_t mode) override {}
    void writeDigital(uint8_t pin, uint8_t val) override {}
    int readDigital(uint8_t pin) override { return 0; }
    int readAnalog(uint8_t pin) override { return 0; }
    void reset() override {}

    void serialBegin(long baudRate) override {}

    int serialAvailable() override {
        return serialInputBuffer.empty() ? 0 : 1;
    }

    void serialReadStringUntil(char terminator, char* buffer, size_t maxLength) override {
        if (!serialInputBuffer.empty()) {
            std::string str = serialInputBuffer.front();
            serialInputBuffer.pop();
            strncpy(buffer, str.c_str(), maxLength);
            buffer[maxLength - 1] = '\0';
        } else {
            buffer[0] = '\0';
        }
    }

    void serialPrint(const char* str) override {
        serialOutputBuffer += str;
    }

    void serialPrintln() override {
        serialOutputBuffer += "\n";
    }
};

IHardware* getHardware() {
    static NativeHardware hw;
    return &hw;
}

#endif
#endif
#endif
