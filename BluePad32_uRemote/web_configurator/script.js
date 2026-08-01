"use strict";

const ui = {
  connect: document.querySelector("#connectButton"),
  refresh: document.querySelector("#refreshButton"),
  state: document.querySelector("#connectionState"),
  support: document.querySelector("#supportMessage"),
  controllerState: document.querySelector("#controllerState"),
  connectedMac: document.querySelector("#connectedMac"),
  allowedMacStatus: document.querySelector("#allowedMacStatus"),
  currentPixelCount: document.querySelector("#currentPixelCount"),
  currentPixelGpio: document.querySelector("#currentPixelGpio"),
  gamepadDashboardEnabled: document.querySelector("#gamepadDashboardEnabled"),
  gamepadDashboardContent: document.querySelector("#gamepadDashboardContent"),
  gamepadLiveState: document.querySelector("#gamepadLiveState"),
  buttonIndicators: [...document.querySelectorAll("[data-button-bit]")],
  dpadIndicators: [...document.querySelectorAll("[data-dpad-bit]")],
  miscIndicators: [...document.querySelectorAll("[data-misc-bit]")],
  leftJoystick: document.querySelector("#leftJoystick"),
  rightJoystick: document.querySelector("#rightJoystick"),
  leftJoystickXValue: document.querySelector("#leftJoystickXValue"),
  leftJoystickYValue: document.querySelector("#leftJoystickYValue"),
  rightJoystickXValue: document.querySelector("#rightJoystickXValue"),
  rightJoystickYValue: document.querySelector("#rightJoystickYValue"),
  accelGraph: document.querySelector("#accelGraph"),
  gyroGraph: document.querySelector("#gyroGraph"),
  accelScale: document.querySelector("#accelScale"),
  gyroScale: document.querySelector("#gyroScale"),
  accelValues: ["X", "Y", "Z"].map((axis) => document.querySelector(`#accel${axis}Value`)),
  gyroValues: ["X", "Y", "Z"].map((axis) => document.querySelector(`#gyro${axis}Value`)),
  graphZoomButtons: [...document.querySelectorAll(".graph-zoom")],
  allowedMac: document.querySelector("#allowedMac"),
  filter: document.querySelector("#filterEnabled"),
  allowNew: document.querySelector("#allowNewEnabled"),
  useConnected: document.querySelector("#useConnectedButton"),
  apply: document.querySelector("#applyButton"),
  clear: document.querySelector("#clearButton"),
  pixelCount: document.querySelector("#pixelCount"),
  pixelGpio: document.querySelector("#pixelGpio"),
  applyNeopixel: document.querySelector("#applyNeopixelButton"),
  neopixelError: document.querySelector("#neopixelError"),
  pixelIndex: document.querySelector("#pixelIndex"),
  pixelColor: document.querySelector("#pixelColor"),
  setPixel: document.querySelector("#setPixelButton"),
  fillPixels: document.querySelector("#fillPixelsButton"),
  clearPixels: document.querySelector("#clearPixelsButton"),
  pixelTestError: document.querySelector("#pixelTestError"),
  servoInputs: [0, 1, 2, 3].map((index) => document.querySelector(`#servo${index}`)),
  servoStates: [0, 1, 2, 3].map((index) => document.querySelector(`#servoState${index}`)),
  servoSetButtons: [...document.querySelectorAll(".servo-set")],
  servoOffButtons: [...document.querySelectorAll(".servo-off")],
  servoOffAll: document.querySelector("#servoOffAllButton"),
  servoError: document.querySelector("#servoError"),
  i2cAddress: document.querySelector("#i2cAddress"),
  i2cRegister: document.querySelector("#i2cRegister"),
  i2cLength: document.querySelector("#i2cLength"),
  i2cWriteData: document.querySelector("#i2cWriteData"),
  i2cScan: document.querySelector("#i2cScanButton"),
  i2cRead: document.querySelector("#i2cReadButton"),
  i2cReadReg: document.querySelector("#i2cReadRegButton"),
  i2cWrite: document.querySelector("#i2cWriteButton"),
  i2cWriteReg: document.querySelector("#i2cWriteRegButton"),
  i2cError: document.querySelector("#i2cError"),
  i2cOutput: document.querySelector("#i2cOutput"),
  macError: document.querySelector("#macError"),
  log: document.querySelector("#log"),
  clearLog: document.querySelector("#clearLogButton"),
};

let port;
let reader;
let writer;
let readTask;
let receiveBuffer = "";
let connectedMac = "";
let i2cPending = false;
let gamepadPollTimer;
const encoder = new TextEncoder();
const graphColors = ["#dc3545", "#198754", "#0d6efd"];
const graphStates = {
  accel: { canvas: ui.accelGraph, scaleElement: ui.accelScale, scale: 1200, history: [[], [], []] },
  gyro: { canvas: ui.gyroGraph, scaleElement: ui.gyroScale, scale: 10000000, history: [[], [], []] },
};

function setConnected(connected) {
  ui.connect.textContent = connected ? "Disconnect" : "Connect";
  ui.state.textContent = connected ? "Connected to serial port" : "Not connected";
  ui.state.className = `state alert ${connected ? "alert-success" : "alert-warning"}`;
  [
    ui.refresh, ui.allowedMac, ui.filter, ui.allowNew, ui.apply,
    ui.clear, ui.pixelCount, ui.pixelGpio, ui.applyNeopixel,
    ui.pixelIndex, ui.pixelColor, ui.setPixel, ui.fillPixels, ui.clearPixels,
    ui.servoOffAll, ...ui.servoInputs, ...ui.servoSetButtons, ...ui.servoOffButtons,
    ui.i2cAddress, ui.i2cRegister, ui.i2cLength, ui.i2cWriteData,
    ui.i2cScan, ui.i2cRead, ui.i2cReadReg, ui.i2cWrite, ui.i2cWriteReg,
  ].forEach((element) => { element.disabled = !connected; });
  ui.useConnected.disabled = !connected || !isMac(connectedMac) || isZeroMac(connectedMac);
  if (!connected) {
    stopGamepadPolling();
    if (ui.gamepadDashboardEnabled.checked) {
      updateGamepadVisuals([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], false);
    }
    ui.gamepadLiveState.textContent = ui.gamepadDashboardEnabled.checked ? "Connect to start" : "Disabled — no polling";
    ui.gamepadLiveState.classList.remove("is-live");
  }
}

function appendLog(text, outgoing = false) {
  ui.log.textContent += `${outgoing ? "> " : ""}${text}\n`;
  ui.log.scrollTop = ui.log.scrollHeight;
}

function isMac(value) {
  return /^[0-9a-f]{2}(?::[0-9a-f]{2}){5}$/i.test(value.trim());
}

function isZeroMac(value) {
  return value.toUpperCase() === "00:00:00:00:00:00";
}

function macToBytes(value) {
  return value.trim().split(":").map((part) => Number.parseInt(part, 16));
}

function bytesToMac(numbers) {
  return numbers.map((value) => value.toString(16).padStart(2, "0")).join(":").toUpperCase();
}

function setIndicatorState(indicators, mask, dataName) {
  indicators.forEach((indicator) => {
    const pressed = (mask & Number(indicator.dataset[dataName])) !== 0;
    indicator.classList.toggle("pressed", pressed);
    indicator.setAttribute("aria-pressed", pressed ? "true" : "false");
  });
}

function drawJoystick(canvas, x, y) {
  const context = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;
  const centerX = width / 2;
  const centerY = height / 2;
  const radius = Math.min(width, height) / 2 - 20;
  const normalizedX = Math.max(-1, Math.min(1, x / 512));
  const normalizedY = Math.max(-1, Math.min(1, y / 512));

  context.clearRect(0, 0, width, height);
  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, width, height);
  context.strokeStyle = "#ced4da";
  context.lineWidth = 2;
  context.beginPath();
  context.arc(centerX, centerY, radius, 0, Math.PI * 2);
  context.stroke();
  context.strokeStyle = "#e2e6ea";
  context.lineWidth = 1;
  context.beginPath();
  context.moveTo(centerX - radius, centerY);
  context.lineTo(centerX + radius, centerY);
  context.moveTo(centerX, centerY - radius);
  context.lineTo(centerX, centerY + radius);
  context.stroke();
  context.fillStyle = "#0d6efd";
  context.beginPath();
  context.arc(centerX + normalizedX * radius, centerY + normalizedY * radius, 10, 0, Math.PI * 2);
  context.fill();
  context.strokeStyle = "#ffffff";
  context.lineWidth = 3;
  context.stroke();
}

function drawGraph(graph) {
  const context = graph.canvas.getContext("2d");
  const width = graph.canvas.width;
  const height = graph.canvas.height;
  const padding = 12;
  const plotHeight = height - padding * 2;

  context.clearRect(0, 0, width, height);
  context.fillStyle = "#ffffff";
  context.fillRect(0, 0, width, height);
  context.strokeStyle = "#e9ecef";
  context.lineWidth = 1;
  for (let row = 0; row <= 4; row += 1) {
    const y = padding + (plotHeight * row) / 4;
    context.beginPath();
    context.moveTo(0, y);
    context.lineTo(width, y);
    context.stroke();
  }
  for (let column = 1; column < 6; column += 1) {
    const x = (width * column) / 6;
    context.beginPath();
    context.moveTo(x, padding);
    context.lineTo(x, height - padding);
    context.stroke();
  }
  context.strokeStyle = "#adb5bd";
  context.beginPath();
  context.moveTo(0, height / 2);
  context.lineTo(width, height / 2);
  context.stroke();

  graph.history.forEach((series, seriesIndex) => {
    if (series.length < 2) return;
    context.strokeStyle = graphColors[seriesIndex];
    context.lineWidth = 2;
    context.beginPath();
    series.forEach((value, index) => {
      const x = (index / 179) * width;
      const normalized = Math.max(-1, Math.min(1, value / graph.scale));
      const y = height / 2 - normalized * plotHeight / 2;
      if (index === 0) context.moveTo(x, y);
      else context.lineTo(x, y);
    });
    context.stroke();
  });
}

function appendGraphSample(graph, values) {
  values.forEach((value, index) => {
    graph.history[index].push(value);
    if (graph.history[index].length > 180) graph.history[index].shift();
  });
  drawGraph(graph);
}

function updateGamepadVisuals(values, recordGraphs = true) {
  const [connected, leftX, leftY, rightX, rightY, buttons, dpad, misc,
    gyroX, gyroY, gyroZ, accelX, accelY, accelZ] = values;
  setIndicatorState(ui.buttonIndicators, buttons, "buttonBit");
  setIndicatorState(ui.dpadIndicators, dpad, "dpadBit");
  setIndicatorState(ui.miscIndicators, misc, "miscBit");
  drawJoystick(ui.leftJoystick, leftX, leftY);
  drawJoystick(ui.rightJoystick, rightX, rightY);
  ui.leftJoystickXValue.textContent = leftX;
  ui.leftJoystickYValue.textContent = leftY;
  ui.rightJoystickXValue.textContent = rightX;
  ui.rightJoystickYValue.textContent = rightY;
  [gyroX, gyroY, gyroZ].forEach((value, index) => { ui.gyroValues[index].textContent = value; });
  [accelX, accelY, accelZ].forEach((value, index) => { ui.accelValues[index].textContent = value; });
  ui.gamepadLiveState.textContent = connected ? "Live" : "Controller not connected";
  ui.gamepadLiveState.classList.toggle("is-live", connected === 1);
  ui.controllerState.textContent = connected ? "Connected" : "Not connected";
  if (recordGraphs) {
    appendGraphSample(graphStates.gyro, [gyroX, gyroY, gyroZ]);
    appendGraphSample(graphStates.accel, [accelX, accelY, accelZ]);
  }
}

function zoomGraph(name, direction) {
  const graph = graphStates[name];
  const factor = direction === "in" ? 2 / 3 : 1.5;
  graph.scale = Math.max(10, Math.min(1000000000, Math.round(graph.scale * factor)));
  graph.scaleElement.textContent = `±${graph.scale}`;
  drawGraph(graph);
}

async function pollGamepad() {
  if (!writer || !ui.gamepadDashboardEnabled.checked) return;
  try {
    await send("GET GAMEPAD", false);
  } catch (error) {
    ui.gamepadLiveState.textContent = "Readout stopped";
    ui.gamepadLiveState.classList.remove("is-live");
    appendLog(`Gamepad readout error: ${error.message}`);
    return;
  }
  if (writer && ui.gamepadDashboardEnabled.checked) {
    gamepadPollTimer = window.setTimeout(pollGamepad, 50);
  }
}

function startGamepadPolling() {
  stopGamepadPolling();
  if (!writer || !ui.gamepadDashboardEnabled.checked) return;
  ui.gamepadLiveState.textContent = "Waiting for controller…";
  pollGamepad();
}

function stopGamepadPolling() {
  if (gamepadPollTimer) window.clearTimeout(gamepadPollTimer);
  gamepadPollTimer = undefined;
}

function setGamepadDashboardEnabled(enabled) {
  ui.gamepadDashboardEnabled.checked = enabled;
  ui.gamepadDashboardEnabled.setAttribute("aria-expanded", enabled ? "true" : "false");
  ui.gamepadDashboardContent.hidden = !enabled;
  if (!enabled) {
    stopGamepadPolling();
    ui.gamepadLiveState.textContent = "Disabled — no polling";
    ui.gamepadLiveState.classList.remove("is-live");
    return;
  }
  updateGamepadVisuals([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], false);
  if (writer) startGamepadPolling();
  else ui.gamepadLiveState.textContent = "Connect to start";
}

function formatI2cAddresses(value) {
  const addresses = value.trim().split(/\s+/).filter(Boolean);
  if (addresses.length === 0) return "(none)";
  return addresses.map((address) => {
    const hex = address.replace(/^0x/i, "").toUpperCase();
    return `0x${hex.padStart(2, "0")}`;
  }).join(", ");
}

function colorBytes(value) {
  return [
    Number.parseInt(value.slice(1, 3), 16),
    Number.parseInt(value.slice(3, 5), 16),
    Number.parseInt(value.slice(5, 7), 16),
  ];
}

function parseI2cAddress() {
  const text = ui.i2cAddress.value.trim();
  const address = Number(text);
  if (text === "" || !Number.isInteger(address) || address < 0 || address > 127) {
    throw new Error("I²C address must be decimal or 0x-prefixed hexadecimal from 0 to 127.");
  }
  return address;
}

function decimalField(element, name, minimum, maximum) {
  const text = element.value.trim();
  const value = Number(text);
  if (!/^\d+$/.test(text) || !Number.isInteger(value) || value < minimum || value > maximum) {
    throw new Error(`${name} must be a decimal whole number from ${minimum} to ${maximum}.`);
  }
  return value;
}

function parseHexData() {
  const parts = ui.i2cWriteData.value.trim().split(/[\s,]+/).filter(Boolean);
  if (parts.length === 0) throw new Error("Enter at least one hexadecimal data byte.");
  if (parts.length > 35) throw new Error("A USB command can contain at most 35 data bytes.");
  return parts.map((part) => {
    const normalized = part.replace(/^0x/i, "");
    if (!/^[0-9a-f]{1,2}$/i.test(normalized)) {
      throw new Error(`“${part}” is not a hexadecimal byte.`);
    }
    return normalized.padStart(2, "0").toUpperCase();
  });
}

function beginI2cOutput(message) {
  ui.i2cError.textContent = "";
  ui.i2cOutput.textContent = message;
  i2cPending = true;
}

function appendI2cOutput(message) {
  if (ui.i2cOutput.textContent === "Waiting for board…") {
    ui.i2cOutput.textContent = message;
  } else {
    ui.i2cOutput.textContent += `\n${message}`;
  }
}

async function send(command, logCommand = true) {
  if (!writer) throw new Error("Serial port is not connected");
  if (logCommand) appendLog(command, true);
  await writer.write(encoder.encode(`${command}\r`));
}

async function refreshSettings() {
  await send("GET BT_CON");
  await send("GET BT_MAC");
  await send("GET BT_ALLOW");
  await send("GET BT_FILTER");
  await send("GET BT_ALLOW_NEW");
  await send("GET NP_NR");
  await send("GET NP_GPIO");
  await send("GET SERVO");
}

function parseLine(rawLine) {
  const line = rawLine.trim();
  if (!line) return;
  const gamepadMatch = line.match(/^gamepad:\s*(-?\d+(?:\s+-?\d+){13})$/i);
  if (gamepadMatch) {
    if (ui.gamepadDashboardEnabled.checked) {
      updateGamepadVisuals(gamepadMatch[1].split(/\s+/).map(Number));
    }
    return;
  }
  appendLog(line);

  if (i2cPending && /^ERROR:/i.test(line)) {
    ui.i2cError.textContent = line;
    ui.i2cOutput.textContent = "Operation failed.";
    i2cPending = false;
  } else if (i2cPending && line === "OK") {
    i2cPending = false;
  }

  let match = line.match(/^bt_con:\s*([01])$/i);
  if (match) ui.controllerState.textContent = match[1] === "1" ? "Connected" : "Not connected";

  match = line.match(/^bt_mac:\s*((?:\d+\s+){5}\d+)$/i);
  if (match) {
    connectedMac = bytesToMac(match[1].trim().split(/\s+/).map(Number));
    ui.connectedMac.textContent = connectedMac;
    ui.useConnected.disabled = !isMac(connectedMac) || isZeroMac(connectedMac);
  }

  match = line.match(/^bt_allow:\s*((?:\d+\s+){5}\d+)$/i);
  if (match) {
    const allowedMac = bytesToMac(match[1].trim().split(/\s+/).map(Number));
    const hasAllowedMac = !isZeroMac(allowedMac);
    ui.allowedMac.value = hasAllowedMac ? allowedMac : "";
    ui.allowedMacStatus.textContent = hasAllowedMac ? allowedMac : "None";
  }

  match = line.match(/^bt_filter:\s*([01])$/i);
  if (match) ui.filter.checked = match[1] === "1";

  match = line.match(/^bt_allow_new:\s*([01])$/i);
  if (match) ui.allowNew.checked = match[1] === "1";

  match = line.match(/^neopixel_nrleds:\s*(\d+)$/i);
  if (match) {
    ui.pixelCount.value = match[1];
    ui.currentPixelCount.textContent = match[1];
  }

  match = line.match(/^neopixel_gpio:\s*(\d+)$/i);
  if (match) {
    ui.pixelGpio.value = match[1];
    ui.currentPixelGpio.textContent = match[1];
  }

  match = line.match(/^servo_angles:\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)$/i);
  if (match) {
    match.slice(1).map(Number).forEach((angle, index) => {
      ui.servoStates[index].textContent = angle < 0 ? "Detached" : `${angle}°`;
      if (angle >= 0) ui.servoInputs[index].value = angle;
    });
  }

  match = line.match(/^i2c_scan_count:\s*(\d+)$/i);
  if (match) appendI2cOutput(`Found: ${match[1]} device(s)`);

  match = line.match(/^i2c_addresses:\s*(.*)$/i);
  if (match) appendI2cOutput(`Addresses (hex): ${formatI2cAddresses(match[1])}`);

  match = line.match(/^i2c_received:\s*(\d+)$/i);
  if (match) appendI2cOutput(`Received: ${match[1]} byte(s)`);

  match = line.match(/^i2c_data:\s*(.*)$/i);
  if (match) appendI2cOutput(`Data: ${match[1].trim() || "(none)"}`);

  match = line.match(/^i2c_error:\s*(\d+)$/i);
  if (match) appendI2cOutput(`Result: ${match[1] === "0" ? "ACK (0)" : `I²C error ${match[1]}`}`);

  match = line.match(/^i2c_written:\s*(\d+)$/i);
  if (match) appendI2cOutput(`Written: ${match[1]} byte(s)`);
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
    if (port) {
      appendLog(`Serial read error: ${error.message}`);
    }
  } finally {
    if (reader) {
      reader.releaseLock();
      reader = undefined;
    }
  }
}

async function closePort() {
  if (reader) await reader.cancel();
  if (readTask) {
    try {
      await readTask;
    } catch {
      // Any read error was already reported by readLoop().
    } finally {
      readTask = undefined;
    }
  }
  if (writer) {
    writer.releaseLock();
    writer = undefined;
  }
  if (port) {
    const openPort = port;
    port = undefined;
    await openPort.close();
  }
}

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function connect() {
  if (!("serial" in navigator)) {
    ui.support.textContent = "Web Serial is not supported. Use current Chrome or Edge on HTTPS or localhost.";
    return;
  }
  port = await navigator.serial.requestPort();
  await port.open({ baudRate: 115200, bufferSize: 4096 });
  writer = port.writable.getWriter();
  setConnected(true);
  readTask = readLoop();
  // Opening a serial port can reset an ESP32. Give setup time to finish.
  await delay(1400);
  await refreshSettings();
  if (ui.gamepadDashboardEnabled.checked) startGamepadPolling();
}

async function disconnect() {
  stopGamepadPolling();
  await closePort();
  setConnected(false);
}

async function toggleConnection() {
  try {
    if (port) await disconnect();
    else await connect();
  } catch (error) {
    stopGamepadPolling();
    appendLog(`Connection error: ${error.message}`);
    setConnected(false);
  }
}

async function applyAndSave() {
  const mac = ui.allowedMac.value.trim().toUpperCase();
  if (!isMac(mac)) {
    ui.macError.textContent = "Enter six hexadecimal bytes separated by colons.";
    return;
  }
  ui.macError.textContent = "";
  const bytes = macToBytes(mac);
  await send(`SET BT_ALLOW ${bytes.join(" ")}`);
  await send(`SET BT_FILTER ${ui.filter.checked ? 1 : 0}`);
  await send(`SET BT_ALLOW_NEW ${ui.allowNew.checked ? 1 : 0}`);
  await send("SAVE");
  await refreshSettings();
}

async function clearAllowList() {
  if (!window.confirm("Clear the allowed Bluetooth controller address?")) return;
  await send("SET BT_FILTER 0");
  await send("SET BT_CLEAR_ALLOW_LIST");
  await send("SAVE");
  await refreshSettings();
}

async function applyNeopixelSettings() {
  const count = Number(ui.pixelCount.value);
  const gpio = Number(ui.pixelGpio.value);
  if (!Number.isInteger(count) || count < 1 || count > 64) {
    ui.neopixelError.textContent = "NeoPixel count must be a whole number from 1 to 64.";
    return;
  }
  if (!Number.isInteger(gpio) || gpio < 0 || gpio > 39) {
    ui.neopixelError.textContent = "GPIO must be a whole number from 0 to 39.";
    return;
  }
  ui.neopixelError.textContent = "";
  await send(`SET NP_NR ${count}`);
  await send(`SET NP_GPIO ${gpio}`);
  await send("GET NP_NR");
  await send("GET NP_GPIO");
}

async function setSelectedPixel(fill = false) {
  const count = Number(ui.pixelCount.value);
  const index = Number(ui.pixelIndex.value);
  if (!fill && (!Number.isInteger(index) || index < 0 || index >= count)) {
    ui.pixelTestError.textContent = `Pixel index must be from 0 to ${Math.max(0, count - 1)}.`;
    return;
  }
  ui.pixelTestError.textContent = "";
  const [r, g, b] = colorBytes(ui.pixelColor.value);
  await send(fill
    ? `NEOPIXEL FILL ${r} ${g} ${b}`
    : `NEOPIXEL SET ${index} ${r} ${g} ${b}`);
}

async function setServoChannel(index) {
  const angle = Number(ui.servoInputs[index].value);
  if (!Number.isInteger(angle) || angle < 0 || angle > 180) {
    ui.servoError.textContent = `Servo ${index + 1} angle must be from 0 to 180.`;
    return;
  }
  ui.servoError.textContent = "";
  await send(`SERVO SET ${index} ${angle}`);
}

async function runI2c(operation) {
  try {
    beginI2cOutput("Waiting for board…");
    if (operation === "scan") {
      await send("I2C SCAN");
      return;
    }
    const address = parseI2cAddress();
    if (operation === "read") {
      const length = decimalField(ui.i2cLength, "Read length", 1, 128);
      await send(`I2C READ ${address} ${length}`);
    } else if (operation === "readReg") {
      const register = decimalField(ui.i2cRegister, "Register", 0, 255);
      const length = decimalField(ui.i2cLength, "Read length", 1, 128);
      await send(`I2C READ_REG ${address} ${register} ${length}`);
    } else {
      const bytes = parseHexData();
      let command = `I2C WRITE ${address} ${bytes.join(" ")}`;
      if (operation === "writeReg") {
        const register = decimalField(ui.i2cRegister, "Register", 0, 255);
        command = `I2C WRITE_REG ${address} ${register} ${bytes.join(" ")}`;
      }
      await send(command);
    }
  } catch (error) {
    ui.i2cError.textContent = error.message;
  }
}

ui.connect.addEventListener("click", toggleConnection);
ui.refresh.addEventListener("click", () => refreshSettings().catch((error) => appendLog(error.message)));
ui.apply.addEventListener("click", () => applyAndSave().catch((error) => appendLog(error.message)));
ui.clear.addEventListener("click", () => clearAllowList().catch((error) => appendLog(error.message)));
ui.applyNeopixel.addEventListener("click", () => applyNeopixelSettings().catch((error) => appendLog(error.message)));
ui.setPixel.addEventListener("click", () => setSelectedPixel(false).catch((error) => appendLog(error.message)));
ui.fillPixels.addEventListener("click", () => setSelectedPixel(true).catch((error) => appendLog(error.message)));
ui.clearPixels.addEventListener("click", () => send("NEOPIXEL CLEAR").catch((error) => appendLog(error.message)));
ui.servoSetButtons.forEach((button) => {
  button.addEventListener("click", () => setServoChannel(Number(button.dataset.servo)).catch((error) => appendLog(error.message)));
});
ui.servoOffButtons.forEach((button) => {
  button.addEventListener("click", () => send(`SERVO OFF ${button.dataset.servo}`).catch((error) => appendLog(error.message)));
});
ui.servoOffAll.addEventListener("click", () => send("SERVO OFF ALL").catch((error) => appendLog(error.message)));
ui.i2cScan.addEventListener("click", () => runI2c("scan"));
ui.i2cRead.addEventListener("click", () => runI2c("read"));
ui.i2cReadReg.addEventListener("click", () => runI2c("readReg"));
ui.i2cWrite.addEventListener("click", () => runI2c("write"));
ui.i2cWriteReg.addEventListener("click", () => runI2c("writeReg"));
ui.graphZoomButtons.forEach((button) => {
  button.addEventListener("click", () => zoomGraph(button.dataset.graph, button.dataset.zoom));
});
ui.gamepadDashboardEnabled.addEventListener("change", () => {
  setGamepadDashboardEnabled(ui.gamepadDashboardEnabled.checked);
});
ui.useConnected.addEventListener("click", () => {
  ui.allowedMac.value = connectedMac;
  ui.macError.textContent = "";
});
ui.allowedMac.addEventListener("input", () => { ui.macError.textContent = ""; });
ui.pixelCount.addEventListener("input", () => { ui.neopixelError.textContent = ""; });
ui.pixelGpio.addEventListener("input", () => { ui.neopixelError.textContent = ""; });
ui.pixelIndex.addEventListener("input", () => { ui.pixelTestError.textContent = ""; });
ui.servoInputs.forEach((input) => input.addEventListener("input", () => { ui.servoError.textContent = ""; }));
[ui.i2cAddress, ui.i2cRegister, ui.i2cLength, ui.i2cWriteData].forEach((input) => {
  input.addEventListener("input", () => { ui.i2cError.textContent = ""; });
});
ui.clearLog.addEventListener("click", () => { ui.log.textContent = ""; });

if (!("serial" in navigator)) {
  ui.support.textContent = "Web Serial is not supported. Use current Chrome or Edge on HTTPS or localhost.";
  ui.connect.disabled = true;
}
setGamepadDashboardEnabled(false);
setConnected(false);
