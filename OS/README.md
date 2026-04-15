# MicroMax OS Firmware

MicroMax OS is the embedded Level 1 runtime that executes deterministic hardware actions from host commands over USB serial.

## Navigation

- Root: [`../../README.md`](../../README.md)
- MicroMax hub: [`../README.md`](../README.md)
- OS Client: [`../OS Client/README.md`](../OS%20Client/README.md)
- Hardware design: [`../../IOT/README.md`](../../IOT/README.md)

## What it does

- Runs on **Arduino Uno** and **RP2040 Pico** targets via PlatformIO
- Accepts newline-delimited JSON commands over USB CDC serial (`115200`, `8-N-1`)
- Exposes reflex/sentry/action roles, telemetry, and fail-safe behavior

See full protocol and pin-level behavior in [`SPEC.md`](./SPEC.md).

## Structure

```text
MicroMax/OS/
├── platformio.ini   # Build environments: uno, pico
├── src/             # Firmware implementation
├── include/         # Firmware headers
└── SPEC.md          # Protocol + runtime spec
```

## Build

```powershell
cd .\MicroMax\OS
platformio run -e uno
platformio run -e pico
```

## Upload

```powershell
cd .\MicroMax\OS
platformio run -e uno -t upload --upload-port COM4
platformio run -e pico -t upload --upload-port COM5
```

## Serial Monitor

```powershell
cd .\MicroMax\OS
platformio device monitor -b 115200
```
