#include <Arduino.h>
#include "IO_Manager.h"
#include "Communication.h"

IO_Manager ioManager;
Communication comm(ioManager);

void setup() {
    ioManager.setCommunication(&comm);
    ioManager.begin();
    comm.begin(115200);
}

void loop() {
    comm.update();
    ioManager.update();
}
