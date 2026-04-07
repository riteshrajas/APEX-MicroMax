# OS Apex Ecosystem: MicroMax (Level 1) Technical Specification

## 1. Overview
MicroMax nodes represent the physical "hands" of the Apex Core at the primary workstation. These are wired, tethered devices designed for high-reliability, low-latency interactions that do not require mobility or wireless overhead.

- **Tier Level:** 1
- **Connectivity:** USB 2.0/3.0 (Serial over USB)
- **Primary Function:** Localized I/O, workstation telemetry, and manual override controls.

## 2. Hardware Architecture
The following hardware platforms are officially supported for MicroMax deployment:
- **Arduino Uno/Nano:** Preferred for 5V logic and relay stabilization.
- **Raspberry Pi Pico (RP2040):** Preferred for high-speed sensor data and Python-based logic (MicroPython/CircuitPython).
- **Generic ATmega328P/ESP32 (Wired mode):** Supported via standard Serial baud rates.

## 3. Communication Protocol (Apex Serial)
To ensure interoperability, all MicroMax nodes must adhere to the Apex Serial Protocol (ASP).

### 3.1 Connection Parameters
- **Baud Rate:** 115,200
- **Data Format:** 8-N-1
- **Handshake:** Software (XON/XOFF) or None.

### 3.2 Message Schema
Communication is conducted via newline-terminated JSON strings.
- **Core to Node:** `{"action": "WRITE", "target": "D13", "value": 1}`
- **Node to Core:** `{"event": "TRIGGER", "source": "D2", "type": "Interrupt"}`
- **Identity Check:** `{"query": "WHO_ARE_YOU"}` -> `{"identity": "MICRO_MAX_RELAY_01", "version": "1.0.4"}`

## 4. Operational Requirements
### 4.1 Power
Nodes must be capable of operating on 500mA USB bus power. For relay banks exceeding 4 channels, an external 5V/12V DC power supply is mandatory to prevent Core workstation brownouts.

### 4.2 Fail-Safe States
In the event of communication loss (USB disconnect):
- **Relays:** Default to OFF (Normally Open).
- **Status LED:** Transition from solid Blue (Connected) to pulsing Red (Disconnected).

## 5. Software Stack
- **Firmware:** Apex-Standard C++ (Arduino) or MicroPython scripts.
- **Driver:** Standard CDC/ACM serial drivers.
- **Apex Core Module:** `MicroMaxManager.py` - responsible for port scanning, auto-identification, and routing messages to the LangChain orchestration layer.

## 6. Known Constraints (SWOT Analysis)
- **Strengths:** Zero latency, no Wi-Fi interference, low cost.
- **Weaknesses:** Limited range (USB cable length), consumes host USB ports.
- **Opportunities:** Can be used as a hardware watchdog for Apex Core.
- **Threats:** Ground loop noise from PC power supplies affecting sensitive analog sensors.
