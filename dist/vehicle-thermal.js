const CAN_BMS_THERMAL_STATUS = 0x18FF50E5;
const CAN_THERMAL_COMMAND = 0x18FF51E5;
const CAN_TIMEOUT_MS = 300;

const OUTPUT_PUMP = 1 << 0;
const OUTPUT_FAN = 1 << 1;
const OUTPUT_HEATER = 1 << 2;
const OUTPUT_CHARGE_ENABLE = 1 << 3;
const OUTPUT_FAULT = 1 << 7;

const DTC_TIMEOUT = 1 << 0;
const DTC_DERATE = 1 << 1;
const DTC_CRITICAL = 1 << 2;

const scenarioPresets = {
  nominal: { label: "nominal drive", temperature: 230, voltage: 4000, mode: "drive", online: true },
  cooling: { label: "thermal cooling", temperature: 520, voltage: 3990, mode: "drive", online: true },
  derate: { label: "power derate", temperature: 610, voltage: 3980, mode: "drive", online: true },
  critical: { label: "critical temperature", temperature: 660, voltage: 3970, mode: "drive", online: true },
  "cold-charge": { label: "cold charge request", temperature: 30, voltage: 4010, mode: "charge", online: true }
};

const faultCampaign = [
  scenarioPresets.nominal,
  scenarioPresets.cooling,
  scenarioPresets.derate,
  scenarioPresets.critical,
  { label: "BMS recovered", temperature: 240, voltage: 4000, mode: "drive", online: true },
  scenarioPresets["cold-charge"],
  { label: "charge ready", temperature: 110, voltage: 4020, mode: "charge", online: true },
  { label: "BMS deadline exceeded", timeout: true }
];

const state = {
  mode: "NORMAL",
  outputs: 0,
  torqueLimit: 100,
  chargeLimit: 100,
  dtc: 0,
  timeMs: 0,
  taskRuns: 0,
  watchdogKicks: 0,
  txCount: 0,
  lastBms: null,
  rxFrame: [0xE6, 0x00, 0xA0, 0x0F, 0x00, 0x00, 0x00, 0x00],
  txFrame: [0x00, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00],
  events: [],
  campaignTimer: null
};

const elements = {
  temperature: document.querySelector("#pack-temperature"),
  voltage: document.querySelector("#pack-voltage"),
  mode: document.querySelector("#vehicle-mode"),
  online: document.querySelector("#bms-online"),
  temperatureValue: document.querySelector("#pack-temperature-value"),
  voltageValue: document.querySelector("#pack-voltage-value"),
  ecuState: document.querySelector("#ecu-state"),
  torqueLimit: document.querySelector("#torque-limit"),
  chargeLimit: document.querySelector("#charge-limit"),
  freshness: document.querySelector("#bms-freshness"),
  taskRuns: document.querySelector("#task-runs"),
  rxData: document.querySelector("#rx-data"),
  txData: document.querySelector("#tx-data"),
  decodedTemp: document.querySelector("#decoded-temp"),
  decodedVoltage: document.querySelector("#decoded-voltage"),
  decodedMode: document.querySelector("#decoded-mode"),
  outputRegister: document.querySelector("#output-register"),
  dtcCode: document.querySelector("#dtc-code"),
  frameCounter: document.querySelector("#frame-counter"),
  bitRegister: document.querySelector("#vehicle-bit-register"),
  log: document.querySelector("#vehicle-log"),
  watchdog: document.querySelector("#watchdog-kicks"),
  inject: document.querySelector("#inject-frame"),
  runTask: document.querySelector("#run-task"),
  campaign: document.querySelector("#run-campaign"),
  reset: document.querySelector("#reset-ecu")
};

function formatTenths(value, unit) {
  return `${(value / 10).toFixed(1)} ${unit}`;
}

function hexByte(value) {
  return value.toString(16).padStart(2, "0").toUpperCase();
}

function hexId(value) {
  return `0x${value.toString(16).padStart(8, "0").toUpperCase()}`;
}

function active(mask) {
  return (state.outputs & mask) !== 0;
}

function stateClass() {
  if (state.mode === "BMS_TIMEOUT") return "timeout";
  if (state.mode === "SAFE_SHUTDOWN") return "shutdown";
  if (state.mode === "POWER_DERATE") return "derate";
  if (state.mode === "PRECONDITIONING") return "preconditioning";
  if (state.mode === "COOLING") return "cooling";
  return "normal";
}

function dtcName() {
  if (state.dtc & DTC_TIMEOUT) return "BMS_CAN_TIMEOUT";
  if (state.dtc & DTC_CRITICAL) return "CRITICAL_PACK_TEMP";
  if (state.dtc & DTC_DERATE) return "POWER_DERATE";
  return "NONE";
}

function bmsAge() {
  return state.lastBms === null ? null : state.timeMs - state.lastBms.timestampMs;
}

function bmsFresh() {
  const age = bmsAge();
  return age !== null && age < CAN_TIMEOUT_MS;
}

function updateInputLabels() {
  elements.temperatureValue.value = formatTenths(Number(elements.temperature.value), "C");
  elements.voltageValue.value = formatTenths(Number(elements.voltage.value), "V");
}

function pushEvent(label, detail) {
  state.events.unshift({
    timestamp: `${String(state.timeMs).padStart(4, "0")} ms`,
    label,
    detail
  });
  state.events = state.events.slice(0, 12);
}

function buildBmsFrame(temperature, voltage, mode) {
  const encodedTemperature = temperature & 0xFFFF;
  return [
    encodedTemperature & 0xFF,
    (encodedTemperature >> 8) & 0xFF,
    voltage & 0xFF,
    (voltage >> 8) & 0xFF,
    mode === "charge" ? 1 : 0,
    0,
    0,
    0
  ];
}

function injectBmsFrame(label = "BMS status injected") {
  if (!elements.online.checked) {
    pushEvent("RX blocked", "BMS CAN node unavailable");
    render();
    return false;
  }

  const temperature = Number(elements.temperature.value);
  const voltage = Number(elements.voltage.value);
  const mode = elements.mode.value;
  state.lastBms = { temperature, voltage, mode, timestampMs: state.timeMs };
  state.rxFrame = buildBmsFrame(temperature, voltage, mode);
  pushEvent(`RX ${hexId(CAN_BMS_THERMAL_STATUS)}`, `${label} | ${formatTenths(temperature, "C")}`);
  render();
  return true;
}

function runEcuTask(label = "100 ms control task") {
  state.timeMs += 100;
  state.taskRuns += 1;
  state.watchdogKicks += 1;

  if (!bmsFresh()) {
    state.mode = "BMS_TIMEOUT";
    state.outputs = OUTPUT_PUMP | OUTPUT_FAN | OUTPUT_FAULT;
    state.torqueLimit = 0;
    state.chargeLimit = 0;
    state.dtc = DTC_TIMEOUT;
  } else {
    const { temperature, mode } = state.lastBms;
    const wasPreconditioning = state.mode === "PRECONDITIONING";
    const wasCooling = state.mode === "COOLING" || state.mode === "POWER_DERATE";
    const needsPreconditioning = mode === "charge" &&
      (temperature <= 50 || (wasPreconditioning && temperature < 100));
    const needsDerate = temperature >= 600 || (state.mode === "POWER_DERATE" && temperature >= 570);
    const needsCooling = temperature >= 500 || (wasCooling && temperature >= 450);

    state.outputs = 0;
    state.torqueLimit = 100;
    state.chargeLimit = 100;
    state.dtc = 0;

    if (temperature >= 650) {
      state.mode = "SAFE_SHUTDOWN";
      state.outputs = OUTPUT_PUMP | OUTPUT_FAN | OUTPUT_FAULT;
      state.torqueLimit = 0;
      state.chargeLimit = 0;
      state.dtc = DTC_CRITICAL;
    } else if (needsPreconditioning) {
      state.mode = "PRECONDITIONING";
      state.outputs = OUTPUT_PUMP | OUTPUT_HEATER;
      state.chargeLimit = 0;
    } else if (needsDerate) {
      state.mode = "POWER_DERATE";
      state.outputs = OUTPUT_PUMP | OUTPUT_FAN;
      state.torqueLimit = 50;
      state.chargeLimit = 50;
      state.dtc = DTC_DERATE;
    } else if (needsCooling) {
      state.mode = "COOLING";
      state.outputs = OUTPUT_PUMP | OUTPUT_FAN;
    } else {
      state.mode = "NORMAL";
      if (mode === "charge") state.outputs = OUTPUT_CHARGE_ENABLE;
    }
  }

  state.txFrame = [
    state.outputs,
    state.torqueLimit,
    state.chargeLimit,
    ["NORMAL", "PRECONDITIONING", "COOLING", "POWER_DERATE", "SAFE_SHUTDOWN", "BMS_TIMEOUT"].indexOf(state.mode),
    state.dtc,
    state.txCount & 0xFF,
    0,
    0
  ];
  state.txCount += 1;
  pushEvent(`TX ${hexId(CAN_THERMAL_COMMAND)}`, `${label} | ${state.mode} | DTC ${dtcName()}`);
  render();
}

function renderRegister() {
  elements.bitRegister.replaceChildren();
  for (let bit = 7; bit >= 0; bit -= 1) {
    const cell = document.createElement("div");
    const isActive = (state.outputs & (1 << bit)) !== 0;
    cell.className = `bit-cell${isActive ? " active" : ""}${bit === 7 ? " alarm" : ""}`;
    const label = document.createElement("small");
    const value = document.createElement("strong");
    label.textContent = `B${bit}`;
    value.textContent = isActive ? "1" : "0";
    cell.append(label, value);
    elements.bitRegister.append(cell);
  }
}

function renderOutputs() {
  const map = {
    pump: OUTPUT_PUMP,
    fan: OUTPUT_FAN,
    heater: OUTPUT_HEATER,
    charge: OUTPUT_CHARGE_ENABLE,
    fault: OUTPUT_FAULT
  };

  Object.entries(map).forEach(([name, mask]) => {
    const row = document.querySelector(`[data-output="${name}"]`);
    const isActive = active(mask);
    row.classList.toggle("active", isActive);
    row.querySelector(".output-state").textContent = isActive ? "ON" : "OFF";
  });
}

function renderLog() {
  elements.log.replaceChildren();
  state.events.forEach((event) => {
    const item = document.createElement("li");
    const time = document.createElement("time");
    const title = document.createElement("strong");
    const detail = document.createElement("span");
    time.textContent = `[${event.timestamp}]`;
    title.textContent = event.label;
    detail.textContent = event.detail;
    item.append(time, title, detail);
    elements.log.append(item);
  });
}

function render() {
  const age = bmsAge();
  const bms = state.lastBms;
  elements.ecuState.textContent = state.mode;
  elements.ecuState.className = `state-badge ${stateClass()}`;
  elements.torqueLimit.textContent = `${state.torqueLimit}%`;
  elements.chargeLimit.textContent = `${state.chargeLimit}%`;
  elements.freshness.textContent = age === null ? "No frame" : `${age} ms${bmsFresh() ? "" : " stale"}`;
  elements.taskRuns.textContent = String(state.taskRuns);
  elements.rxData.textContent = state.rxFrame.map(hexByte).join(" ");
  elements.txData.textContent = state.txFrame.map(hexByte).join(" ");
  elements.decodedTemp.textContent = bms === null ? "--.- C" : formatTenths(bms.temperature, "C");
  elements.decodedVoltage.textContent = bms === null ? "---.- V" : formatTenths(bms.voltage, "V");
  elements.decodedMode.textContent = bms === null ? "--" : bms.mode.toUpperCase();
  elements.outputRegister.textContent = `0x${hexByte(state.outputs)}`;
  elements.dtcCode.textContent = dtcName();
  elements.frameCounter.textContent = String((state.txCount - 1 + 256) % 256);
  elements.watchdog.textContent = String(state.watchdogKicks);
  renderRegister();
  renderOutputs();
  renderLog();
}

function loadScenario(scenario, execute = true) {
  elements.temperature.value = String(scenario.temperature);
  elements.voltage.value = String(scenario.voltage);
  elements.mode.value = scenario.mode;
  elements.online.checked = scenario.online;
  updateInputLabels();
  if (execute) {
    injectBmsFrame(scenario.label);
    runEcuTask(scenario.label);
  }
}

function runTimeoutScenario() {
  stopCampaign();
  state.timeMs += 200;
  runEcuTask("BMS deadline exceeded");
}

function stopCampaign() {
  if (state.campaignTimer !== null) {
    window.clearInterval(state.campaignTimer);
    state.campaignTimer = null;
  }
  elements.campaign.textContent = "Run fault campaign";
}

function runCampaign() {
  stopCampaign();
  let index = 0;
  elements.campaign.textContent = "Running campaign";
  const advance = () => {
    const step = faultCampaign[index];
    if (step.timeout) {
      state.timeMs += 200;
      runEcuTask(step.label);
    } else {
      loadScenario(step);
    }
    index += 1;
    if (index >= faultCampaign.length) stopCampaign();
  };
  advance();
  state.campaignTimer = window.setInterval(advance, 850);
}

function resetEcu() {
  stopCampaign();
  state.mode = "NORMAL";
  state.outputs = 0;
  state.torqueLimit = 100;
  state.chargeLimit = 100;
  state.dtc = 0;
  state.timeMs = 0;
  state.taskRuns = 0;
  state.watchdogKicks = 0;
  state.txCount = 0;
  state.lastBms = null;
  state.events = [];
  loadScenario(scenarioPresets.nominal, false);
  injectBmsFrame("startup BMS status");
  runEcuTask("initial control task");
}

elements.temperature.addEventListener("input", updateInputLabels);
elements.voltage.addEventListener("input", updateInputLabels);
elements.inject.addEventListener("click", () => {
  stopCampaign();
  injectBmsFrame();
});
elements.runTask.addEventListener("click", () => {
  stopCampaign();
  runEcuTask();
});
elements.campaign.addEventListener("click", runCampaign);
elements.reset.addEventListener("click", resetEcu);
document.querySelectorAll("[data-scenario]").forEach((button) => {
  button.addEventListener("click", () => {
    stopCampaign();
    if (button.dataset.scenario === "timeout") {
      runTimeoutScenario();
    } else {
      loadScenario(scenarioPresets[button.dataset.scenario]);
    }
  });
});

resetEcu();
