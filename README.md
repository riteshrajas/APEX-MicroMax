# APEX-MicroMax (Level 1 Node)

## Overview
MicroMax is the baseline "wired reflex" node for the APEX ecosystem. These nodes are tethered via USB (Serial) and provide low-latency, deterministic control for workstation-adjacent hardware.

## Hardware Support
- **Arduino Uno / Nano** (ATmega328P)
- **Raspberry Pi Pico** (RP2040)
- **ESP32** (Wired mode)

## Capabilities
- **Wired IoT Endpoint:** USB CDC Serial communication.
- **Actuation:** Relays, Servos, RGB LEDs, Buzzers.
- **Sensing:** Environmental (Temp/Humidity), Light (LDR), Presence (PIR).
- **Fail-Safe:** Local watchdog logic to revert to safe states on host disconnect.

## Protocol
Uses the **Apex Serial Protocol (ASP)**: a newline-terminated JSON framing protocol running at 115200 baud.

---
Part of the [APEX Ecosystem](https://github.com/riteshrajas/APEX)
