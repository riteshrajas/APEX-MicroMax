/**
 * @file main.cpp
 * @author APEX Ecosystem
 * @brief Level 1 Node (MicroMax) Entry Point.
 * 
 * This firmware implements the "wired reflex" behavior for Level 1 APEX nodes.
 * It provides deterministic hardware control via serial JSON commands and 
 * ensures safety via a host-linked watchdog timer.
 * 
 * @version 2.1.0
 * @date 2026-04-16
 */

#include <Arduino.h>
#include "IO_Manager.h"
#include "Communication.h"

// System-wide managers
IO_Manager ioManager;
Communication comm(ioManager);

/**
 * @brief Standard Arduino Setup
 * Initializes serial communication, hardware IO, and system status.
 */
void setup() {
    // Initialize serial communication bridge
    ioManager.setCommunication(&comm);
    comm.begin(115200);
    
    // Initialize pins, sensors, and internal state
    ioManager.begin();
    
    // Announce identity and current status to APEX Core
    comm.sendIdentity();
    comm.sendStatus();
}

/**
 * @brief Main Execution Loop
 * Updates communication buffers and handles real-time IO/Telemetry.
 */
void loop() {
    // Poll for and process incoming serial messages
    comm.update();
    
    // Maintain hardware states, sensor readings, and watchdog health
    ioManager.update();
}
