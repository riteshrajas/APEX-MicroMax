# Apex OS Client

Chrome-based JavaScript web app for MicroMax.

## What It Does

- Maintains a scoped devices list in local storage
- Connects to devices through Web Serial
- Sends Level 1 runtime commands and raw JSON
- Displays live telemetry and status
- Generates PlatformIO build and flash commands for the selected firmware profile

## Open It

Use Chromium, Chrome, or Edge and open:

- `P:\APEX\MicroMax\OS Client\index.html`

For the best behavior, serve it from a lightweight local HTTP server instead of `file://`, but Chromium Web Serial is the main requirement.

## Flashing

The app can assist flashing by generating the correct commands for the selected scope:

- `platformio run -d "P:\APEX\MicroMax\OS" -e uno -t upload --upload-port COM4`
- `platformio run -d "P:\APEX\MicroMax\OS" -e pico -t upload --upload-port COM5`

Browsers cannot run those commands directly, so the client focuses on:

- selecting the right firmware profile
- binding it to a scoped device
- copying the exact build and upload commands
- handling direct runtime interaction over serial
