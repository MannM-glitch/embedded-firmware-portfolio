const OUTPUT_HEATER = 1 << 0;
const OUTPUT_FAN = 1 << 1;
const OUTPUT_DEHUMIDIFIER = 1 << 2;
const OUTPUT_ALARM = 1 << 7;

const HEATER_ON = 180;
const HEATER_OFF = 200;
const FAN_ON = 260;
const FAN_OFF = 240;
const DEHUMIDIFIER_ON = 650;
const DEHUMIDIFIER_OFF = 600;

const scenarioPresets = {
  cold: { label: "cold room", temperature: 165, humidity: 450, online: true },
  humid: { label: "high humidity", temperature: 225, humidity: 710, online: true },
  hot: { label: "over-temperature", temperature: 275, humidity: 580, online: true },
  fault: { label: "sensor transaction failed", temperature: 0, humidity: 0, online: false }
};

const demoScenario = [
  { label: "startup", temperature: 220, humidity: 450, online: true },
  scenarioPresets.cold,
  { label: "warming up", temperature: 195, humidity: 450, online: true },
  { label: "comfortable", temperature: 205, humidity: 450, online: true },
  scenarioPresets.humid,
  { label: "humidity recovered", temperature: 225, humidity: 580, online: true },
  scenarioPresets.hot,
  { label: "cooling complete", temperature: 235, humidity: 580, online: true },
  scenarioPresets.fault,
  { label: "sensor recovered", temperature: 225, humidity: 500, online: true }
];

const state = {
  mode: "NORMAL",
  outputs: 0,
  sensorOk: true,
  temperature: 220,
  humidity: 450,
  timeMs: 0,
  cycles: 0,
  watchdogKicks: 0,
  events: [],
  demoTimer: null
};

const elements = {
  temperature: document.querySelector("#temperature"),
  humidity: document.querySelector("#humidity"),
  sensorOnline: document.querySelector("#sensor-online"),
  temperatureInput: document.querySelector("#temperature-input-value"),
  humidityInput: document.querySelector("#humidity-input-value"),
  stateBadge: document.querySelector("#state-badge"),
  temperatureReading: document.querySelector("#temperature-reading"),
  humidityReading: document.querySelector("#humidity-reading"),
  sensorReading: document.querySelector("#sensor-reading"),
  cycleReading: document.querySelector("#cycle-reading"),
  registerValue: document.querySelector("#register-value"),
  binaryValue: document.querySelector("#binary-value"),
  bitRegister: document.querySelector("#bit-register"),
  eventLog: document.querySelector("#event-log"),
  watchdogReading: document.querySelector("#watchdog-reading"),
  runCycle: document.querySelector("#run-cycle"),
  runDemo: document.querySelector("#run-demo"),
  reset: document.querySelector("#reset")
};

function formatTenths(value, unit) {
  return `${(value / 10).toFixed(1)} ${unit}`;
}

function outputActive(mask) {
  return (state.outputs & mask) !== 0;
}

function updateInputLabels() {
  elements.temperatureInput.value = formatTenths(Number(elements.temperature.value), "C");
  elements.humidityInput.value = formatTenths(Number(elements.humidity.value), "%");
}

function addEvent(label) {
  const busState = state.sensorOk ? "I2C OK" : "I2C FAIL";
  const register = `0x${state.outputs.toString(16).padStart(2, "0").toUpperCase()}`;
  state.events.unshift({
    timestamp: `${String(state.timeMs / 1000).padStart(3, "0")} s`,
    label,
    detail: `${busState} | ${state.mode} | ${register}`
  });
  state.events = state.events.slice(0, 10);
}

function runControlCycle(label = "manual sample") {
  const temperature = Number(elements.temperature.value);
  const humidity = Number(elements.humidity.value);
  const sensorOnline = elements.sensorOnline.checked;

  state.cycles += 1;
  state.watchdogKicks += 1;
  state.timeMs += 1000;

  if (!sensorOnline) {
    state.mode = "SENSOR_FAULT";
    state.outputs = OUTPUT_ALARM;
    state.sensorOk = false;
    addEvent(label);
    render();
    return;
  }

  state.sensorOk = true;
  state.temperature = temperature;
  state.humidity = humidity;

  if (state.mode === "SENSOR_FAULT") {
    state.mode = "NORMAL";
  }

  let outputs = state.outputs & OUTPUT_DEHUMIDIFIER;

  if (state.mode === "NORMAL") {
    if (temperature <= HEATER_ON) {
      state.mode = "HEATING";
    } else if (temperature >= FAN_ON) {
      state.mode = "COOLING";
    }
  } else if (state.mode === "HEATING" && temperature >= HEATER_OFF) {
    state.mode = "NORMAL";
  } else if (state.mode === "COOLING" && temperature <= FAN_OFF) {
    state.mode = "NORMAL";
  }

  if (state.mode === "HEATING") {
    outputs |= OUTPUT_HEATER;
  } else if (state.mode === "COOLING") {
    outputs |= OUTPUT_FAN;
  }

  if (humidity >= DEHUMIDIFIER_ON) {
    outputs |= OUTPUT_DEHUMIDIFIER;
  } else if (humidity <= DEHUMIDIFIER_OFF) {
    outputs &= ~OUTPUT_DEHUMIDIFIER;
  }

  state.outputs = outputs;
  addEvent(label);
  render();
}

function renderRegister() {
  elements.bitRegister.replaceChildren();
  for (let bit = 7; bit >= 0; bit -= 1) {
    const cell = document.createElement("div");
    const active = (state.outputs & (1 << bit)) !== 0;
    cell.className = `bit-cell${active ? " active" : ""}${bit === 7 ? " alarm" : ""}`;
    cell.innerHTML = `<small>B${bit}</small><strong>${active ? "1" : "0"}</strong>`;
    elements.bitRegister.append(cell);
  }
  elements.registerValue.textContent = `0x${state.outputs.toString(16).padStart(2, "0").toUpperCase()}`;
  elements.binaryValue.textContent = `0b${state.outputs.toString(2).padStart(8, "0")}`;
}

function renderOutputs() {
  const outputMap = {
    heater: OUTPUT_HEATER,
    fan: OUTPUT_FAN,
    dehumidifier: OUTPUT_DEHUMIDIFIER,
    alarm: OUTPUT_ALARM
  };

  Object.entries(outputMap).forEach(([name, mask]) => {
    const row = document.querySelector(`[data-output="${name}"]`);
    const active = outputActive(mask);
    row.classList.toggle("active", active);
    row.querySelector(".output-state").textContent = active ? "ON" : "OFF";
  });
}

function renderLog() {
  elements.eventLog.replaceChildren();
  state.events.forEach((event) => {
    const item = document.createElement("li");
    const time = document.createElement("time");
    const label = document.createElement("strong");
    const detail = document.createElement("span");
    time.textContent = `[${event.timestamp}]`;
    label.textContent = event.label;
    detail.textContent = event.detail;
    item.append(time, label, detail);
    elements.eventLog.append(item);
  });
}

function render() {
  const stateClass = state.mode === "SENSOR_FAULT" ? "fault" : state.mode.toLowerCase();
  elements.stateBadge.textContent = state.mode;
  elements.stateBadge.className = `state-badge ${stateClass}`;
  elements.temperatureReading.textContent = state.sensorOk ? formatTenths(state.temperature, "C") : "--.- C";
  elements.humidityReading.textContent = state.sensorOk ? formatTenths(state.humidity, "%") : "--.- %";
  elements.sensorReading.textContent = state.sensorOk ? "I2C OK" : "I2C FAIL";
  elements.cycleReading.textContent = String(state.cycles);
  elements.watchdogReading.textContent = String(state.watchdogKicks);
  renderRegister();
  renderOutputs();
  renderLog();
}

function loadScenario(scenario, runNow = true) {
  elements.temperature.value = String(scenario.temperature);
  elements.humidity.value = String(scenario.humidity);
  elements.sensorOnline.checked = scenario.online;
  updateInputLabels();
  if (runNow) {
    runControlCycle(scenario.label);
  }
}

function stopDemo() {
  if (state.demoTimer !== null) {
    window.clearInterval(state.demoTimer);
    state.demoTimer = null;
  }
  elements.runDemo.textContent = "Run demo scenario";
}

function runDemo() {
  stopDemo();
  let index = 0;
  elements.runDemo.textContent = "Running demo";
  loadScenario(demoScenario[index]);
  index += 1;
  state.demoTimer = window.setInterval(() => {
    if (index >= demoScenario.length) {
      stopDemo();
      return;
    }
    loadScenario(demoScenario[index]);
    index += 1;
  }, 900);
}

function resetController() {
  stopDemo();
  state.mode = "NORMAL";
  state.outputs = 0;
  state.sensorOk = true;
  state.temperature = 220;
  state.humidity = 450;
  state.timeMs = 0;
  state.cycles = 0;
  state.watchdogKicks = 0;
  state.events = [];
  loadScenario(demoScenario[0], false);
  addEvent("controller reset");
  render();
}

elements.temperature.addEventListener("input", updateInputLabels);
elements.humidity.addEventListener("input", updateInputLabels);
elements.runCycle.addEventListener("click", () => {
  stopDemo();
  runControlCycle();
});
elements.runDemo.addEventListener("click", runDemo);
elements.reset.addEventListener("click", resetController);
document.querySelectorAll("[data-scenario]").forEach((button) => {
  button.addEventListener("click", () => {
    stopDemo();
    loadScenario(scenarioPresets[button.dataset.scenario]);
  });
});

resetController();
