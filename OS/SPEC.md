# MicroMax Level 1 Runtime

## Overview

MicroMax is a wired reflex node for Apex Core. It does not make autonomous decisions. It mirrors host state, actuates local hardware, emits raw telemetry, and enters fail-safe when the serial heartbeat expires.

- Tier: `1`
- Transport: `USB CDC serial`
- Baud: `115200`
- Framing: `8-N-1`, newline-terminated JSON
- Firmware version: `2.0.0`

## Runtime Model

The firmware exposes three roles:

- `reflex`: default mixed I/O mode
- `sentry`: telemetry-first mode for presence and environment reporting
- `action`: actuation-first mode for relays, RGB, buzzer, and servo actions

The host can also mirror its own state into the node:

- `IDLE`
- `BUSY`
- `ALERT`
- `DISCONNECTED`

Those states drive the status indicator behavior locally even if no UI is visible.

## Supported Targets

- `STATUS_LED` / `LED_01`: RGB status light via PWM pins
- `RELAY_01`, `RELAY_02`
- `SERVO_01`
- `BUZZER_01`
- `PRESENCE_01`
- `LUX_01`
- `TEMP_01`
- `HUMIDITY_01`
- Generic GPIO aliases like `D13`, `A0`

## Commands

Core to node examples:

```json
{"query":"WHO_ARE_YOU"}
{"query":"GET_STATUS"}
{"query":"GET_TELEMETRY"}
{"query":"GET_CAPABILITIES"}
{"action":"SET_STATE","value":"BUSY"}
{"action":"SET_ROLE","value":"sentry"}
{"action":"SET_RGB","target":"LED_01","value":[255,0,0],"duration":500}
{"action":"SET_RELAY","target":"RELAY_01","value":1}
{"action":"SET_SERVO","target":"SERVO_01","value":120}
{"action":"BUZZ","target":"BUZZER_01","frequency":2400,"duration":200}
{"action":"CONFIG_WATCHDOG","timeout":5000}
{"action":"WRITE","target":"D13","value":1}
{"action":"READ","target":"A0"}
```

Legacy compatibility commands are still accepted for the existing Python diagnostics:

```json
{"command":"ping"}
{"command":"set_role","role":"sentry"}
{"command":"trigger","pin":13,"state":"HIGH"}
{"command":"trigger","target":"neopixel","r":255,"g":0,"b":0}
```

## Node Responses

Identity:

```json
{
  "identity": "MMX-UNO-01",
  "node_id": "MMX-UNO-01",
  "hardware": "ATmega328P",
  "version": "2.0.0",
  "tier": 1,
  "transport": "usb-serial"
}
```

Telemetry:

```json
{
  "node_id": "MMX-UNO-01",
  "status": "READY",
  "role": "reflex",
  "core_state": "IDLE",
  "uptime_ms": 12345,
  "sensors": {
    "temp": 24.5,
    "humidity": 41.2,
    "lux": 150,
    "presence": true,
    "temp_raw": 390,
    "humidity_raw": 301,
    "lux_raw": 154
  }
}
```

Interrupt event:

```json
{
  "event": "TRIGGER",
  "node_id": "MMX-UNO-01",
  "source": "D2",
  "type": "INTERRUPT",
  "presence": true
}
```

## Pin Map

Default wiring in the current firmware:

- `D2`: presence interrupt input
- `D3`: auxiliary interrupt input
- `D4`, `D7`: relay outputs
- `D5`: buzzer
- `D6`: servo
- `D9`, `D10`, `D11`: RGB LED PWM
- `A0`: lux / LDR
- `A1`: temperature analog input
- `A2`: humidity analog input

## Fail-Safe Behavior

If host heartbeats stop longer than the configured watchdog interval:

- node state becomes `DISCONNECTED`
- status becomes `FAIL_SAFE`
- all relays are forced `OFF`
- buzzer is silenced
- servo returns to `90`
- indicator shifts to pulsing alert behavior

Hardware watchdog support is enabled where the active board core exposes it:

- AVR: `avr/wdt.h`
- RP2040: `hardware/watchdog.h`
