/*
  BluePad32_uRemote.ino

  Version 1.1

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
int servoAngles[] = { -1, -1, -1, -1 };

uint8_t neopixel_gpio = DEFAULT_LED_PIN;
uint8_t neopixel_nrleds = DEFAULT_LED_COUNT;

struct BtConfig {
  char magic[8];
  byte bt_allow[6];
  bool bt_filter;
};

BtConfig bt_conf;
byte current_bt_mac[6] = { 0, 0, 0, 0, 0, 0 };
bool bt_allow_new = true;

static const uint8_t SERIAL_COMMAND_MAX = 127;
char serialCommand[SERIAL_COMMAND_MAX + 1];
uint8_t serialCommandLength = 0;

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

/* ================= USB SERIAL CONFIG COMMANDS ================= */

void printBtMac(const char *name, const byte mac[6]) {
  Serial.printf("%s: %u %u %u %u %u %u\r\n", name,
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool parseByteValue(const String &text, byte &value) {
  if (text.length() == 0) return false;
  char *end = nullptr;
  long parsed = strtol(text.c_str(), &end, 0);
  if (*end != '\0' || parsed < 0 || parsed > 255) return false;
  value = (byte)parsed;
  return true;
}

bool parseHexByte(const String &text, byte &value) {
  String hex = text;
  if (hex.startsWith("0X")) hex.remove(0, 2);
  if (hex.length() < 1 || hex.length() > 2) return false;
  for (uint8_t i = 0; i < hex.length(); i++) {
    if (!isxdigit((unsigned char)hex.charAt(i))) return false;
  }
  value = (byte)strtoul(hex.c_str(), nullptr, 16);
  return true;
}

bool parseDecimalByte(const String &text, byte &value) {
  if (text.length() == 0) return false;
  for (uint8_t i = 0; i < text.length(); i++) {
    if (!isdigit((unsigned char)text.charAt(i))) return false;
  }
  unsigned long parsed = strtoul(text.c_str(), nullptr, 10);
  if (parsed > 255) return false;
  value = (byte)parsed;
  return true;
}

void printHexBytes(const char *name, const uint8_t *data, uint8_t length) {
  Serial.printf("%s:", name);
  for (uint8_t i = 0; i < length; i++) {
    Serial.printf(" %02X", data[i]);
  }
  Serial.println();
}

void printHexAddresses(const char *name, const uint8_t *addresses, uint8_t length) {
  Serial.printf("%s:", name);
  for (uint8_t i = 0; i < length; i++) {
    Serial.printf(" 0x%02X", addresses[i]);
  }
  Serial.println();
}

uint8_t splitCommand(const String &line, String tokens[], uint8_t maxTokens) {
  uint8_t count = 0;
  int pos = 0;
  while (pos < (int)line.length() && count < maxTokens) {
    while (pos < (int)line.length() && isspace((unsigned char)line.charAt(pos))) pos++;
    if (pos >= (int)line.length()) break;
    int start = pos;
    while (pos < (int)line.length() && !isspace((unsigned char)line.charAt(pos))) pos++;
    tokens[count++] = line.substring(start, pos);
  }
  return count;
}

void printSerialHelp() {
  Serial.println("Configuration commands (case-insensitive):");
  Serial.println("GET BT_MAC");
  Serial.println("GET BT_CON");
  Serial.println("GET BT_ALLOW");
  Serial.println("GET BT_FILTER");
  Serial.println("GET BT_ALLOW_LIST");
  Serial.println("GET BT_IN_ALLOW_LIST <b0> <b1> <b2> <b3> <b4> <b5>");
  Serial.println("GET BT_ALLOW_NEW");
  Serial.println("GET GAMEPAD");
  Serial.println("SET BT_ALLOW <b0> <b1> <b2> <b3> <b4> <b5>");
  Serial.println("SET BT_FILTER <0|1>");
  Serial.println("SET BT_CLEAR_ALLOW_LIST");
  Serial.println("SET BT_FORGET");
  Serial.println("SET BT_ALLOW_NEW <0|1>");
  Serial.println("GET NP_NR");
  Serial.println("GET NP_GPIO");
  Serial.println("SET NP_NR <1..64>");
  Serial.println("SET NP_GPIO <0..39>");
  Serial.println("NEOPIXEL SET <index> <r> <g> <b>");
  Serial.println("NEOPIXEL FILL <r> <g> <b>");
  Serial.println("NEOPIXEL CLEAR");
  Serial.println("GET SERVO");
  Serial.println("SERVO SET <0..3> <0..180>");
  Serial.println("SERVO OFF [0..3|ALL]");
  Serial.println("I2C SCAN");
  Serial.println("I2C READ <address> <length>");
  Serial.println("I2C READ_REG <address> <decimal_register> <length>");
  Serial.println("I2C WRITE <address> <hex_byte> [...]");
  Serial.println("I2C WRITE_REG <address> <decimal_register> <hex_byte> [...]");
  Serial.println("SAVE");
  Serial.println("OK");
}

void handleSerialGet(const String tokens[], uint8_t count) {
  if (count < 2) {
    Serial.println("ERROR: GET needs a BT_*, GAMEPAD, NP_*, or SERVO command");
    return;
  }

  if (tokens[1] == "BT_MAC") {
    readBtAddress();
    printBtMac("bt_mac", current_bt_mac);
  } else if (tokens[1] == "BT_CON") {
    Serial.printf("bt_con: %u\r\n", gamepadConnected() ? 1 : 0);
  } else if (tokens[1] == "BT_ALLOW") {
    printBtMac("bt_allow", bt_conf.bt_allow);
  } else if (tokens[1] == "BT_FILTER") {
    Serial.printf("bt_filter: %u\r\n", bt_conf.bt_filter ? 1 : 0);
  } else if (tokens[1] == "BT_ALLOW_NEW") {
    Serial.printf("bt_allow_new: %u\r\n", bt_allow_new ? 1 : 0);
  } else if (tokens[1] == "GAMEPAD") {
    GamepadPtr gp = firstGamepad();
    Serial.printf("gamepad: %u %d %d %d %d %u %u %u %d %d %d %d %d %d\r\n",
                  gp ? 1 : 0,
                  gp ? gp->axisX() : 0,
                  gp ? gp->axisY() : 0,
                  gp ? gp->axisRX() : 0,
                  gp ? gp->axisRY() : 0,
                  gp ? (unsigned int)(gp->buttons() & 0xffff) : 0,
                  gp ? (unsigned int)(gp->dpad() & 0xff) : 0,
                  gp ? (unsigned int)(gp->miscButtons() & 0xff) : 0,
                  gp ? (int)gp->gyroX() : 0,
                  gp ? (int)gp->gyroY() : 0,
                  gp ? (int)gp->gyroZ() : 0,
                  gp ? (int)gp->accelX() : 0,
                  gp ? (int)gp->accelY() : 0,
                  gp ? (int)gp->accelZ() : 0);
    return;
  } else if (tokens[1] == "BT_ALLOW_LIST") {
    const bd_addr_t *addresses;
    int addressCount = 0;
    uni_bt_allowlist_get_all(&addresses, &addressCount);
    Serial.println("allowed mac addresses:");
    Serial.printf("nr=%d\r\n", addressCount);
    Serial.printf("bt_allow_list_count: %d\r\n", addressCount);
    for (int i = 0; i < addressCount; i++) {
      Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",
                    addresses[i][0], addresses[i][1], addresses[i][2],
                    addresses[i][3], addresses[i][4], addresses[i][5]);
      Serial.printf("bt_allow_list: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                    addresses[i][0], addresses[i][1], addresses[i][2],
                    addresses[i][3], addresses[i][4], addresses[i][5]);
    }
  } else if (tokens[1] == "BT_IN_ALLOW_LIST") {
    if (count != 8) {
      Serial.println("ERROR: GET BT_IN_ALLOW_LIST needs 6 byte values");
      return;
    }
    byte mac[6];
    for (uint8_t i = 0; i < 6; i++) {
      if (!parseByteValue(tokens[i + 2], mac[i])) {
        Serial.println("ERROR: Bluetooth address bytes must be 0..255");
        return;
      }
    }
    bool allowed = uni_bt_allowlist_is_allowed_addr(mac);
    if (allowed) {
      Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X is in allow list\r\n",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    Serial.printf("bt_in_allow_list: %u\r\n", allowed ? 1 : 0);
  } else if (tokens[1] == "NP_NR") {
    Serial.printf("neopixel_nrleds: %u\r\n", neopixel_nrleds);
  } else if (tokens[1] == "NP_GPIO") {
    Serial.printf("neopixel_gpio: %u\r\n", neopixel_gpio);
  } else if (tokens[1] == "SERVO") {
    Serial.printf("servo_angles: %d %d %d %d\r\n",
                  servoAngles[0], servoAngles[1], servoAngles[2], servoAngles[3]);
  } else {
    Serial.println("ERROR: unknown GET command");
    return;
  }
  Serial.println("OK");
}

void handleSerialSet(const String tokens[], uint8_t count) {
  if (count < 2) {
    Serial.println("ERROR: SET needs a BT_* or NP_* command");
    return;
  }

  if (tokens[1] == "BT_ALLOW") {
    if (count != 8) {
      Serial.println("ERROR: SET BT_ALLOW needs 6 byte values");
      return;
    }
    byte mac[6];
    for (uint8_t i = 0; i < 6; i++) {
      if (!parseByteValue(tokens[i + 2], mac[i])) {
        Serial.println("ERROR: Bluetooth address bytes must be 0..255");
        return;
      }
    }
    memcpy(bt_conf.bt_allow, mac, 6);
    addMacToAllowList(bt_conf.bt_allow);
    printBtMac("bt_allow", bt_conf.bt_allow);
  } else if (tokens[1] == "BT_FILTER") {
    byte enabled;
    if (count != 3 || !parseByteValue(tokens[2], enabled) || enabled > 1) {
      Serial.println("ERROR: SET BT_FILTER needs 0 or 1");
      return;
    }
    setBtFilter(enabled == 1);
    Serial.printf("bt_filter: %u\r\n", bt_conf.bt_filter ? 1 : 0);
  } else if (tokens[1] == "BT_CLEAR_ALLOW_LIST") {
    memset(bt_conf.bt_allow, 0, 6);
    uni_bt_allowlist_remove_all();
    Serial.println("allowlist removed");
    Serial.println("bt_allow_list_count: 0");
  } else if (tokens[1] == "BT_FORGET") {
    BP32.forgetBluetoothKeys();
    Serial.println("bt_forget: 1");
  } else if (tokens[1] == "BT_ALLOW_NEW") {
    byte enabled;
    if (count != 3 || !parseByteValue(tokens[2], enabled) || enabled > 1) {
      Serial.println("ERROR: SET BT_ALLOW_NEW needs 0 or 1");
      return;
    }
    bt_allow_new = enabled == 1;
    BP32.enableNewBluetoothConnections(bt_allow_new);
    Serial.printf("bt_allow_new: %u\r\n", bt_allow_new ? 1 : 0);
  } else if (tokens[1] == "NP_NR") {
    byte countValue;
    if (count != 3 || !parseByteValue(tokens[2], countValue) ||
        countValue < 1 || countValue > 64) {
      Serial.println("ERROR: SET NP_NR needs a value from 1 to 64");
      return;
    }
    rebuildNeoPixels(countValue, neopixel_gpio);
    Serial.printf("neopixel_nrleds: %u\r\n", neopixel_nrleds);
  } else if (tokens[1] == "NP_GPIO") {
    byte gpio;
    if (count != 3 || !parseByteValue(tokens[2], gpio) || gpio > 39) {
      Serial.println("ERROR: SET NP_GPIO needs a value from 0 to 39");
      return;
    }
    rebuildNeoPixels(neopixel_nrleds, gpio);
    Serial.printf("neopixel_gpio: %u\r\n", neopixel_gpio);
  } else {
    Serial.println("ERROR: unknown SET command");
    return;
  }
  Serial.println("OK");
}

void handleSerialNeopixel(const String tokens[], uint8_t count) {
  if (count < 2) {
    Serial.println("ERROR: NEOPIXEL needs SET, FILL, or CLEAR");
    return;
  }

  if (tokens[1] == "CLEAR") {
    leds->clear();
    leds->show();
  } else if (tokens[1] == "FILL") {
    byte r, g, b;
    if (count != 5 || !parseByteValue(tokens[2], r) ||
        !parseByteValue(tokens[3], g) || !parseByteValue(tokens[4], b)) {
      Serial.println("ERROR: NEOPIXEL FILL needs r g b values from 0 to 255");
      return;
    }
    for (uint16_t i = 0; i < leds->numPixels(); i++) {
      leds->setPixelColor(i, r, g, b);
    }
    leds->show();
  } else if (tokens[1] == "SET") {
    byte index, r, g, b;
    if (count != 6 || !parseByteValue(tokens[2], index) ||
        !parseByteValue(tokens[3], r) || !parseByteValue(tokens[4], g) ||
        !parseByteValue(tokens[5], b)) {
      Serial.println("ERROR: NEOPIXEL SET needs index r g b");
      return;
    }
    if (index >= leds->numPixels()) {
      Serial.println("ERROR: NeoPixel index is outside the configured strip");
      return;
    }
    leds->setPixelColor(index, r, g, b);
    leds->show();
  } else {
    Serial.println("ERROR: unknown NEOPIXEL command");
    return;
  }
  Serial.println("OK");
}

void handleSerialServo(const String tokens[], uint8_t count) {
  if (count < 2) {
    Serial.println("ERROR: SERVO needs SET or OFF");
    return;
  }

  if (tokens[1] == "SET") {
    byte index;
    byte angle;
    if (count != 4 || !parseByteValue(tokens[2], index) || index > 3 ||
        !parseByteValue(tokens[3], angle) || angle > 180) {
      Serial.println("ERROR: SERVO SET needs index 0..3 and angle 0..180");
      return;
    }
    setServo(index, angle);
    Serial.printf("servo_%u: %u\r\n", index, angle);
  } else if (tokens[1] == "OFF") {
    if (count == 2 || (count == 3 && tokens[2] == "ALL")) {
      stopAllServos();
    } else {
      byte index;
      if (count != 3 || !parseByteValue(tokens[2], index) || index > 3) {
        Serial.println("ERROR: SERVO OFF needs an index from 0 to 3 or ALL");
        return;
      }
      setServo(index, 1000);
    }
  } else {
    Serial.println("ERROR: unknown SERVO command");
    return;
  }
  Serial.printf("servo_angles: %d %d %d %d\r\n",
                servoAngles[0], servoAngles[1], servoAngles[2], servoAngles[3]);
  Serial.println("OK");
}

void handleSerialI2C(const String tokens[], uint8_t count) {
  if (count < 2) {
    Serial.println("ERROR: I2C needs SCAN, READ, READ_REG, WRITE, or WRITE_REG");
    return;
  }

  if (tokens[1] == "SCAN") {
    uint8_t addresses[127];
    uint8_t found = scanI2C(addresses);
    Serial.printf("i2c_scan_count: %u\r\n", found);
    printHexAddresses("i2c_addresses", addresses, found);
    Serial.println("OK");
    return;
  }

  byte address;
  if (count < 3 || !parseByteValue(tokens[2], address) || address > 127) {
    Serial.println("ERROR: I2C address must be a decimal or 0x-prefixed value from 0 to 127");
    return;
  }

  if (tokens[1] == "READ") {
    byte length;
    if (count != 4 || !parseDecimalByte(tokens[3], length) ||
        length < 1 || length > I2C_MAX_BYTES) {
      Serial.println("ERROR: I2C READ length must be 1..128");
      return;
    }
    uint8_t data[I2C_MAX_BYTES];
    uint8_t received = readI2CBytes(address, data, length);
    Serial.printf("i2c_received: %u\r\n", received);
    printHexBytes("i2c_data", data, received);
  } else if (tokens[1] == "READ_REG") {
    byte reg;
    byte length;
    if (count != 5 || !parseDecimalByte(tokens[3], reg) ||
        !parseDecimalByte(tokens[4], length) || length < 1 ||
        length > I2C_MAX_BYTES) {
      Serial.println("ERROR: I2C READ_REG needs decimal register 0..255 and length 1..128");
      return;
    }
    uint8_t data[I2C_MAX_BYTES];
    uint8_t received = readI2CRegBytes(address, reg, data, length);
    Serial.printf("i2c_received: %u\r\n", received);
    printHexBytes("i2c_data", data, received);
  } else if (tokens[1] == "WRITE" || tokens[1] == "WRITE_REG") {
    bool withRegister = tokens[1] == "WRITE_REG";
    uint8_t dataStart = withRegister ? 4 : 3;
    byte reg = 0;
    if (withRegister && (count < 5 || !parseDecimalByte(tokens[3], reg))) {
      Serial.println("ERROR: I2C WRITE_REG needs a decimal register from 0 to 255");
      return;
    }
    if (count <= dataStart) {
      Serial.println("ERROR: I2C write needs at least one hexadecimal data byte");
      return;
    }
    uint8_t data[I2C_MAX_BYTES];
    uint8_t length = count - dataStart;
    for (uint8_t i = 0; i < length; i++) {
      if (!parseHexByte(tokens[dataStart + i], data[i])) {
        Serial.println("ERROR: I2C data bytes must be hexadecimal values such as 00 or FF");
        return;
      }
    }
    uint8_t error = withRegister
                      ? writeI2CRegBytes(address, reg, data, length)
                      : writeI2CBytes(address, data, length);
    Serial.printf("i2c_error: %u\r\n", error);
    Serial.printf("i2c_written: %u\r\n", length);
  } else {
    Serial.println("ERROR: unknown I2C command");
    return;
  }
  Serial.println("OK");
}

void handleSerialCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  String tokens[40];
  uint8_t count = splitCommand(line, tokens, 40);
  for (uint8_t i = 0; i < count; i++) tokens[i].toUpperCase();

  if (tokens[0] == "GET") {
    handleSerialGet(tokens, count);
  } else if (tokens[0] == "SET") {
    handleSerialSet(tokens, count);
  } else if (tokens[0] == "NEOPIXEL") {
    handleSerialNeopixel(tokens, count);
  } else if (tokens[0] == "SERVO") {
    handleSerialServo(tokens, count);
  } else if (tokens[0] == "I2C") {
    handleSerialI2C(tokens, count);
  } else if (tokens[0] == "SAVE") {
    saveBtConfig();
    Serial.println("OK");
  } else if (tokens[0] == "HELP") {
    printSerialHelp();
  } else {
    Serial.println("ERROR: supported commands are GET, SET, NEOPIXEL, SERVO, I2C, SAVE, HELP");
  }
}

void processSerialCommands() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (serialCommandLength > 0) {
        serialCommand[serialCommandLength] = '\0';
        handleSerialCommand(String(serialCommand));
        serialCommandLength = 0;
      }
    } else if (serialCommandLength < SERIAL_COMMAND_MAX) {
      serialCommand[serialCommandLength++] = c;
    } else {
      serialCommandLength = 0;
      Serial.println("ERROR: command is too long");
    }
  }
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
    servoAngles[servoNr] = -1;
    return true;
  }

  if (!attachedServos[servoNr]) {
    attachServo(servoNr, servoPins[servoNr]);
    attachedServos[servoNr] = true;
  }
  setServoAngle(servoNr, angle);
  servoAngles[servoNr] = angle;
  return true;
}

void stopAllServos() {
  for (uint8_t i = 0; i < 4; i++) {
    stopServoPin(servoPins[i]);
    attachedServos[i] = false;
    servoAngles[i] = -1;
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
    bool show_pixel = true;
    if (argc==5) {
      show_pixel = args[4];
    }
    if (n < leds->numPixels()) {
      leds->setPixelColor(n, r, g, b);
      if (show_pixel) leds->show();
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
      setServo(n, 1000);
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
  BP32.enableNewBluetoothConnections(bt_allow_new);

  Serial.println("BluePad32 uRemote V1.1 ready");
  Serial.printf("UART: RX=%u TX=%u baud=%lu\r\n", RXD2, TXD2, (unsigned long)UREMOTE_BAUD);
  Serial.printf("BP32 firmware: %s\r\n", BP32.firmwareVersion());
  Serial.printf("I2C: SDA=%u SCL=%u\r\n", I2C_SDA_PIN, I2C_SCL_PIN);
}

void loop() {
  BP32.update();
  remote.process();
  processSerialCommands();
  delay(1);
}
