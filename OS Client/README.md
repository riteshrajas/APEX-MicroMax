# MicroMax OS Client

MicroMax OS Client is a Rust + web UI companion app for device interaction and flashing workflows.

It serves a local frontend and exposes helper APIs for hardware-adjacent tasks (drive discovery and flash flow scaffolding).

## Navigation

- Root: [`../../README.md`](../../README.md)
- MicroMax hub: [`../README.md`](../README.md)
- Firmware module: [`../OS/README.md`](../OS/README.md)
- Hardware design docs: [`../../IOT/README.md`](../../IOT/README.md)

## Project Structure

```text
MicroMax/OS Client/
├── Cargo.toml
├── src/main.rs         # Axum server and API routes
├── index.html          # Frontend shell
├── app.js              # Frontend behavior
├── styles.css
├── manifest.json       # PWA manifest
└── service-worker.js
```

## API Endpoints

- `GET /health` → simple health check
- `GET /api/drives` → list removable writable drives
- `POST /flash` → prepares a flash command preview (execution intentionally placeholder)

## Run Locally

```powershell
cd ".\MicroMax\OS Client"
cargo run
```

Open: `http://127.0.0.1:4173`

## Install as Desktop App (PWA)

The client now supports desktop installation on Windows/macOS from Chromium-based browsers.

1. Run the server with `cargo run`.
2. Open `http://127.0.0.1:4173` in Chrome or Edge.
3. Click **Install App** in the UI (or use browser menu → **Install App**).
4. Launch **Apex OS Client** from your OS app launcher; it opens in standalone app mode.

## Optional Tooling

The backend prepares `image_writer_rs` command previews for flash workflows.

```powershell
cargo install image_writer_rs
```
