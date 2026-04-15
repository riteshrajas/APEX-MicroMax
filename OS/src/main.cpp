#include <Arduino.h>
#include "IO_Manager.h"
#include "Communication.h"

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
