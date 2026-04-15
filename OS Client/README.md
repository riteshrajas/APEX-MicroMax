# Apex OS Client (Axum + PWA)

Rust-backed PWA for MicroMax, with Axum serving the frontend and hardware endpoints.

## Project Structure

```text
MicroMax/OS Client/
├── Cargo.toml
├── src/
│   └── main.rs                # Axum server + /flash + removable-drive discovery
├── index.html                 # Frontend entry
├── styles.css
├── app.js                     # Frontend logic + backend API helpers
├── manifest.json              # PWA manifest
├── service-worker.js          # Offline caching shell
└── README.md
```

## Backend Endpoints

- `GET /api/drives`: lists removable writable drives (via `sysinfo`)
- `POST /flash`: placeholder route for OS image flashing workflow
- `GET /health`: health check

## Run

1. `cd "P:\APEX\MicroMax\OS Client"`
2. `cargo run`
3. Open `http://127.0.0.1:4173`

## Crates Used

- `axum`: HTTP server and routing
- `sysinfo`: removable drive discovery
- `image_writer_rs`: expected writer tool in the flash pipeline (invoked by command preview in backend placeholder flow)

Install the writer tool if needed:

```bash
cargo install image_writer_rs
```
