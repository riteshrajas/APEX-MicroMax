# MicroMax

MicroMax is the hardware execution layer for APEX. It includes embedded firmware and a companion client used for control/flash workflows.

## Navigation

- Root: [`../README.md`](../README.md)
- Firmware module: [`./OS/README.md`](./OS/README.md)
- Client module: [`./OS Client/README.md`](./OS%20Client/README.md)
- Hardware design docs: [`../IOT/README.md`](../IOT/README.md)

## Modules

| Module | Purpose |
| --- | --- |
| `OS` | Embedded firmware runtime (PlatformIO) |
| `OS Client` | Browser + Rust-backed helper app for serial/flash workflows |

## Structure

```text
MicroMax/
├── OS/
└── OS Client/
```

## Quick Commands

```powershell
# Build firmware
cd .\MicroMax\OS
platformio run -e uno
platformio run -e pico

# Start OS Client server
cd "..\OS Client"
cargo run
```

