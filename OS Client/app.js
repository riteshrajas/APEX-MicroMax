const FIRMWARE_PROFILES = {
  uno: {
    id: "uno",
    label: "Arduino Uno",
    board: "atmelavr / uno",
    environment: "uno",
    buildCommand: 'platformio run -d "P:\\APEX\\MicroMax\\OS" -e uno',
    flashCommand: (port) =>
      `platformio run -d "P:\\APEX\\MicroMax\\OS" -e uno -t upload${port ? ` --upload-port ${port}` : ""}`,
  },
  pico: {
    id: "pico",
    label: "RP2040 Pico",
    board: "raspberrypi / pico",
    environment: "pico",
    buildCommand: 'platformio run -d "P:\\APEX\\MicroMax\\OS" -e pico',
    flashCommand: (port) =>
      `platformio run -d "P:\\APEX\\MicroMax\\OS" -e pico -t upload${port ? ` --upload-port ${port}` : ""}`,
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
  registerServiceWorker();
  bindElements();
  hydrateProfileOptions();
  wireEvents();
  state.selectedScopeId = state.scopes[0]?.id ?? null;
  render();
  updateBrowserSupport();
  refreshGrantedPorts();
}

function registerServiceWorker() {
  if (!("serviceWorker" in navigator)) return;
  navigator.serviceWorker.register("/service-worker.js").catch((error) => {
    console.error("Service worker registration failed:", error);
  });
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
  els["copy-spec-path"].addEventListener("click", () => copyText("P:\\APEX\\MicroMax\\OS\\SPEC.md"));
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

function renderFirmwarePanel() {
  const scope = getSelectedScope();
  const profile = FIRMWARE_PROFILES[scope?.profile || "uno"];
  els["firmware-profile-name"].textContent = profile.label;
  els["firmware-board"].textContent = profile.board;
  els["firmware-env"].textContent = profile.environment;
  updateFirmwareCommands();
}

function updateFirmwareCommands() {
  const scope = getSelectedScope();
  if (!scope) return;
  const profile = FIRMWARE_PROFILES[scope.profile];
  const port = els["firmware-port"].value.trim();
  els["build-command"].value = profile.buildCommand;
  els["flash-command"].value = profile.flashCommand(port);
}

function renderConnectionSummary() {
  const scope = getSelectedScope();
  els["bound-scope"].textContent = scope?.name ?? "-";
  els["port-label"].textContent = state.serial.connectedLabel || "-";
  els["node-id"].textContent = state.nodeId || scope?.expectedNodeId || "-";
  els["node-status"].textContent = state.nodeStatus || "-";

  const connectionChip = els["connection-status"];
  connectionChip.textContent = state.serial.port ? "Connected" : "Disconnected";
  connectionChip.className = `status-chip ${state.serial.port ? "ok" : "warn"}`;
}

function renderTelemetry() {
  const telemetry = state.telemetry;
  const sensors = telemetry.sensors || {};

  els["telemetry-role"].textContent = telemetry.role ?? "-";
  els["telemetry-core-state"].textContent = telemetry.core_state ?? "-";
  els["telemetry-status"].textContent = telemetry.status ?? "-";
  els["telemetry-uptime"].textContent = telemetry.uptime_ms ?? "-";
  els["telemetry-temp"].textContent = sensors.temp ?? "-";
  els["telemetry-humidity"].textContent = sensors.humidity ?? "-";
  els["telemetry-lux"].textContent = sensors.lux ?? "-";
  els["telemetry-presence"].textContent =
    typeof sensors.presence === "boolean" ? String(sensors.presence) : "-";
}

function updateBrowserSupport() {
  const chip = els["browser-support"];
  if ("serial" in navigator) {
    chip.textContent = "Web Serial available";
    chip.className = "status-chip ok";
  } else {
    chip.textContent = "Use Chrome / Edge / Chromium";
    chip.className = "status-chip bad";
  }
}

function addScope() {
  const name = `Scoped Device ${state.scopes.length + 1}`;
  const scope = {
    id: crypto.randomUUID(),
    name,
    profile: "uno",
    expectedNodeId: "",
    notes: "",
    relayState: [false, false],
  };
  state.scopes.push(scope);
  state.selectedScopeId = scope.id;
  persistScopes();
  render();
  log(`Added new scope: ${name}`);
}

function saveSelectedScope() {
  const scope = getSelectedScope();
  if (!scope) return;

  scope.name = els["device-name"].value.trim() || "Unnamed Scope";
  scope.profile = els["device-profile"].value;
  scope.expectedNodeId = els["device-node-id"].value.trim();
  scope.notes = els["device-notes"].value.trim();
  persistScopes();
  render();
  log(`Saved scope: ${scope.name}`);
}

function removeSelectedScope() {
  const scope = getSelectedScope();
  if (!scope) return;
  if (state.scopes.length === 1) {
    window.alert("Keep at least one scope.");
    return;
  }
  const confirmed = window.confirm(`Delete scope "${scope.name}"?`);
  if (!confirmed) return;
  state.scopes = state.scopes.filter((item) => item.id !== scope.id);
  state.selectedScopeId = state.scopes[0]?.id ?? null;
  persistScopes();
  render();
  log(`Deleted scope: ${scope.name}`);
}

async function refreshGrantedPorts() {
  if (!navigator.serial) return;
  const select = els["granted-port-list"];
  select.innerHTML = "";
  const ports = await navigator.serial.getPorts();
  state.serial.grantedPorts = ports;

  if (ports.length === 0) {
    const option = document.createElement("option");
    option.textContent = "No granted ports";
    option.value = "";
    select.appendChild(option);
    return;
  }

  ports.forEach((port, index) => {
    const option = document.createElement("option");
    option.value = String(index);
    option.textContent = formatPortInfo(port.getInfo(), index);
    select.appendChild(option);
  });
}

async function requestAndConnectPort() {
  if (!navigator.serial) {
    window.alert("Web Serial is not available in this browser.");
    return;
  }

  try {
    const port = await navigator.serial.requestPort();
    await connectPort(port);
    await refreshGrantedPorts();
  } catch (error) {
    log(`Connect cancelled or failed: ${error.message}`);
  }
}

async function reconnectGrantedPort() {
  if (!navigator.serial) return;
  const index = Number(els["granted-port-list"].value || 0);
  const port = state.serial.grantedPorts[index];
  if (!port) {
    window.alert("No granted port selected.");
    return;
  }
  await connectPort(port);
}

async function connectPort(port) {
  await disconnectSerial();
  await port.open({ baudRate: 115200 });
  state.serial.port = port;
  state.serial.connectedLabel = formatPortInfo(port.getInfo(), 0);
  els["firmware-port"].value = guessWindowsPortLabel(port.getInfo());
  updateFirmwareCommands();

  const textEncoder = new TextEncoderStream();
  state.serial.writableClosed = textEncoder.readable.pipeTo(port.writable);
  state.serial.writer = textEncoder.writable.getWriter();

  const textDecoder = new TextDecoderStream();
  state.serial.readableClosed = port.readable.pipeTo(textDecoder.writable);
  state.serial.reader = textDecoder.readable
    .pipeThrough(new TransformStream(new LineBreakTransformer()))
    .getReader();

  state.serial.keepReading = true;
  readLoop().catch((error) => log(`Read loop ended: ${error.message}`));
  renderConnectionSummary();
  log(`Connected to ${state.serial.connectedLabel}`);
  await bootstrapConnection();
}

async function bootstrapConnection() {
  await sendPayload({ query: "WHO_ARE_YOU" });
  await sendPayload({ query: "GET_STATUS" });
  await sendPayload({ query: "GET_TELEMETRY" });
}

async function disconnectSerial() {
  stopAutoPoll();
  state.serial.keepReading = false;

  if (state.serial.reader) {
    try {
      await state.serial.reader.cancel();
    } catch {}
    state.serial.reader.releaseLock();
    state.serial.reader = null;
  }

  if (state.serial.writer) {
    try {
      await state.serial.writer.close();
    } catch {}
    state.serial.writer.releaseLock();
    state.serial.writer = null;
  }

  if (state.serial.readableClosed) {
    try {
      await state.serial.readableClosed;
    } catch {}
    state.serial.readableClosed = null;
  }

  if (state.serial.writableClosed) {
    try {
      await state.serial.writableClosed;
    } catch {}
    state.serial.writableClosed = null;
  }

  if (state.serial.port) {
    try {
      await state.serial.port.close();
    } catch {}
  }

  state.serial.port = null;
  state.serial.connectedLabel = "";
  renderConnectionSummary();
}

async function readLoop() {
  while (state.serial.port && state.serial.keepReading && state.serial.reader) {
    const { value, done } = await state.serial.reader.read();
    if (done) break;
    if (!value) continue;
    handleIncomingLine(value);
  }
}

function handleIncomingLine(line) {
  log(`< ${line}`);
  try {
    const payload = JSON.parse(line);
    applyIncomingPayload(payload);
  } catch {
    log("Non-JSON payload received");
  }
}

function applyIncomingPayload(payload) {
  if (payload.node_id || payload.identity) {
    state.nodeId = payload.node_id || payload.identity;
  }

  if (payload.status) {
    state.nodeStatus = payload.status;
  }

  if (payload.role) {
    els["role-select"].value = payload.role;
  }

  if (payload.core_state) {
    els["state-select"].value = payload.core_state;
  }

  if (payload.relays && Array.isArray(payload.relays)) {
    const scope = getSelectedScope();
    if (scope) {
      scope.relayState = [Boolean(payload.relays[0]), Boolean(payload.relays[1])];
      persistScopes();
    }
  }

  if (payload.sensors || payload.uptime_ms || payload.role || payload.core_state) {
    state.telemetry = payload;
  }

  render();
}

async function sendPayload(payload) {
  if (!state.serial.writer) {
    window.alert("Connect to a serial device first.");
    return;
  }

  const serialized = `${JSON.stringify(payload)}\n`;
  await state.serial.writer.write(serialized);
  log(`> ${serialized.trim()}`);
}

async function sendRawJson() {
  try {
    const payload = JSON.parse(els["raw-json"].value);
    await sendPayload(payload);
  } catch (error) {
    window.alert(`Invalid JSON: ${error.message}`);
  }
}

function toggleRelay(index) {
  const scope = getSelectedScope();
  if (!scope) return;
  const nextValue = !Boolean(scope.relayState?.[index]);
  scope.relayState = scope.relayState || [false, false];
  scope.relayState[index] = nextValue;
  persistScopes();
  render();
  sendPayload({
    action: "SET_RELAY",
    target: index === 0 ? "RELAY_01" : "RELAY_02",
    value: nextValue ? 1 : 0,
  });
}

function toggleAutoPoll() {
  if (state.serial.autoPoll) {
    stopAutoPoll();
    return;
  }

  state.serial.autoPoll = true;
  els["toggle-auto-poll"].textContent = "Stop Auto Poll";
  state.serial.pollHandle = window.setInterval(() => {
    sendPayload({ query: "GET_TELEMETRY" });
  }, POLL_MS);
  log("Started telemetry auto polling");
}

function stopAutoPoll() {
  state.serial.autoPoll = false;
  if (state.serial.pollHandle) {
    clearInterval(state.serial.pollHandle);
    state.serial.pollHandle = null;
  }
  els["toggle-auto-poll"].textContent = "Start Auto Poll";
}

function formatPortInfo(info, index) {
  const vendor = info.usbVendorId ? `VID ${info.usbVendorId}` : "VID ?";
  const product = info.usbProductId ? `PID ${info.usbProductId}` : "PID ?";
  return `Port ${index + 1} • ${vendor} • ${product}`;
}

function guessWindowsPortLabel(info) {
  if (info.usbVendorId && info.usbProductId) {
    return `COM?  // VID:${info.usbVendorId} PID:${info.usbProductId}`;
  }
  return "";
}

async function copyText(value) {
  try {
    await navigator.clipboard.writeText(value);
    log(`Copied to clipboard: ${value}`);
  } catch (error) {
    window.alert(`Clipboard failed: ${error.message}`);
  }
}

function log(message) {
  const now = new Date().toLocaleTimeString();
  els["log-output"].textContent += `[${now}] ${message}\n`;
  els["log-output"].scrollTop = els["log-output"].scrollHeight;
}

async function fetchRemovableDrives() {
  const response = await fetch("/api/drives");
  if (!response.ok) {
    throw new Error(`Drive lookup failed with status ${response.status}`);
  }
  return response.json();
}

async function prepareFlash(imagePath, driveMountPoint) {
  const response = await fetch("/flash", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      image_path: imagePath,
      drive_mount_point: driveMountPoint,
      dry_run: true,
    }),
  });

  if (!response.ok) {
    const message = await response.text();
    throw new Error(message || `Flash preparation failed with status ${response.status}`);
  }

  return response.json();
}

window.apexBackend = {
  fetchRemovableDrives,
  prepareFlash,
};

class LineBreakTransformer {
  constructor() {
    this.container = "";
  }

  transform(chunk, controller) {
    this.container += chunk;
    const lines = this.container.split("\n");
    this.container = lines.pop();
    lines.forEach((line) => controller.enqueue(line.trim()));
  }

  flush(controller) {
    if (this.container) {
      controller.enqueue(this.container.trim());
    }
  }
}
