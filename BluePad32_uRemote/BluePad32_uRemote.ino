/*
  BluePad32_uRemote.ino

  uRemote conversion of BluePad32_LPF2.ino.

  This sketch removes the LPF2 mode emulation layer and exposes the gamepad,
  NeoPixel writes, servo writes, and Bluetooth allow-list settings as short
  uRemote RPC commands.

  EEPROM is used only for Bluetooth-related settings:
    - allowed Bluetooth address
    - allow-list filter enabled/disabled

  Default UART:
    - 115200 baud
    - ESP32: Serial2 on GPIO 18 RX / GPIO 19 TX
    - LMS-ESP32 v2 / ESP32-PICO-V3-02: GPIO 8 RX / GPIO 7 TX

  From Pybricks/SPIKE/EV3 use the normal uRemote client, for example:
    ur.call("pad")
    ur.call("joyl")
    ur.call("joyr")
    ur.call("imu")
    ur.call("bt_allow", "AA:BB:CC:DD:EE:FF")
*/

#include <Arduino.h>
#include <Bluepad32.h>
#include <bt/uni_bt_allowlist.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "uRemote.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* ================= USER CONFIG ================= */

static const uint32_t UREMOTE_BAUD = 115200;

static const uint16_t EEPROM_SIZE = 64;
static const char CONFIG_MAGIC[] = "BPU32BT";

static const uint8_t DEFAULT_LED_PIN = 12;
static const uint8_t DEFAULT_LED_COUNT = 16;
static const uint8_t DEBUG_LED_PIN = 25;
static const uint8_t I2C_SDA_PIN = 5;
static const uint8_t I2C_SCL_PIN = 4;
static const uint8_t I2C_MAX_BYTES = 128;

/* ================= GLOBAL STATE ================= */

GamepadPtr myGamepads[BP32_MAX_GAMEPADS];

uint8_t RXD2 = 18;
uint8_t TXD2 = 19;
uint8_t is_lms_esp32_version2 = 0;

int servoPins[] = { 21, 22, 23, 25 };
bool attachedServos[] = { false, false, false, false };

uint8_t neopixel_gpio = DEFAULT_LED_PIN;
uint8_t neopixel_nrleds = DEFAULT_LED_COUNT;

struct BtConfig {
  char magic[8];
  byte bt_allow[6];
  bool bt_filter;
};

BtConfig bt_conf;
byte current_bt_mac[6] = { 0, 0, 0, 0, 0, 0 };

Adafruit_NeoPixel *leds = new Adafruit_NeoPixel(DEFAULT_LED_COUNT,
                                                DEFAULT_LED_PIN,
                                                NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel *debugLed = new Adafruit_NeoPixel(1,
                                                    DEBUG_LED_PIN,
                                                    NEO_GRB + NEO_KHZ800);

void handleRemote(const String &cmd,
                  const uRemoteArg *args,
                  uint8_t argc,
                  uRemoteResponse &response);

uRemote remote(Serial2, handleRemote);

/* ================= SMALL HELPERS ================= */

int clipInt(int n, int lower, int upper) {
  return MAX(lower, MIN(n, upper));
}

uint8_t axisByte(int value) {
  // Bluepad32 axes are normally about -512..512. Return 0..255.
  return (uint8_t)(clipInt(value + 512, 0, 1023) >> 2);
}

uint8_t triggerByte(int value) {
  return (uint8_t)(clipInt(value, 0, 1023) >> 2);
}

GamepadPtr firstGamepad() {
  GamepadPtr gp = myGamepads[0];
  if (gp && gp->isConnected()) {
    return gp;
  }
  return nullptr;
}

bool gamepadConnected() {
  return firstGamepad() != nullptr;
}

void makePadBytes(uint8_t out[10]) {
  memset(out, 0, 10);
  GamepadPtr gp = firstGamepad();
  if (!gp) return;

  out[0] = 1;
  out[1] = axisByte(gp->axisX());
  out[2] = axisByte(gp->axisY());
  out[3] = axisByte(gp->axisRX());
  out[4] = axisByte(gp->axisRY());
  out[5] = gp->buttons() & 0xff;
  out[6] = gp->dpad() & 0xff;
  out[7] = gp->miscButtons() & 0xff;
  out[8] = triggerByte(gp->brake());
  out[9] = triggerByte(gp->throttle());
}

void updateDebugLed() {
  if (!is_lms_esp32_version2) return;
  debugLed->setPixelColor(0, debugLed->Color(0,
                                             gamepadConnected() ? 10 : 0,
                                             gamepadConnected() ? 10 : 0));
  debugLed->show();
}

void rebuildNeoPixels(uint8_t count, uint8_t pin) {
  if (leds != nullptr) {
    leds->clear();
    leds->show();
    delete leds;
  }
  neopixel_nrleds = count;
  neopixel_gpio = pin;
  leds = new Adafruit_NeoPixel(count, pin, NEO_GRB + NEO_KHZ800);
  leds->begin();
  leds->clear();
  leds->show();
}

void readBtAddress() {
  GamepadPtr gp = firstGamepad();
  if (gp) {
    GamepadProperties properties = gp->getProperties();
    memcpy(current_bt_mac, properties.btaddr, 6);
  } else {
    memset(current_bt_mac, 0, 6);
  }
}

String macToString(const byte mac[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool parseMacString(const String &text, byte out[6]) {
  if (text.length() != 17) return false;

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t pos = i * 3;
    int hi = hexNibble(text.charAt(pos));
    int lo = hexNibble(text.charAt(pos + 1));
    if (hi < 0 || lo < 0) return false;
    if (i < 5 && text.charAt(pos + 2) != ':') return false;
    out[i] = (byte)((hi << 4) | lo);
  }
  return true;
}

void addMacToAllowList(const byte bt_mac[6]) {
  byte addr[6];
  memcpy(addr, bt_mac, 6);
  uni_bt_allowlist_remove_all();
  uni_bt_allowlist_add_addr(addr);
}

void setBtFilter(bool enabled) {
  bt_conf.bt_filter = enabled;
  uni_bt_allowlist_set_enabled(enabled);
}

void setBtDefaults() {
  strcpy(bt_conf.magic, CONFIG_MAGIC);
  memset(bt_conf.bt_allow, 0, 6);
  bt_conf.bt_filter = false;
}

bool loadBtConfig() {
  EEPROM.get(0, bt_conf);
  if (strncmp(bt_conf.magic, CONFIG_MAGIC, sizeof(bt_conf.magic)) != 0) {
    setBtDefaults();
    EEPROM.put(0, bt_conf);
    EEPROM.commit();
    return false;
  }
  return true;
}

void saveBtConfig() {
  EEPROM.put(0, bt_conf);
  EEPROM.commit();
}

void applyBtConfig() {
  addMacToAllowList(bt_conf.bt_allow);
  setBtFilter(bt_conf.bt_filter);
}

/* ================= I2C HELPERS ================= */

uint8_t scanI2C(uint8_t addresses[]) {
  uint8_t nDevices = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      addresses[nDevices++] = address;
    }
  }
  return nDevices;
}

uint8_t clampI2CLen(int len) {
  if (len < 0) return 0;
  if (len > I2C_MAX_BYTES) return I2C_MAX_BYTES;
  return (uint8_t)len;
}

uint8_t bytesFromArgs(const uRemoteArg *args, uint8_t start, uint8_t argc, uint8_t *buf, uint8_t maxLen) {
  uint8_t len = 0;
  for (uint8_t i = start; i < argc && len < maxLen; i++) {
    if (args[i].type == UREMOTE_TYPE_BYTES) {
      for (uint8_t j = 0; j < args[i].length && len < maxLen; j++) {
        buf[len++] = args[i].data[j];
      }
    } else {
      buf[len++] = (uint8_t)args[i].toInt();
    }
  }
  return len;
}

uint8_t readI2CBytes(uint8_t address, uint8_t *buf, uint8_t len) {
  uint8_t got = Wire.requestFrom((int)address, (int)len);
  uint8_t i = 0;
  while (Wire.available() && i < got && i < len) {
    buf[i++] = (uint8_t)Wire.read();
  }
  return i;
}

uint8_t readI2CRegBytes(uint8_t address, uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  uint8_t error = Wire.endTransmission(false);
  if (error != 0) return 0;
  return readI2CBytes(address, buf, len);
}

uint8_t writeI2CBytes(uint8_t address, const uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(address);
  for (uint8_t i = 0; i < len; i++) {
    Wire.write(buf[i]);
  }
  return Wire.endTransmission();
}

uint8_t writeI2CRegBytes(uint8_t address, uint8_t reg, const uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  for (uint8_t i = 0; i < len; i++) {
    Wire.write(buf[i]);
  }
  return Wire.endTransmission();
}

/* ================= SERVO HELPERS ================= */

#define FREQ_PWM 50
#define RESOLUTION_PWM 14

void setServoAngle(int chan, int angle) {
  angle = clipInt(angle, 0, 180);
  int duty = map(angle, 0, 180, 819, 1638);
  ledcWrite(chan, duty);
}

void attachServo(int chan, int pin) {
  ledcSetup(chan, FREQ_PWM, RESOLUTION_PWM);
  ledcAttachPin(pin, chan);
}

void stopServoPin(int pin) {
  ledcDetachPin(pin);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

bool setServo(uint8_t servoNr, int angle) {
  if (servoNr >= 4) return false;

  if (angle == 1000) {
    stopServoPin(servoPins[servoNr]);
    attachedServos[servoNr] = false;
    return true;
  }

  if (!attachedServos[servoNr]) {
    attachServo(servoNr, servoPins[servoNr]);
    attachedServos[servoNr] = true;
  }
  setServoAngle(servoNr, angle);
  return true;
}

void stopAllServos() {
  for (uint8_t i = 0; i < 4; i++) {
    stopServoPin(servoPins[i]);
    attachedServos[i] = false;
  }
}

/* ================= BLUEPAD32 CALLBACKS ================= */

void onConnectedGamepad(GamepadPtr gp) {
  if (myGamepads[0] == nullptr) {
    myGamepads[0] = gp;
    readBtAddress();
  }
  updateDebugLed();
}

void onDisconnectedGamepad(GamepadPtr gp) {
  if (myGamepads[0] == gp) {
    myGamepads[0] = nullptr;
  }
  readBtAddress();
  updateDebugLed();
}

/* ================= UREMOTE COMMAND HANDLER ================= */

void handleRemote(const String &cmd,
                  const uRemoteArg *args,
                  uint8_t argc,
                  uRemoteResponse &response) {
  if (cmd == "ping") {
    response.add(millis());
    return;
  }

  if (cmd == "ver") {
    response.add(String("BluePad32 uRemote 20260707-BT-I2C; BP32 ") + BP32.firmwareVersion());
    return;
  }

  if (cmd == "status") {
    response.add(gamepadConnected() ? 1 : 0);
    response.add((int)neopixel_nrleds);
    response.add((int)neopixel_gpio);
    response.add(bt_conf.bt_filter ? 1 : 0);
    return;
  }

  if (cmd == "pad") {
    uint8_t data[10];
    makePadBytes(data);
    response.add(data, sizeof(data));
    return;
  }

  if (cmd == "joy") {
    GamepadPtr gp = firstGamepad();
    response.add(gp ? axisByte(gp->axisX()) : 0);
    response.add(gp ? axisByte(gp->axisY()) : 0);
    response.add(gp ? axisByte(gp->axisRX()) : 0);
    response.add(gp ? axisByte(gp->axisRY()) : 0);
    return;
  }

  if (cmd == "joyl") {
    GamepadPtr gp = firstGamepad();
    response.add(gp ? axisByte(gp->axisX()) : 0);
    response.add(gp ? axisByte(gp->axisY()) : 0);
    return;
  }

  if (cmd == "joyr") {
    GamepadPtr gp = firstGamepad();
    response.add(gp ? axisByte(gp->axisRX()) : 0);
    response.add(gp ? axisByte(gp->axisRY()) : 0);
    return;
  }

  if (cmd == "imu") {
    GamepadPtr gp = firstGamepad();
    response.add(gp ? (int)gp->gyroX() : 0);
    response.add(gp ? (int)gp->gyroY() : 0);
    response.add(gp ? (int)gp->gyroZ() : 0);
    response.add(gp ? (int)gp->accelX() : 0);
    response.add(gp ? (int)gp->accelY() : 0);
    response.add(gp ? (int)gp->accelZ() : 0);
    return;
  }

  if (cmd == "btn") {
    GamepadPtr gp = firstGamepad();
    response.add(gp ? (int)(gp->buttons() & 0xffff) : 0);
    response.add(gp ? (int)(gp->dpad() & 0xff) : 0);
    response.add(gp ? (int)(gp->miscButtons() & 0xff) : 0);
    return;
  }

  if (cmd == "pix") {
    if (argc < 4) {
      response.setError("pix needs n,r,g,b");
      return;
    }
    uint8_t n = args[0];
    uint8_t r = args[1];
    uint8_t g = args[2];
    uint8_t b = args[3];
    if (n < leds->numPixels()) {
      leds->setPixelColor(n, r, g, b);
      leds->show();
    }
    response.add(1);
    return;
  }

  if (cmd == "fill") {
    if (argc < 3) {
      response.setError("fill needs r,g,b");
      return;
    }
    uint8_t r = args[0];
    uint8_t g = args[1];
    uint8_t b = args[2];
    for (uint16_t i = 0; i < leds->numPixels(); i++) {
      leds->setPixelColor(i, r, g, b);
    }
    leds->show();
    response.add(1);
    return;
  }

  if (cmd == "clear") {
    leds->clear();
    leds->show();
    response.add(1);
    return;
  }

  if (cmd == "np_cfg") {
    uint8_t count = neopixel_nrleds;
    uint8_t gpio = neopixel_gpio;
    if (argc >= 1) count = (byte)args[0].toInt();
    if (argc >= 2) gpio = (byte)args[1].toInt();
    if (count == 0 || count > 64) {
      response.setError("np_cfg count must be 1..64");
      return;
    }
    if (gpio > 39) {
      response.setError("np_cfg gpio must be 0..39");
      return;
    }
    rebuildNeoPixels(count, gpio);
    response.add((int)neopixel_nrleds);
    response.add((int)neopixel_gpio);
    return;
  }

  if (cmd == "servo") {
    if (argc == 0) {
      response.setError("servo needs n,angle or four angles");
      return;
    }
    if (argc == 2) {
      if (!setServo((uint8_t)args[0].toInt(), args[1].toInt())) {
        response.setError("servo index must be 0..3");
        return;
      }
      response.add(1);
      return;
    }
    if (argc >= 4) {
      for (uint8_t i = 0; i < 4; i++) {
        setServo(i, args[i].toInt());
      }
      response.add(1);
      return;
    }
    response.setError("servo needs n,angle or four angles");
    return;
  }

  if (cmd == "servo_off") {
    if (argc >= 1) {
      uint8_t n = args[0];
      if (n >= 4) {
        response.setError("servo index must be 0..3");
        return;
      }
      stopServoPin(servoPins[n]);
      attachedServos[n] = false;
    } else {
      stopAllServos();
    }
    response.add(1);
    return;
  }

  if (cmd == "i2c_scan") {
    uint8_t addresses[127];
    uint8_t nDevices = scanI2C(addresses);
    response.add((int)nDevices);
    response.add(addresses, nDevices);
    return;
  }

  if (cmd == "i2c_read") {
    if (argc < 2) {
      response.setError("i2c_read needs address,len");
      return;
    }
    uint8_t address = (uint8_t)args[0].toInt();
    uint8_t len = clampI2CLen(args[1].toInt());
    uint8_t buf[I2C_MAX_BYTES];
    uint8_t got = readI2CBytes(address, buf, len);
    response.add(buf, got);
    return;
  }

  if (cmd == "i2c_read_reg") {
    if (argc < 3) {
      response.setError("i2c_read_reg needs address,reg,len");
      return;
    }
    uint8_t address = (uint8_t)args[0].toInt();
    uint8_t reg = (uint8_t)args[1].toInt();
    uint8_t len = clampI2CLen(args[2].toInt());
    uint8_t buf[I2C_MAX_BYTES];
    uint8_t got = readI2CRegBytes(address, reg, buf, len);
    response.add(buf, got);
    return;
  }

  if (cmd == "i2c_write") {
    if (argc < 2) {
      response.setError("i2c_write needs address,data");
      return;
    }
    uint8_t address = (uint8_t)args[0].toInt();
    uint8_t buf[I2C_MAX_BYTES];
    uint8_t len = bytesFromArgs(args, 1, argc, buf, I2C_MAX_BYTES);
    uint8_t error = writeI2CBytes(address, buf, len);
    response.add((int)error);
    response.add((int)len);
    return;
  }

  if (cmd == "i2c_write_reg") {
    if (argc < 3) {
      response.setError("i2c_write_reg needs address,reg,data");
      return;
    }
    uint8_t address = (uint8_t)args[0].toInt();
    uint8_t reg = (uint8_t)args[1].toInt();
    uint8_t buf[I2C_MAX_BYTES];
    uint8_t len = bytesFromArgs(args, 2, argc, buf, I2C_MAX_BYTES);
    uint8_t error = writeI2CRegBytes(address, reg, buf, len);
    response.add((int)error);
    response.add((int)len);
    return;
  }

  if (cmd == "bt_mac") {
    readBtAddress();
    response.add(macToString(current_bt_mac));
    return;
  }

  if (cmd == "bt_allow") {
    if (argc >= 1) {
      if (args[0].type != UREMOTE_TYPE_STR) {
        response.setError("bt_allow needs xx:xx:xx:xx:xx:xx");
        return;
      }
      byte parsed[6];
      if (!parseMacString(args[0].toString(), parsed)) {
        response.setError("bt_allow needs xx:xx:xx:xx:xx:xx");
        return;
      }
      memcpy(bt_conf.bt_allow, parsed, 6);
      addMacToAllowList(bt_conf.bt_allow);
    }
    response.add(macToString(bt_conf.bt_allow));
    return;
  }

  if (cmd == "bt_filter") {
    if (argc >= 1) setBtFilter(args[0].toBool());
    response.add(bt_conf.bt_filter ? 1 : 0);
    return;
  }

  if (cmd == "bt_clear") {
    memset(bt_conf.bt_allow, 0, 6);
    bt_conf.bt_filter = false;
    uni_bt_allowlist_remove_all();
    uni_bt_allowlist_set_enabled(false);
    response.add(1);
    return;
  }

  if (cmd == "bt_forget") {
    BP32.forgetBluetoothKeys();
    response.add(1);
    return;
  }

  if (cmd == "save") {
    saveBtConfig();
    response.add(1);
    return;
  }

  if (cmd == "load") {
    loadBtConfig();
    applyBtConfig();
    response.add(1);
    return;
  }

  if (cmd == "defaults") {
    setBtDefaults();
    applyBtConfig();
    response.add(1);
    return;
  }

  response.setError(cmd + "() handler not found remotely");
}

/* ================= ARDUINO SETUP/LOOP ================= */

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setRxBufferSize(1000);

  if (strcmp(ESP.getChipModel(), "ESP32-PICO-V3-02") == 0) {
    is_lms_esp32_version2 = 1;
    RXD2 = 8;
    TXD2 = 7;
    servoPins[0] = 19;
    servoPins[1] = 20;
    servoPins[2] = 21;
    servoPins[3] = 22;
  }

  debugLed->begin();
  updateDebugLed();

  EEPROM.begin(EEPROM_SIZE);
  loadBtConfig();
  applyBtConfig();

  rebuildNeoPixels(neopixel_nrleds, neopixel_gpio);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  Serial2.begin(UREMOTE_BAUD, SERIAL_8N1, RXD2, TXD2);
  remote.begin(Serial2, handleRemote);

  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  Serial.println("BluePad32 uRemote ready");
  Serial.printf("UART: RX=%u TX=%u baud=%lu\n", RXD2, TXD2, (unsigned long)UREMOTE_BAUD);
  Serial.printf("BP32 firmware: %s\n", BP32.firmwareVersion());
  Serial.printf("I2C: SDA=%u SCL=%u\n", I2C_SDA_PIN, I2C_SCL_PIN);
}

void loop() {
  BP32.update();
  remote.process();
  delay(1);
}
