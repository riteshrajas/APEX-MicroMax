# Apex OS

This directory contains the current Level 1 MicroMax firmware snapshot and supporting documentation.

## Contents

- `platformio.ini`: PlatformIO environments for `uno` and `pico`
- `SPEC.md`: serial protocol, pin map, node behavior, and fail-safe model
- `src/`: firmware runtime
- `include/`: firmware interfaces

## Build

```powershell
cd P:\APEX\MicroMax\OS
platformio run -e uno
platformio run -e pico
```

## Upload

```powershell
cd P:\APEX\MicroMax\OS
platformio run -e uno -t upload --upload-port COM4
platformio run -e pico -t upload --upload-port COM5
```

## Notes

- The browser client in `P:\APEX\MicroMax\OS Client` can interact with devices directly over Web Serial.
- Browser code cannot execute PlatformIO itself, so the client generates the exact upload commands for flashing.
