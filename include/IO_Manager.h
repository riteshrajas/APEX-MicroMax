#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <Arduino.h>

class Communication; // Forward declaration

class IO_Manager {
public:
    IO_Manager();
    void begin();
    void update();
    void setCommunication(Communication* comm);

    void configWatchdog(unsigned long timeout);
    void writePin(const String& target, int value);
    int readPin(const String& target);

    // Made public so ISRs can call it if needed, or handled inside IO_Manager
    void handleInterrupt(int pin);

private:
    unsigned long _lastHeartbeat;
    unsigned long _watchdogTimeout;
    bool _isFailSafe;
    Communication* _comm;

    void enterFailSafe();
    int getPinNumber(const String& target);
    bool isAnalog(const String& target);
};

#endif // IO_MANAGER_H
