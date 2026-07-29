"use strict";

const ui = {
  connect: document.querySelector("#connectButton"),
  refresh: document.querySelector("#refreshButton"),
  state: document.querySelector("#connectionState"),
  support: document.querySelector("#supportMessage"),
  controllerState: document.querySelector("#controllerState"),
  connectedMac: document.querySelector("#connectedMac"),
  allowListCount: document.querySelector("#allowListCount"),
  currentPixelCount: document.querySelector("#currentPixelCount"),
  currentPixelGpio: document.querySelector("#currentPixelGpio"),
  allowedMac: document.querySelector("#allowedMac"),
  filter: document.querySelector("#filterEnabled"),
  allowNew: document.querySelector("#allowNewEnabled"),
  useConnected: document.querySelector("#useConnectedButton"),
  apply: document.querySelector("#applyButton"),
  clear: document.querySelector("#clearButton"),
  forget: document.querySelector("#forgetButton"),
  allowList: document.querySelector("#allowList"),
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
let allowListEntries = [];
let i2cPending = false;
const encoder = new TextEncoder();

function setConnected(connected) {
  ui.connect.textContent = connected ? "Disconnect" : "Connect";
  ui.state.textContent = connected ? "Connected to serial port" : "Not connected";
  ui.state.className = `state alert ${connected ? "alert-success" : "alert-warning"}`;
  [
    ui.refresh, ui.allowedMac, ui.filter, ui.allowNew, ui.apply,
    ui.clear, ui.forget, ui.pixelCount, ui.pixelGpio, ui.applyNeopixel,
    ui.pixelIndex, ui.pixelColor, ui.setPixel, ui.fillPixels, ui.clearPixels,
    ui.servoOffAll, ...ui.servoInputs, ...ui.servoSetButtons, ...ui.servoOffButtons,
    ui.i2cAddress, ui.i2cRegister, ui.i2cLength, ui.i2cWriteData,
    ui.i2cScan, ui.i2cRead, ui.i2cReadReg, ui.i2cWrite, ui.i2cWriteReg,
  ].forEach((element) => { element.disabled = !connected; });
  ui.useConnected.disabled = !connected || !isMac(connectedMac) || isZeroMac(connectedMac);
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

async function send(command) {
  if (!writer) throw new Error("Serial port is not connected");
  appendLog(command, true);
  await writer.write(encoder.encode(`${command}\r`));
}

async function refreshSettings() {
  allowListEntries = [];
  renderAllowList();
  await send("GET BT_CON");
  await send("GET BT_MAC");
  await send("GET BT_ALLOW");
  await send("GET BT_FILTER");
  await send("GET BT_ALLOW_NEW");
  await send("GET BT_ALLOW_LIST");
  await send("GET NP_NR");
  await send("GET NP_GPIO");
  await send("GET SERVO");
}

function renderAllowList() {
  ui.allowList.replaceChildren();
  if (allowListEntries.length === 0) {
    const item = document.createElement("li");
    item.className = "text-muted";
    item.textContent = "The active allow list is empty.";
    ui.allowList.append(item);
    return;
  }
  allowListEntries.forEach((mac) => {
    const item = document.createElement("li");
    item.textContent = mac;
    ui.allowList.append(item);
  });
}

function parseLine(rawLine) {
  const line = rawLine.trim();
  if (!line) return;
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
  if (match) ui.allowedMac.value = bytesToMac(match[1].trim().split(/\s+/).map(Number));

  match = line.match(/^bt_filter:\s*([01])$/i);
  if (match) ui.filter.checked = match[1] === "1";

  match = line.match(/^bt_allow_new:\s*([01])$/i);
  if (match) ui.allowNew.checked = match[1] === "1";

  match = line.match(/^bt_allow_list_count:\s*(\d+)$/i);
  if (match) {
    ui.allowListCount.textContent = match[1];
    allowListEntries = [];
    renderAllowList();
  }

  match = line.match(/^bt_allow_list:\s*([0-9a-f:]{17})$/i);
  if (match) {
    allowListEntries.push(match[1].toUpperCase());
    renderAllowList();
  }

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
  if (match) appendI2cOutput(`Addresses: ${match[1].trim() || "(none)"}`);

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
}

async function disconnect() {
  await closePort();
  setConnected(false);
}

async function toggleConnection() {
  try {
    if (port) await disconnect();
    else await connect();
  } catch (error) {
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
  if (!window.confirm("Clear the Bluetooth allow list and save the empty list?")) return;
  await send("SET BT_FILTER 0");
  await send("SET BT_CLEAR_ALLOW_LIST");
  await send("SAVE");
  await refreshSettings();
}

async function forgetControllers() {
  if (!window.confirm("Forget all paired Bluetooth controllers? They will need to pair again.")) return;
  await send("SET BT_FORGET");
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
ui.forget.addEventListener("click", () => forgetControllers().catch((error) => appendLog(error.message)));
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
setConnected(false);
