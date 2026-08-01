"use strict";

const ui = {
  connect: document.querySelector("#connectButton"),
  calibrate: document.querySelector("#calibrateButton"),
  zero: document.querySelector("#zeroButton"),
  connectionState: document.querySelector("#connectionState"),
  support: document.querySelector("#supportMessage"),
  roll: document.querySelector("#rollValue"),
  pitch: document.querySelector("#pitchValue"),
  heading: document.querySelector("#headingValue"),
  attitudePlane: document.querySelector("#attitudePlane"),
  compassRose: document.querySelector("#compassRose"),
  gyroWeight: document.querySelector("#gyroWeight"),
  gyroWeightOutput: document.querySelector("#gyroWeightOutput"),
  gyroScale: document.querySelector("#gyroScale"),
  biasValue: document.querySelector("#biasValue"),
  sampleInterval: document.querySelector("#sampleInterval"),
  sampleState: document.querySelector("#sampleState"),
  gyroValues: ["X", "Y", "Z"].map((axis) => document.querySelector(`#gyro${axis}`)),
  accelValues: ["X", "Y", "Z"].map((axis) => document.querySelector(`#accel${axis}`)),
};

const encoder = new TextEncoder();
const filter = {
  roll: 0,
  pitch: 0,
  heading: 0,
  initialized: false,
  lastSampleTime: 0,
  gyroBias: [0, 0, 0],
  calibration: null,
};

let port;
let reader;
let writer;
let readTask;
let receiveBuffer = "";
let pollTimer;

function wrap180(angle) {
  return ((angle + 180) % 360 + 360) % 360 - 180;
}

function wrap360(angle) {
  return ((angle % 360) + 360) % 360;
}

function blendAngle(predicted, measured, gyroWeightValue) {
  return predicted + (1 - gyroWeightValue) * wrap180(measured - predicted);
}

function accelerometerAngles(x, y, z) {
  const magnitude = Math.hypot(x, y, z);
  if (magnitude < 1) return null;
  return {
    roll: Math.atan2(y, z) * 180 / Math.PI,
    pitch: Math.atan2(-x, Math.hypot(y, z)) * 180 / Math.PI,
  };
}

function gyroWeight() {
  return Number(ui.gyroWeight.value) / 100;
}

function gyroScale() {
  const value = Number(ui.gyroScale.value);
  return Number.isFinite(value) && value > 0 ? value : 65536;
}

function setConnected(connected) {
  ui.connect.textContent = connected ? "Disconnect" : "Connect";
  ui.connectionState.textContent = connected ? "Serial connected" : "Disconnected";
  ui.connectionState.classList.toggle("connected", connected);
  ui.calibrate.disabled = !connected;
  ui.zero.disabled = !connected;
  if (!connected) {
    ui.sampleState.textContent = "Waiting for connection";
    ui.sampleState.classList.remove("live");
  }
}

function updateWeightLabel() {
  const weight = Number(ui.gyroWeight.value);
  ui.gyroWeightOutput.textContent = `${weight}% gyro · ${100 - weight}% accelerometer`;
}

function renderAngles() {
  const roll = wrap180(filter.roll);
  const pitch = Math.max(-90, Math.min(90, filter.pitch));
  const heading = wrap360(filter.heading);
  ui.roll.textContent = `${roll.toFixed(1)}°`;
  ui.pitch.textContent = `${pitch.toFixed(1)}°`;
  ui.heading.textContent = `${heading.toFixed(1)}°`;
  ui.attitudePlane.style.transform = `translateY(${pitch * 1.7}px) rotate(${-roll}deg)`;
  ui.compassRose.style.transform = `rotate(${-heading}deg)`;
}

function resetOrientation(accel) {
  const angles = accelerometerAngles(accel[0], accel[1], accel[2]);
  filter.roll = angles?.roll ?? 0;
  filter.pitch = angles?.pitch ?? 0;
  filter.heading = 0;
  filter.initialized = Boolean(angles);
  filter.lastSampleTime = 0;
  renderAngles();
}

function beginCalibration() {
  filter.calibration = { remaining: 60, sums: [0, 0, 0], lastAccel: [0, 0, 0] };
  ui.calibrate.disabled = true;
  ui.sampleState.textContent = "Calibrating — keep controller still";
}

function updateCalibration(gyro, accel) {
  if (!filter.calibration) return false;
  gyro.forEach((value, index) => { filter.calibration.sums[index] += value; });
  filter.calibration.lastAccel = accel;
  filter.calibration.remaining -= 1;
  if (filter.calibration.remaining > 0) {
    ui.sampleState.textContent = `Calibrating — ${filter.calibration.remaining} samples`;
    return true;
  }
  filter.gyroBias = filter.calibration.sums.map((sum) => sum / 60);
  const lastAccel = filter.calibration.lastAccel;
  filter.calibration = null;
  ui.calibrate.disabled = false;
  ui.biasValue.textContent = filter.gyroBias.map((value) => value.toFixed(0)).join(", ");
  ui.sampleState.textContent = "Live · calibrated";
  resetOrientation(lastAccel);
  return true;
}

function processSample(values) {
  const connected = values[0] === 1;
  const gyro = values.slice(8, 11);
  const accel = values.slice(11, 14);
  gyro.forEach((value, index) => { ui.gyroValues[index].textContent = value; });
  accel.forEach((value, index) => { ui.accelValues[index].textContent = value; });

  if (!connected) {
    filter.lastSampleTime = 0;
    ui.sampleState.textContent = "Gamepad not connected";
    ui.sampleState.classList.remove("live");
    return;
  }
  ui.sampleState.classList.add("live");
  if (updateCalibration(gyro, accel)) return;

  const now = performance.now();
  if (!filter.lastSampleTime) {
    filter.lastSampleTime = now;
    if (!filter.initialized) resetOrientation(accel);
    return;
  }
  const dt = Math.min(0.2, Math.max(0.001, (now - filter.lastSampleTime) / 1000));
  filter.lastSampleTime = now;
  ui.sampleInterval.textContent = `${(dt * 1000).toFixed(1)} ms`;

  const scale = gyroScale();
  const rateX = (gyro[0] - filter.gyroBias[0]) / scale;
  const rateY = (gyro[1] - filter.gyroBias[1]) / scale;
  const rateZ = (gyro[2] - filter.gyroBias[2]) / scale;
  const predictedRoll = filter.roll + rateX * dt;
  const predictedPitch = filter.pitch + rateY * dt;
  filter.heading = wrap360(filter.heading + rateZ * dt);

  const accelAngles = accelerometerAngles(accel[0], accel[1], accel[2]);
  const weight = gyroWeight();
  if (accelAngles) {
    filter.roll = blendAngle(predictedRoll, accelAngles.roll, weight);
    filter.pitch = blendAngle(predictedPitch, accelAngles.pitch, weight);
  } else {
    filter.roll = predictedRoll;
    filter.pitch = predictedPitch;
  }
  ui.sampleState.textContent = "Live";
  renderAngles();
}

function parseLine(rawLine) {
  const line = rawLine.trim();
  const match = line.match(/^gamepad:\s*(-?\d+(?:\s+-?\d+){13})$/i);
  if (match) processSample(match[1].split(/\s+/).map(Number));
}

async function send(command) {
  if (!writer) return;
  await writer.write(encoder.encode(`${command}\r`));
}

async function pollGamepad() {
  if (!writer) return;
  try {
    await send("GET GAMEPAD");
  } catch (error) {
    ui.sampleState.textContent = `Polling stopped: ${error.message}`;
    return;
  }
  if (writer) pollTimer = window.setTimeout(pollGamepad, 50);
}

function stopPolling() {
  if (pollTimer) window.clearTimeout(pollTimer);
  pollTimer = undefined;
}

async function readLoop() {
  const decoder = new TextDecoder();
  try {
    reader = port.readable.getReader();
    while (true) {
      const { value, done } = await reader.read();
      if (done) break;
      receiveBuffer += decoder.decode(value, { stream: true });
      const lines = receiveBuffer.split(/\r?\n|\r/);
      receiveBuffer = lines.pop() ?? "";
      lines.forEach(parseLine);
    }
  } catch (error) {
    if (port) ui.sampleState.textContent = `Read error: ${error.message}`;
  } finally {
    reader?.releaseLock();
    reader = undefined;
  }
}

async function closePort() {
  stopPolling();
  if (reader) await reader.cancel();
  if (readTask) {
    try { await readTask; } catch { /* readLoop reports errors */ }
    readTask = undefined;
  }
  writer?.releaseLock();
  writer = undefined;
  if (port) {
    const openPort = port;
    port = undefined;
    await openPort.close();
  }
}

async function toggleConnection() {
  try {
    if (port) {
      await closePort();
      setConnected(false);
      return;
    }
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 115200, bufferSize: 4096 });
    writer = port.writable.getWriter();
    readTask = readLoop();
    setConnected(true);
    await new Promise((resolve) => window.setTimeout(resolve, 1400));
    beginCalibration();
    pollGamepad();
  } catch (error) {
    ui.sampleState.textContent = `Connection error: ${error.message}`;
    await closePort().catch(() => {});
    setConnected(false);
  }
}

ui.connect.addEventListener("click", toggleConnection);
ui.calibrate.addEventListener("click", beginCalibration);
ui.zero.addEventListener("click", () => {
  filter.heading = 0;
  renderAngles();
});
ui.gyroWeight.addEventListener("input", updateWeightLabel);
ui.gyroScale.addEventListener("change", () => {
  if (gyroScale() !== Number(ui.gyroScale.value)) ui.gyroScale.value = "65536";
});

if (!("serial" in navigator)) {
  ui.support.textContent = "Web Serial is unavailable. Use current Chrome or Edge on HTTPS or localhost.";
  ui.connect.disabled = true;
}
updateWeightLabel();
renderAngles();
setConnected(false);
