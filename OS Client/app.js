const FIRMWARE_PROFILES = {
  uno: {
    id: "uno",
    label: "Arduino Uno",
    board: "atmelavr / uno",
    environment: "uno",
    buildCommand: 'platformio run -d "P:\\APEX\\OS" -e uno',
    flashCommand: (port) =>
      `platformio run -d "P:\\APEX\\OS" -e uno -t upload${port ? ` --upload-port ${port}` : ""}`,
  },
  pico: {
    id: "pico",
    label: "RP2040 Pico",
    board: "raspberrypi / pico",
    environment: "pico",
    buildCommand: 'platformio run -d "P:\\APEX\\OS" -e pico',
    flashCommand: (port) =>
      `platformio run -d "P:\\APEX\\OS" -e pico -t upload${port ? ` --upload-port ${port}` : ""}`,
  },
};

const DEFAULT_SCOPES = [
  {
    id: crypto.randomUUID(),
    name: "Desk Node Alpha",
    profile: "uno",
    expectedNodeId: "MMX-UNO-01",
    notes: "Primary workstation controller",
    relayState: [false, false],
  },
  {
    id: crypto.randomUUID(),
    name: "Desk Node Beta",
    profile: "pico",
    expectedNodeId: "MMX-RP2040-01",
    notes: "Fast telemetry and RGB node",
    relayState: [false, false],
  },
];

const STORAGE_KEY = "apex-os-client-scopes";
const POLL_MS = 2000;

const state = {
  scopes: loadScopes(),
  selectedScopeId: null,
  serial: {
    port: null,
    reader: null,
    writer: null,
    readableClosed: null,
    writableClosed: null,
    keepReading: false,
    autoPoll: false,
    pollHandle: null,
    connectedLabel: "",
    grantedPorts: [],
  },
  telemetry: {},
  nodeId: "",
  nodeStatus: "",
};

const els = {};

window.addEventListener("DOMContentLoaded", init);

function init() {
  bindElements();
  hydrateProfileOptions();
  wireEvents();
  state.selectedScopeId = state.scopes[0]?.id ?? null;
  render();
  updateBrowserSupport();
  refreshGrantedPorts();
}

function bindElements() {
  const ids = [
    "device-list",
    "device-name",
    "device-profile",
    "device-node-id",
    "device-notes",
    "save-device",
    "remove-device",
    "add-device",
    "browser-support",
    "connection-status",
    "selected-profile-chip",
    "authorize-port",
    "reconnect-port",
    "disconnect-port",
    "granted-port-list",
    "bound-scope",
    "port-label",
    "node-id",
    "node-status",
    "role-select",
    "state-select",
    "apply-role",
    "apply-state",
    "relay-1",
    "relay-2",
    "servo-angle",
    "apply-servo",
    "buzzer-frequency",
    "buzzer-duration",
    "buzz-button",
    "rgb-r",
    "rgb-g",
    "rgb-b",
    "rgb-duration",
    "apply-rgb",
    "raw-json",
    "send-raw-json",
    "toggle-auto-poll",
    "telemetry-role",
    "telemetry-core-state",
    "telemetry-status",
    "telemetry-uptime",
    "telemetry-temp",
    "telemetry-humidity",
    "telemetry-lux",
    "telemetry-presence",
    "firmware-profile-name",
    "firmware-board",
    "firmware-env",
    "firmware-port",
    "build-command",
    "flash-command",
    "copy-build-command",
    "copy-flash-command",
    "copy-spec-path",
    "clear-log",
    "log-output",
    "device-item-template",
  ];

  ids.forEach((id) => {
    els[id] = document.getElementById(id);
  });
}

function hydrateProfileOptions() {
  Object.values(FIRMWARE_PROFILES).forEach((profile) => {
    const option = document.createElement("option");
    option.value = profile.id;
    option.textContent = `${profile.label} (${profile.environment})`;
    els["device-profile"].appendChild(option);
  });
}

function wireEvents() {
  els["add-device"].addEventListener("click", addScope);
  els["save-device"].addEventListener("click", saveSelectedScope);
  els["remove-device"].addEventListener("click", removeSelectedScope);
  els["authorize-port"].addEventListener("click", requestAndConnectPort);
  els["reconnect-port"].addEventListener("click", reconnectGrantedPort);
  els["disconnect-port"].addEventListener("click", disconnectSerial);
  els["apply-role"].addEventListener("click", () => sendPayload({ action: "SET_ROLE", value: els["role-select"].value }));
  els["apply-state"].addEventListener("click", () => sendPayload({ action: "SET_STATE", value: els["state-select"].value }));
  els["relay-1"].addEventListener("click", () => toggleRelay(0));
  els["relay-2"].addEventListener("click", () => toggleRelay(1));
  els["apply-servo"].addEventListener("click", () => sendPayload({ action: "SET_SERVO", target: "SERVO_01", value: Number(els["servo-angle"].value) }));
  els["buzz-button"].addEventListener("click", () => sendPayload({ action: "BUZZ", target: "BUZZER_01", frequency: Number(els["buzzer-frequency"].value || 2200), duration: Number(els["buzzer-duration"].value || 150) }));
  els["apply-rgb"].addEventListener("click", () => sendPayload({ action: "SET_RGB", target: "LED_01", value: [Number(els["rgb-r"].value), Number(els["rgb-g"].value), Number(els["rgb-b"].value)], duration: Number(els["rgb-duration"].value || 500) }));
  els["send-raw-json"].addEventListener("click", sendRawJson);
  els["toggle-auto-poll"].addEventListener("click", toggleAutoPoll);
  els["copy-build-command"].addEventListener("click", () => copyText(els["build-command"].value));
  els["copy-flash-command"].addEventListener("click", () => copyText(els["flash-command"].value));
  els["copy-spec-path"].addEventListener("click", () => copyText("P:\\APEX\\OS\\SPEC.md"));
  els["firmware-port"].addEventListener("input", updateFirmwareCommands);
  els["clear-log"].addEventListener("click", () => { els["log-output"].textContent = ""; });

  document.querySelectorAll(".quick-command").forEach((button) => {
    button.addEventListener("click", () => sendPayload({ command: button.dataset.command }));
  });

  document.querySelectorAll(".quick-query").forEach((button) => {
    button.addEventListener("click", () => sendPayload({ query: button.dataset.query }));
  });

  if (navigator.serial) {
    navigator.serial.addEventListener("disconnect", async () => {
      log("Serial device disconnected");
      await disconnectSerial();
    });
  }
}

function loadScopes() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (!raw) return DEFAULT_SCOPES;
    const parsed = JSON.parse(raw);
    if (!Array.isArray(parsed) || parsed.length === 0) return DEFAULT_SCOPES;
    return parsed;
  } catch {
    return DEFAULT_SCOPES;
  }
}

function persistScopes() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(state.scopes));
}

function getSelectedScope() {
  return state.scopes.find((scope) => scope.id === state.selectedScopeId) ?? null;
}

function render() {
  renderScopeList();
  renderSelectedScopeFields();
  renderFirmwarePanel();
  renderConnectionSummary();
  renderTelemetry();
}

function renderScopeList() {
  const root = els["device-list"];
  root.innerHTML = "";
  const template = els["device-item-template"];

  state.scopes.forEach((scope) => {
    const node = template.content.firstElementChild.cloneNode(true);
    node.querySelector(".device-name").textContent = scope.name;
    node.querySelector(".device-meta").textContent = `${FIRMWARE_PROFILES[scope.profile]?.label ?? scope.profile} • ${scope.expectedNodeId || "No expected node"}`;
    if (scope.id === state.selectedScopeId) {
      node.classList.add("active");
    }
    node.addEventListener("click", () => {
      state.selectedScopeId = scope.id;
      render();
      refreshGrantedPorts();
    });
    root.appendChild(node);
  });
}

function renderSelectedScopeFields() {
  const scope = getSelectedScope();
  if (!scope) return;
  els["device-name"].value = scope.name || "";
  els["device-profile"].value = scope.profile || "uno";
  els["device-node-id"].value = scope.expectedNodeId || "";
  els["device-notes"].value = scope.notes || "";
  els["selected-profile-chip"].textContent = `Profile: ${FIRMWARE_PROFILES[scope.profile]?.label ?? "-"}`;
}
