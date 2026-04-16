#if !defined(HAL_NATIVE)
#include <Arduino.h>
#endif
#include "../include/IO_Manager.h"
#include "../include/Communication.h"

#ifndef UNIT_TEST
IO_Manager ioManager;
Communication comm(ioManager);

void setup() {
    ioManager.setCommunication(&comm);
    comm.begin(115200);
    ioManager.begin();
    
    comm.sendIdentity();
    comm.sendStatus();
}

void loop() {
    comm.update();
    ioManager.update();
}

#if defined(HAL_NATIVE)
int main() {
    setup();
    while(true) {
        loop();
    }
    return 0;
}
#endif
#endif
