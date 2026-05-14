/**
 * Disarray Desnarler — companion app frontend
 *
 * Communicates with the FastAPI backend via:
 *   REST  GET  /api/config           → load all key configs
 *   REST  PUT  /api/keys/:key        → save a key config
 *   WS    /ws                        → receive keypress / config_updated events
 */

const KEY_NAMES     = ["F13", "F14", "F15", "F16"];
const KEY_POSITIONS = ["Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right"];

const ACTION_META = {
  shell: {
    label: "Shell Command",
    placeholder: "e.g. open -a Spotify",
    examples: [
      "open -a Spotify",
      "open -a 'Visual Studio Code'",
      "osascript -e 'set volume output muted not (output muted of (get volume settings))'",
      "screencapture -i ~/Desktop/screenshot.png",
    ],
  },
  app: {
    label: "App Name",
    placeholder: "e.g. Finder",
    examples: ["Spotify", "Safari", "Visual Studio Code", "Terminal", "Finder"],
  },
  url: {
    label: "URL",
    placeholder: "e.g. https://github.com",
    examples: ["https://github.com", "https://linear.app", "https://notion.so"],
  },
};

// ─── State ───────────────────────────────────────────────────────────────────

let configData = {};         // { F13: { label, action }, … }
let editingKey  = null;      // key currently open in modal
let ws          = null;

// ─── DOM refs ────────────────────────────────────────────────────────────────

const $statusPill  = document.getElementById("status-pill");
const $statusText  = document.getElementById("status-text");
const $keyCards    = document.getElementById("key-cards");
const $activityLog = document.getElementById("activity-log");
const $overlay     = document.getElementById("modal-overlay");
const $modalTitle  = document.getElementById("modal-title");
const $editLabel   = document.getElementById("edit-label");
const $editType    = document.getElementById("edit-type");
const $editValue   = document.getElementById("edit-value");
const $editValueLbl= document.getElementById("edit-value-label");
const $exampleList = document.getElementById("examples-list");

// ─── Init ─────────────────────────────────────────────────────────────────────

async function init() {
  await loadConfig();
  renderKeyCards();
  connectWebSocket();
  bindModalEvents();
  bindKeyClicksOnDevice();
}

// ─── Config ──────────────────────────────────────────────────────────────────

async function loadConfig() {
  try {
    const res = await fetch("/api/config");
    configData = await res.json();
  } catch {
    setStatus("error", "Server unreachable");
  }
}

async function saveKey(keyName, label, action) {
  await fetch(`/api/keys/${keyName}`, {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ label, action }),
  });
  // Optimistic local update (WS broadcast will also trigger full refresh)
  if (!configData[keyName]) configData[keyName] = {};
  configData[keyName].label  = label;
  configData[keyName].action = action;
}

// ─── Key cards (config panel) ─────────────────────────────────────────────────

function renderKeyCards() {
  $keyCards.innerHTML = "";
  KEY_NAMES.forEach((key, i) => {
    const data = configData[key] || {};
    const card = document.createElement("div");
    card.className = "key-card";
    card.id = `card-${key}`;
    card.innerHTML = `
      <div class="key-card-badge">${key}</div>
      <div class="key-card-info">
        <div class="key-card-name">${esc(data.label || key)}</div>
        <div class="key-card-action ${actionSummary(data.action) === "No action set" ? "empty" : ""}">
          ${esc(actionSummary(data.action))}
        </div>
      </div>
      <button class="key-card-edit-btn" data-key="${key}">Edit</button>
    `;
    card.querySelector(".key-card-edit-btn").addEventListener("click", (e) => {
      e.stopPropagation();
      openModal(key);
    });
    card.addEventListener("click", () => openModal(key));
    $keyCards.appendChild(card);

    // Sync label on the device visual
    const klabel = document.getElementById(`klabel-${key}`);
    if (klabel) klabel.textContent = data.label || key;
  });
}

function refreshCard(key) {
  const data = configData[key] || {};
  const card = document.getElementById(`card-${key}`);
  if (!card) return;
  card.querySelector(".key-card-name").textContent  = data.label || key;
  const actionEl = card.querySelector(".key-card-action");
  const summary  = actionSummary(data.action);
  actionEl.textContent = summary;
  actionEl.className   = "key-card-action" + (summary === "No action set" ? " empty" : "");
  const klabel = document.getElementById(`klabel-${key}`);
  if (klabel) klabel.textContent = data.label || key;
}

function actionSummary(action) {
  if (!action) return "No action set";
  const val = action.command || action.app || action.url || "";
  if (!val) return "No action set";
  const prefix = { shell: "$", app: "open", url: "" }[action.type] || "";
  const short  = val.length > 40 ? val.slice(0, 38) + "…" : val;
  return prefix ? `${prefix} ${short}` : short;
}

// ─── Device visual ────────────────────────────────────────────────────────────

function bindKeyClicksOnDevice() {
  KEY_NAMES.forEach(key => {
    const btn = document.getElementById(`key-${key}`);
    if (btn) btn.addEventListener("click", () => openModal(key));
  });
}

function flashKey(keyName) {
  const btn = document.getElementById(`key-${keyName}`);
  if (!btn) return;
  btn.classList.add("pressed");
  setTimeout(() => btn.classList.remove("pressed"), 220);

  // Pulse the matching card
  const card = document.getElementById(`card-${keyName}`);
  if (card) {
    card.classList.add("active");
    setTimeout(() => card.classList.remove("active"), 600);
  }
}

// ─── Activity log ─────────────────────────────────────────────────────────────

const MAX_LOG_EVENTS = 8;

function logEvent(keyName) {
  const empty = $activityLog.querySelector(".activity-empty");
  if (empty) empty.remove();

  const data    = configData[keyName] || {};
  const label   = data.label || keyName;
  const chip    = document.createElement("div");
  chip.className = "activity-event";
  chip.innerHTML = `<span class="ev-key">${esc(keyName)}</span><span class="ev-label">${esc(label)}</span>`;
  $activityLog.prepend(chip);

  const events = $activityLog.querySelectorAll(".activity-event");
  if (events.length > MAX_LOG_EVENTS) events[events.length - 1].remove();
}

// ─── WebSocket ────────────────────────────────────────────────────────────────

function connectWebSocket() {
  ws = new WebSocket(`ws://${location.host}/ws`);

  ws.addEventListener("open", () => {
    setStatus("connected", "Listening");
    // Keep-alive ping every 20 s
    setInterval(() => ws.readyState === WebSocket.OPEN && ws.send("ping"), 20000);
  });

  ws.addEventListener("message", (e) => {
    const msg = JSON.parse(e.data);
    if (msg.type === "keypress") {
      flashKey(msg.key);
      logEvent(msg.key);
    } else if (msg.type === "config_updated") {
      // Another browser tab saved a config — reload and refresh
      loadConfig().then(() => refreshCard(msg.key));
    }
  });

  ws.addEventListener("close", () => {
    setStatus("error", "Disconnected");
    setTimeout(connectWebSocket, 3000);  // auto-reconnect
  });

  ws.addEventListener("error", () => setStatus("error", "Connection error"));
}

// ─── Status pill ──────────────────────────────────────────────────────────────

function setStatus(state, text) {
  $statusPill.className = `status-pill ${state}`;
  $statusText.textContent = text;
}

// ─── Modal ────────────────────────────────────────────────────────────────────

function openModal(keyName) {
  editingKey = keyName;
  const data   = configData[keyName] || {};
  const action = data.action || { type: "shell", command: "" };

  $modalTitle.textContent = `Edit ${keyName}`;
  $editLabel.value        = data.label || "";
  $editType.value         = action.type || "shell";
  $editValue.value        = action.command || action.app || action.url || "";

  updateModalMeta(action.type || "shell");
  $overlay.classList.remove("hidden");
  $editLabel.focus();
}

function closeModal() {
  $overlay.classList.add("hidden");
  editingKey = null;
}

function bindModalEvents() {
  document.getElementById("modal-close").addEventListener("click", closeModal);
  document.getElementById("modal-cancel").addEventListener("click", closeModal);
  $overlay.addEventListener("click", (e) => { if (e.target === $overlay) closeModal(); });

  $editType.addEventListener("change", () => updateModalMeta($editType.value));

  document.getElementById("modal-save").addEventListener("click", async () => {
    if (!editingKey) return;
    const label  = $editLabel.value.trim() || editingKey;
    const type   = $editType.value;
    const value  = $editValue.value.trim();
    const field  = { shell: "command", app: "app", url: "url" }[type];
    const action = { type, [field]: value };

    await saveKey(editingKey, label, action);
    refreshCard(editingKey);
    closeModal();
  });

  // Close on Escape
  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && !$overlay.classList.contains("hidden")) closeModal();
  });
}

function updateModalMeta(type) {
  const meta = ACTION_META[type] || ACTION_META.shell;
  $editValueLbl.textContent    = meta.label;
  $editValue.placeholder       = meta.placeholder;

  $exampleList.innerHTML = "";
  meta.examples.forEach(ex => {
    const chip = document.createElement("span");
    chip.className   = "example-chip";
    chip.textContent = ex;
    chip.title       = "Click to use this example";
    chip.addEventListener("click", () => { $editValue.value = ex; $editValue.focus(); });
    $exampleList.appendChild(chip);
  });
}

// ─── Util ─────────────────────────────────────────────────────────────────────

function esc(str) {
  return String(str)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

// ─── Go ──────────────────────────────────────────────────────────────────────

init();
