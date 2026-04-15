use axum::{
    extract::State,
    http::StatusCode,
    response::IntoResponse,
    routing::{get, post},
    Json, Router,
};
use serde::{Deserialize, Serialize};
use std::{
    path::{Path, PathBuf},
    process::Command,
    sync::Arc,
};
use sysinfo::{DiskKind, Disks};
use tower_http::services::ServeDir;

#[derive(Clone)]
struct AppState {
    image_root: Arc<PathBuf>,
}

#[derive(Debug, Serialize)]
struct DriveInfo {
    name: String,
    mount_point: String,
    file_system: String,
    total_space_bytes: u64,
    available_space_bytes: u64,
    removable: bool,
    disk_kind: String,
}

#[derive(Debug, Deserialize)]
struct FlashRequest {
    image_path: String,
    drive_mount_point: String,
    dry_run: Option<bool>,
}

#[derive(Debug, Serialize)]
struct FlashResponse {
    status: String,
    image_path: String,
    drive_mount_point: String,
    command_preview: String,
    note: String,
}

#[tokio::main]
async fn main() {
    let state = AppState {
        image_root: Arc::new(PathBuf::from(".")),
    };

    let app = Router::new()
        .route("/health", get(health))
        .route("/api/drives", get(list_removable_drives))
        .route("/flash", post(flash_image))
        .fallback_service(ServeDir::new("."))
        .with_state(state);

    let listener = tokio::net::TcpListener::bind("0.0.0.0:4173")
        .await
        .expect("Failed to bind 0.0.0.0:4173");

    println!("Apex OS Client server listening on http://127.0.0.1:4173");
    axum::serve(listener, app)
        .await
        .expect("Axum server terminated unexpectedly");
}

async fn health() -> &'static str {
    "ok"
}

async fn list_removable_drives() -> Json<Vec<DriveInfo>> {
    let disks = Disks::new_with_refreshed_list();
    let drives = disks
        .list()
        .iter()
        .filter(|disk| disk.is_removable() && !disk.is_read_only())
        .map(|disk| DriveInfo {
            name: disk.name().to_string_lossy().to_string(),
            mount_point: disk.mount_point().to_string_lossy().to_string(),
            file_system: disk.file_system().to_string_lossy().to_string(),
            total_space_bytes: disk.total_space(),
            available_space_bytes: disk.available_space(),
            removable: disk.is_removable(),
            disk_kind: disk_kind_label(disk.kind()),
        })
        .collect::<Vec<_>>();

    Json(drives)
}

async fn flash_image(
    State(state): State<AppState>,
    Json(request): Json<FlashRequest>,
) -> impl IntoResponse {
    let requested_image = PathBuf::from(&request.image_path);
    let image_path = resolve_image_path(&state.image_root, &requested_image);

    if !image_path.exists() {
        return (
            StatusCode::BAD_REQUEST,
            Json(serde_json::json!({
                "error": "Image file does not exist",
                "image_path": image_path.to_string_lossy().to_string()
            })),
        )
            .into_response();
    }

    let mut command = Command::new("image_writer_rs");
    command.arg(image_path.to_string_lossy().to_string());
    command.arg(&request.drive_mount_point);

    let command_preview = format!(
        "image_writer_rs \"{}\" \"{}\"",
        image_path.to_string_lossy(),
        request.drive_mount_point
    );

    if request.dry_run.unwrap_or(true) {
        let response = FlashResponse {
            status: "prepared".to_string(),
            image_path: image_path.to_string_lossy().to_string(),
            drive_mount_point: request.drive_mount_point,
            command_preview,
            note: "Dry-run only. Replace this path with actual write execution once target flow is finalized."
                .to_string(),
        };
        return (StatusCode::ACCEPTED, Json(response)).into_response();
    }

    (
        StatusCode::NOT_IMPLEMENTED,
        Json(serde_json::json!({
            "status": "not_implemented",
            "command_preview": command_preview,
            "error": "POST /flash execution is intentionally a placeholder."
        })),
    )
        .into_response()
}

fn resolve_image_path(base: &Path, requested: &Path) -> PathBuf {
    if requested.is_absolute() {
        requested.to_path_buf()
    } else {
        base.join(requested)
    }
}

fn disk_kind_label(kind: DiskKind) -> String {
    match kind {
        DiskKind::HDD => "HDD".to_string(),
        DiskKind::SSD => "SSD".to_string(),
        _ => "Unknown".to_string(),
    }
}
