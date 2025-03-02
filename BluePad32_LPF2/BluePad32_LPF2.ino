/*

  from pybricks.iodevices import PUPDevice
  from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
  from pybricks.robotics import DriveBase
  from pybricks.tools import wait, StopWatch
  import ustruct

  p=PUPDevice(Port.A)

  while True:
    a=p.read(0)
    outp=ustruct.unpack('6h',ustruct.pack('12b',*a))
    print(outp)
*/
#include <bitset>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// uncomment the following line if building for PyBricks
//#define PYBRICKS 1
#define SPIKE3 1

//#include <WebServer.h>
//#include <ESPmDNS.h>
//#include <WebConfig.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <Bluepad32.h>
// installed version 4.1.0
// see https://github.com/ricardoquesada/bluepad32/blob/main/docs/plat_arduino.md
#include <bt/uni_bt_allowlist.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// get a slightly adapted LPF2 version from the program directory
#include "LPF2.h"
#include <Wire.h>
// Adafruit NeoPixe by Adafruit version 1.12.3
#include <Adafruit_NeoPixel.h>
// ESP32Servo by Kevin Harrington, John K.Bennett v3.0.6 can be installed via library manager
#include <ESP32Servo.h>
/*
patch ESP32PWM.h in <library>/ESP32Servo/ESP32PWM.h:
change:
#else
		if ((pin == 2) || //1
				(pin == 4) || //1
				(pin == 5) || //1
				((pin >= 12) && (pin <= 19)) || //8
				((pin >= 21) && (pin <= 23)) || //3
				((pin >= 25) && (pin <= 27)) || //3
				(pin == 32) || (pin == 33)) //2
to:
#else
		if ((pin == 2) || //1
				(pin == 4) || //1
				(pin == 5) || //1
				((pin >= 12) && (pin <= 20)) || //8
				((pin >= 21) && (pin <= 23)) || //3
				((pin >= 25) && (pin <= 27)) || //3
				(pin == 32) || (pin == 33)) //2

*/

GamepadPtr myGamepads[BP32_MAX_GAMEPADS];
byte RXD2 = 18;
byte TXD2 = 19;
#define COLOR_MATRIX 0x40
#define COLOR_SENSOR 0x3D
#define DEFAULT_SENSOR COLOR_SENSOR
#include <EEPROM.h>
#define EEPROM_SIZE 256

#define LED_PIN 12
#define LED_COUNT 16

Adafruit_NeoPixel *strip = new Adafruit_NeoPixel(LED_COUNT, LED_PIN);  //, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel *neopixel_debug = new Adafruit_NeoPixel(1, 25);               //, neopixel lms_esp32_v2
Adafruit_NeoPixel *neopixel_strip = new Adafruit_NeoPixel(LED_COUNT, LED_PIN);  //nr_leds, pin

byte connected = 0;
byte last_status = 0;
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

int servo1Pin = 21;
int servo2Pin = 22;
int servo3Pin = 23;
int servo4Pin = 25;

int minUs = 1000;
int maxUs = 2000;

char version[] = "BluePad32 LPF2 generic version 20250302";
#include <CmdBuffer.hpp>
#include <CmdCallback.hpp>
#include <CmdParser.hpp>
// https://github.com/ricardoquesada/bluepad32/blob/main/docs/plat_arduino.md
// CmdParser v1.7
// can be installed via library manager

CmdCallback<15> cmdCallback;

CmdBuffer<50> myBuffer;
CmdParser myParser;

char strHelp[] = "HELP";
char strSet[] = "SET";
char strGet[] = "GET";
char strShow[] = "SHOW";
char strSave[] = "SAVE";
char strDefault[] = "DEFAULT";
char strResponse[] = "RESPONSE";
char strNeopixel[] = "NEOPIXEL";
char strEeprom[] = "EEPROM";

char strMAGIC[] = "LMSESP";

byte response = 1;  // no response of commands except OK or ERROR


byte is_lms_esp32_version2 = 0;  // filled by calling ESP.getChipModel()
/*

  div_palette = [
  (  0,  0,  0), # (  0,  0,  0), #  0 black
  (225, 75,115), # (215,  0,154), #  1 magenta
  (215,135,220), # (163, 74,198), #  2 violet
  ( 85,115,240), # ( 24,145,242), #  3 blue
  (105,205,220), # ( 21,180,160), #  4 turquoise
  (115,210,170), # ( 32,189,124), #  5 mint
  (135,235, 90), # ( 15,168, 73), #  6 green
  (230,230, 90), # (249,212, 84), #  7 yellow
  (250,165, 80), # (253,162, 51), #  8 orange
  (255, 70, 70), # (253,  0, 30), #  9 red
  (255,255,255), # (255,255,255), # 10 white
  ]
  // Maarten pennigs
  ['#000000', '#E14B73', '#D787DC', '#5573F0', '#69CDDC', '#73D2AA', '#87EB5A', '#E6E65A', '#FAA550', '#FF4646', '#FFFFFF']
  // zelf
  ['#000000', '#C8C8FF', '#FF00FF', '#0000FF', '#00FFFF', '#00FF96', '#00FF00', '#FFFF00', '#FF8C00', '#FF0000', '#FFFFFF']
*/

// zelf
byte lego_colors[11][3] = { { 0, 0, 0 }, { 200, 200, 255 }, { 255, 0, 255 }, { 0, 0, 255 }, { 0, 255, 255 }, { 0, 255, 150 }, { 0, 255, 0 }, { 255, 255, 0 }, { 255, 140, 0 }, { 255, 0, 0 }, { 255, 255, 255 } };
// 5x5 matrix
// byte led_mapping[9] = { 3, 4, 5, 11, 12, 13, 19, 20, 21 };
// 3x3 matrix
byte led_mapping[9][8] = {};  //empty bitssets




// configure as color matrix sensor by default
EV3UARTEmulation *lpf2_sensor = new EV3UARTEmulation(RXD2, TXD2, COLOR_MATRIX, 115200);  // light sensor 0x3d

//char bt_allow[6] = { 0, 0, 0, 0, 0, 0 };
//bool bt_filter = false;

int debug = 0;  // global debug; if 1 -> print debug.



struct Sensor {
  char magic[7];  // magic for checking eeprom
  byte sensor_id;
  byte neopixel_gpio;
  byte neopixel_nrleds;
  byte led_mapping[9][8];
  byte lego_colors[11][3];
  byte bt_allow[6];
  byte bt_mac[6];
  bool bt_filter;
} sensor_conf;
// use pointer allows to dynamically change nrumber of leds or pin
// change strip.begin() to strip->begin(), etc.
// delete object before initiating a new one


#define FILL 0x10
#define ZERO 0x20
#define SET 0x30
#define CONFIG 0x40
#define WRITE 0x80

void pybricks_neopixel_callback(byte buf[], byte s) {
  // Serial.println();
  byte num_pixels = strip->numPixels();
  byte cmd = buf[0] & 0x7f;  // all but bit 7 (write) and strip lower 4 bits.
  byte write_leds = buf[0] & 0x80;
  //Serial.printf("neopixel: cmd %02X write_leds %02X\r\n", cmd, write_leds);
  //for (int i = 0; i < s; i++) Serial.printf("%02X ", buf[i]);
  //Serial.printf("\n");
  if (cmd == FILL) {
    for (int i = 0; i < num_pixels; i++) {
      strip->setPixelColor(i, buf[1], buf[2], buf[3]);
    }
  } else if (cmd == ZERO) {
    for (int i = 0; i < num_pixels; i++) {
      strip->setPixelColor(i, 0, 0, 0);
    }
  } else if (cmd == SET) {
    // [SET][nr_leds][start_led][r0,g0,b0][r1,g1,b1][r2,g2,b2][r3,g3,b3][0]
    byte nr_leds = buf[1];
    byte start_led = buf[2];
    //Serial.printf("Neopixel SET %d %d num_pixels %d\r\n", nr_leds, start_led, num_pixels);
    if ((nr_leds == 0) or (nr_leds > 4)) nr_leds = 4;
    if (start_led + nr_leds <= num_pixels) {
      for (int i = 0; i < nr_leds; i++) {
        // Serial.printf("set_led: %d %d %d %d \r\n",i+start_led, buf[3+i*3], buf[4+i*3], buf[5+i*3]);
        strip->setPixelColor(i + start_led, buf[3 + i * 3], buf[4 + i * 3], buf[5 + i * 3]);
      }
    }
  } else if (cmd == CONFIG) {
    delete strip;
    strip = new Adafruit_NeoPixel(buf[1], buf[2]);  //nr_leds, pin
  }
  if (write_leds & WRITE) {
    // Serial.println("Write leds");
    strip->show();
  }
}


void pybricks_servo_callback(byte buf[], byte s) {
  byte nr_short = int(s / 2);
  short vals[nr_short];
  //Serial.printf("size %d, nr short %d\n", s, nr_short);
  for (int i = 0; i < nr_short; i++) {
    vals[i] = buf[i * 2] + buf[i * 2 + 1] * 256;  // in Block SPIKE language only up to 128 can be used:  vals[i]=buf[i*2]+buf[i*2+1]*128;
    //Serial.printf("vals[%d]=%d\n", i, vals[i]);
  }

  //Serial.printf("servo %d %d %d %d\n", vals[0], vals[1], vals[2], vals[3]);
  servo1.write(vals[0]);
  servo2.write(vals[1]);
  servo3.write(vals[2]);
  servo4.write(vals[3]);
}



void add_mac_to_allow_list(byte bt_mac[6]) {
    char mac_addr[40];
    sprintf(mac_addr,"recv %02X:%02X:%02X:%02X:%02X:%02X",
          bt_mac[0],bt_mac[1],bt_mac[2],bt_mac[3],bt_mac[4],bt_mac[5]);
          Serial.println(mac_addr);
    if (uni_bt_allowlist_is_allowed_addr(bt_mac)) {
        sprintf(mac_addr,"in allow %02X:%02X:%02X:%02X:%02X:%02X",
          bt_mac[0],bt_mac[1],bt_mac[2],bt_mac[3],bt_mac[4],bt_mac[5]);
          Serial.println(mac_addr);
    } else {
      uni_bt_allowlist_remove_all();
      uni_bt_allowlist_add_addr(bt_mac);
       sprintf(mac_addr,"adding %02X:%02X:%02X:%02X:%02X:%02X",
          bt_mac[0],bt_mac[1],bt_mac[2],bt_mac[3],bt_mac[4],bt_mac[5]);
          Serial.println(mac_addr);
    }
}



// get bluetooth address of gamepad with index idx
/*
  // ur.call('gamepad','B',idx)
*/
void read_btaddress() {
  if (myGamepads[0] != nullptr) {  // check whether the gamepad entry exists
    GamepadPtr myGamepad = myGamepads[0];
    GamepadProperties properties = myGamepad->getProperties();
    char btaddr_str[18];  // Allocate space for the formatted string (six 2-digit hexadecimal numbers plus five colons plus '\x00')
    sprintf(btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
            properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
    sensor_conf.bt_mac[0] = properties.btaddr[0];
    sensor_conf.bt_mac[1] = properties.btaddr[1];
    sensor_conf.bt_mac[2] = properties.btaddr[2];
    sensor_conf.bt_mac[3] = properties.btaddr[3];
    sensor_conf.bt_mac[4] = properties.btaddr[4];
    sensor_conf.bt_mac[5] = properties.btaddr[5];
  } else {
    sensor_conf.bt_mac[0] = 0;
    sensor_conf.bt_mac[1] = 0;
    sensor_conf.bt_mac[2] = 0;
    sensor_conf.bt_mac[3] = 0;
    sensor_conf.bt_mac[4] = 0;
    sensor_conf.bt_mac[5] = 0;
  }
}



byte cmd_sensor(byte sensor_id) {
  if (sensor_id == 0x40 || sensor_id == 0x3d) {
    sensor_conf.sensor_id = sensor_id;
    return 1;
  } else {
    return 0;
  }
}

byte cmd_np_nr(byte np_nr) {
  if (np_nr > 0 && np_nr < 64) {
    sensor_conf.neopixel_nrleds = np_nr;
    neopixel_strip->clear();
    neopixel_strip->show();
    delete neopixel_strip;
    neopixel_strip = new Adafruit_NeoPixel(sensor_conf.neopixel_nrleds, sensor_conf.neopixel_gpio);  //nr_leds, pin

    return 1;
  } else {
    return 0;
  }
}

byte cmd_np_gpio(byte np_gpio) {
  if (np_gpio >= 0 && np_gpio < 33) {
    sensor_conf.neopixel_gpio = np_gpio;
    delete neopixel_strip;
    neopixel_strip = new Adafruit_NeoPixel(sensor_conf.neopixel_nrleds, sensor_conf.neopixel_gpio);  //nr_leds, pin

    return 1;
  } else {
    return 0;
  }
}

byte cmd_map(byte lego_led_nr, byte b1, byte b2, byte b3, byte b4, byte b5, byte b6, byte b7, byte b8) {
  //Serial.println((String)"lego_led_nr="+lego_led_nr+" np_led_nr="+np_led_nr);
  if (lego_led_nr >= 0 && lego_led_nr < 11) {
    sensor_conf.led_mapping[lego_led_nr][0] = b1;
    sensor_conf.led_mapping[lego_led_nr][1] = b2;
    sensor_conf.led_mapping[lego_led_nr][2] = b3;
    sensor_conf.led_mapping[lego_led_nr][3] = b4;
    sensor_conf.led_mapping[lego_led_nr][4] = b5;
    sensor_conf.led_mapping[lego_led_nr][5] = b6;
    sensor_conf.led_mapping[lego_led_nr][6] = b7;
    sensor_conf.led_mapping[lego_led_nr][7] = b8;
    return 1;
  } else {
    return 0;
  }
}

byte cmd_set_bt(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5) {
  //Serial.println((String)"lego_led_nr="+lego_led_nr+" np_led_nr="+np_led_nr);
  sensor_conf.bt_allow[0] = b0;
  sensor_conf.bt_allow[1] = b1;
  sensor_conf.bt_allow[2] = b2;
  sensor_conf.bt_allow[3] = b3;
  sensor_conf.bt_allow[4] = b4;
  sensor_conf.bt_allow[5] = b5;
  //char bt_addr_str[18];
  //sprintf(bt_addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",b0,b1,b2,b3,b4,b5);
  add_mac_to_allow_list(sensor_conf.bt_allow);
  // uni_bt_allowlist_add_addr(sensor_conf.bt_allow);  
  return 1;
}

void cmd_get_bt_in_allow_list(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5) {
  byte mac[6];
  mac[0]=b0; mac[1]=b1; mac[2]=b2; mac[3]=b3; mac[4]=b4; mac[5]=b5;
  if (uni_bt_allowlist_is_allowed_addr(mac)) {
    char bt_addr_str[18];
    sprintf(bt_addr_str, "%02X:%02X:%02X:%02X:%02X:%02X",b0,b1,b2,b3,b4,b5);
    Serial.println((String)bt_addr_str+" is in allow list");
  }
}


byte cmd_set_bt_filter(byte filter) {
  sensor_conf.bt_filter = filter;
  if (filter==0) {
    if (uni_bt_allowlist_is_enabled()) {
      uni_bt_allowlist_set_enabled(false);
    }
  } else if (filter==1) {
    if (!uni_bt_allowlist_is_enabled()) {
      uni_bt_allowlist_set_enabled(true);
    }
  }
  return 1;
}

byte cmd_color(byte lego_col_nr, byte r, byte g, byte b) {
  //Serial.println("---------- entering cmd_color---------");
  if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && lego_col_nr >= 0 && lego_col_nr <= 10) {
    sensor_conf.lego_colors[lego_col_nr][0] = r;
    sensor_conf.lego_colors[lego_col_nr][1] = g;
    sensor_conf.lego_colors[lego_col_nr][2] = b;
    return 1;
  } else {
    return 0;
  }
}

void functShow(CmdParser *myParser) {
  Serial.println((String) "magic: " + sensor_conf.magic);
  Serial.println((String) "sensor: " + sensor_conf.sensor_id);
  Serial.println((String) "neopixel_nrleds: " + sensor_conf.neopixel_nrleds);
  Serial.println((String) "neopixel_gpio: " + sensor_conf.neopixel_gpio);
  Serial.println("mapping: ");
  for (int i = 0; i < 9; i++) {
    Serial.print((String)i + " ");
    for (int j = 0; j < 8; j++) {
      Serial.print((String)sensor_conf.led_mapping[i][j] + " ");
    }
    Serial.println();
  }
  //Serial.println();
  for (int i = 0; i < 11; i++) {
    Serial.print((String) "color " + i + " ");
    for (int j = 0; j < 3; j++) {
      Serial.print((String)sensor_conf.lego_colors[i][j] + " ");
    }
    Serial.println();
  }
  Serial.println((String) "bt_filter: " + sensor_conf.bt_filter);
  Serial.print("bt_allow: ");
  for (int i = 0; i < 6; i++) {
    Serial.print((String)sensor_conf.bt_allow[i] + " ");
  }
  Serial.println();
  read_btaddress();
  Serial.printf("bt_mac: %d %d %d %d %d %d \r\n", sensor_conf.bt_mac[0], sensor_conf.bt_mac[1], sensor_conf.bt_mac[2],
                sensor_conf.bt_mac[3], sensor_conf.bt_mac[4], sensor_conf.bt_mac[5]);
  //Serial.println((String) "bt_allow_new: " + sensor_conf.bt_allow_new);
  //Serial.println((String) "bt_forget: " + sensor_conf.bt_forget);
  Serial.println("OK");
}

void functResponse(CmdParser *myParser) {
  if (myParser->equalCmdParam(1, "1")) {
    response = 1;
    myBuffer.setEcho(true);
  } else {
    response = 0;
    myBuffer.setEcho(false);
  }
  Serial.println("OK");
}

void functEeprom(CmdParser *myParser) {
  if (myParser->equalCmdParam(1, "READ")) {
    byte eeprom[256];
    EEPROM.get(0, eeprom);
    Serial.println("EEPROM content");
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        Serial.printf("%02X ", eeprom[i * 16 + j]);
      }
      Serial.println();
    }
    //Serial.println("hallo");
  }
  if (myParser->equalCmdParam(1, "GET")) {
    EEPROM.get(0, sensor_conf);
    Serial.println((String) "magic (string): " + sensor_conf.magic);
    Serial.printf("magic: %s\r\n", sensor_conf.magic);
    Serial.printf("strcmp: %d\r\n", strcmp(sensor_conf.magic, strMAGIC));
    Serial.printf("memcmp: %d\r\n", memcmp(sensor_conf.magic, strMAGIC, 6));
  }
  Serial.println("OK");
  if (myParser->equalCmdParam(1, "CLEAR")) {
    strcpy(sensor_conf.magic, "      ");
    EEPROM.put(0, sensor_conf);
    EEPROM.commit();
    Serial.printf("eeprom cleared");
  }
  Serial.println("OK");
}


void functNeopixel(CmdParser *myParser) {
  if (myParser->equalCmdParam(1, "CLEAR")) {
    neopixel_strip->clear();
    neopixel_strip->show();
  } else if (myParser->equalCmdParam(1, "SET")) {
    neopixel_strip->setPixelColor(atoi(myParser->getCmdParam(2)),
                                  atoi(myParser->getCmdParam(3)),
                                  atoi(myParser->getCmdParam(4)),
                                  atoi(myParser->getCmdParam(5)));
    neopixel_strip->show();
  }
  Serial.println("OK");
}
void functHelp(CmdParser *myParser) {
  Serial.println("Receive Help");
  Serial.println("set sensor  <id>                    : set sensor id");
  Serial.println("set np_nr   <nr>                    : sets number of neopixels to <nr>");
  Serial.println("set np_gpio <gpio>                  : sets GPIO of neopixels to <gpio>");
  Serial.println("set map     <lego_led> <b1..b4>     : maps lego led number <lego_led> to neopixel bitmask <b1..b4>");
  Serial.println("set color   <lego_col>  <r> <g> <b> : sets Lego color number <lego_col> to RGB <r,g,b> with r,g,b<=255");
  Serial.println("set bt_allow <b0> .. <b5>           : set allow bt mac address <b0:b1..:b5>");
  Serial.println("set bt_filter <0/1>                 : set filtering of BT on or off");
  Serial.println("set bt_forget                       : forget old paired gamepads");
  Serial.println("set bt_clear_allow_list             : remove all items from allow list");
  Serial.println("set bt_allow_new                    : falow new gamepads to pair");
  Serial.println("get bt_allow                        : get alow bt mac address: <b0:b1..:b5>");


  Serial.println("get bt_filter                       : get bt_filter setting: 0 or 1");
  Serial.println("get bt_mac                          : get paired bt mac address returns <b0> <b1> .. <b5>");
  Serial.println("get bt_con                          : get bt connection state <0>=not <1> is connected");
  Serial.println("get bt_allow_list                   : print bt allow list");
  Serial.println("get bt_in_allow_list                : print bt allow list");

  Serial.println("show                                : prints the sensor configuration");
  Serial.println("save                                : saves paramaters to flash");
  Serial.println("default                             : sets all parameters to default");
  Serial.println("response <resp>                     : sets response off <resp>=0 or on <resp>=1");
  Serial.println("eeprom read                         : reads 100 bytes of EEPROM");
  Serial.println("eeprom get                          : gets configuration from eeprom");
  Serial.println("eeprom clear                        : clear configuration from eeprom");
  Serial.println("neopixel clear                      : clears all neopixels");
  Serial.println("neopixel set <np_nr> <r> <g> <r>    : sets neopixel number <np_nr> to color <r>,<g>,<b>");
}

void functSave(CmdParser *myParser) {
  if (response) Serial.println("Received Save");
  EEPROM.put(0, sensor_conf);
  EEPROM.commit();
  Serial.println("OK");
}


void functDefault(CmdParser *myParser) {
  if (response) Serial.println("Receive Default");
  strcpy(sensor_conf.magic, "LMSESP");
  sensor_conf.sensor_id = DEFAULT_SENSOR;
  sensor_conf.neopixel_gpio = LED_PIN;
  sensor_conf.neopixel_nrleds = LED_COUNT;
  memcpy(sensor_conf.lego_colors, lego_colors, sizeof(lego_colors));
  memcpy(sensor_conf.led_mapping, led_mapping, sizeof(led_mapping));
  sensor_conf.bt_filter = 0;
  //sensor_conf.bt_allow_new = 1;
  //sensor_conf.bt_forget = 0;
  memset(sensor_conf.bt_allow, 0, 6);
  memset(sensor_conf.bt_mac, 0, 6);
  uni_bt_allowlist_remove_all();
  uni_bt_allowlist_set_enabled(false);
  Serial.println("OK");
}


void functSet(CmdParser *myParser) {
  if (myParser->equalCmdParam(1, "SENSOR")) {
    if (cmd_sensor(atoi(myParser->getCmdParam(2)))) {
      Serial.println("OK");
    } else {
      Serial.println("ERROR");
    }
  } else if (myParser->equalCmdParam(1, "NP_NR")) {
    if (cmd_np_nr(atoi(myParser->getCmdParam(2)))) {
      Serial.println("OK");
    } else {
      Serial.println("ERROR");
    }
  } else if (myParser->equalCmdParam(1, "NP_GPIO")) {
    if (cmd_np_gpio(atoi(myParser->getCmdParam(2)))) {
      Serial.println('OK');
    } else {
      Serial.println('ERROR');
    }
  } else if (myParser->equalCmdParam(1, "MAP")) {
    if (cmd_map(atoi(myParser->getCmdParam(2)),
                atoi(myParser->getCmdParam(3)),
                atoi(myParser->getCmdParam(4)),
                atoi(myParser->getCmdParam(5)),
                atoi(myParser->getCmdParam(6)),
                atoi(myParser->getCmdParam(7)),
                atoi(myParser->getCmdParam(8)),
                atoi(myParser->getCmdParam(9)),
                atoi(myParser->getCmdParam(10)))) {
      Serial.println("OK");
    } else {
      Serial.println("ERROR");
    }
/*  } else if (myParser->equalCmdParam(1, "BT_ALLOW_NEW")) {
    sensor_conf.bt_allow_new = atoi(myParser->getCmdParam(2));
    BP32.enableNewBluetoothConnections(sensor_conf.bt_allow_new);

  } else if (myParser->equalCmdParam(1, "BT_FORGET")) {
    sensor_conf.bt_forget = atoi(myParser->getCmdParam(2));
    if (sensor_conf.bt_forget) {

      BP32.forgetBluetoothKeys();
    } */
  } else if (myParser->equalCmdParam(1, "COLOR")) {
    if (cmd_color(atoi(myParser->getCmdParam(2)),
                  atoi(myParser->getCmdParam(3)),
                  atoi(myParser->getCmdParam(4)),
                  atoi(myParser->getCmdParam(5)))) {
      Serial.println("OK");
    } else {
      Serial.println("ERROR");
    }
  } else if (myParser->equalCmdParam(1, "BT_ALLOW")) {
    if (cmd_set_bt(atoi(myParser->getCmdParam(2)),
                   atoi(myParser->getCmdParam(3)),
                   atoi(myParser->getCmdParam(4)),
                   atoi(myParser->getCmdParam(5)),
                   atoi(myParser->getCmdParam(6)),
                   atoi(myParser->getCmdParam(7)))) {
      Serial.println("OK");
    } else {
      Serial.println("ERROR");
    }
  } else if (myParser->equalCmdParam(1, "BT_FILTER")) {
    cmd_set_bt_filter(atoi(myParser->getCmdParam(2)));
  } else if (myParser->equalCmdParam(1, "BT_CLEAR_ALLOW_LIST")) {
    uni_bt_allowlist_remove_all();
    Serial.println("allowlist removed");
  }

  if (response) Serial.println("Receive Set");
}

void functGet(CmdParser *myParser) {
  if (myParser->equalCmdParam(1, "BT_MAC")) {
    read_btaddress();
    Serial.printf("bt_mac: %d %d %d %d %d %d\r\n", sensor_conf.bt_mac[0], sensor_conf.bt_mac[1], sensor_conf.bt_mac[2],
                  sensor_conf.bt_mac[3], sensor_conf.bt_mac[4], sensor_conf.bt_mac[5]);

  } else if (myParser->equalCmdParam(1, "BT_IN_ALLOW_LIST")) {
    cmd_get_bt_in_allow_list(atoi(myParser->getCmdParam(2)),
                   atoi(myParser->getCmdParam(3)),
                   atoi(myParser->getCmdParam(4)),
                   atoi(myParser->getCmdParam(5)),
                   atoi(myParser->getCmdParam(6)),
                   atoi(myParser->getCmdParam(7))); 
  } else if (myParser->equalCmdParam(1, "BT_ALLOW")) {
    Serial.printf("bt_allow: %d %d %d %d %d %d\r\n", sensor_conf.bt_allow[0], sensor_conf.bt_allow[1], sensor_conf.bt_allow[2],
                  sensor_conf.bt_allow[3], sensor_conf.bt_allow[4], sensor_conf.bt_allow[5]);
  } else if (myParser->equalCmdParam(1, "BT_FILTER")) {
    Serial.printf("bt_filter: %d\r\n", sensor_conf.bt_filter);
  } else if (myParser->equalCmdParam(1, "BT_CON")) {
    GamepadPtr myGamepad = myGamepads[0];
    int con = 0;
    if (myGamepad && myGamepad->isConnected())
      con = 1;
    else
      con = 0;
    Serial.printf("bt_con: %d\r\n", con);
  } else if (myParser->equalCmdParam(1, "BT_ALLOW_LIST")) {
    const bd_addr_t* addr; 
    int nr;
    char mac_addr[17];
    Serial.println("allowed mac addresses:");
    uni_bt_allowlist_get_all(&addr, &nr);
    Serial.println((String)"nr="+nr);
    for (int i=0; i<nr; i++) {
      sprintf(mac_addr,"%02X:%02X:%02X:%02X:%02X:%02X",
          addr[i][0],addr[i][1],addr[i][2],addr[i][3],addr[i][4],addr[i][5]);
      Serial.println(mac_addr);
    }

  }


}


// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedGamepad(GamepadPtr gp) {
  bool foundEmptySlot = false;
  if (myGamepads[0] == nullptr && !foundEmptySlot) {
    Serial.printf("CALLBACK: Gamepad is connected\n");
    // Additionally, you can get certain gamepad properties like:
    // Model, VID, PID, BTAddr, flags, etc.
    GamepadProperties properties = gp->getProperties();
    char btaddr_str[18];                                                                                                                                                                       // Allocate space for the formatted string (six 2-digit hexadecimal numbers plus five colons)
    sprintf(btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);  // Format the address as a string


    Serial.printf("Gamepad model: %s, VID=0x%04x, PID=0x%04x  bt_addr=%s\r\n",
                  gp->getModelName().c_str(), properties.vendor_id,
                  properties.product_id, btaddr_str);
    read_btaddress();
 /*   if (sensor_conf.bt_filter) {
      Serial.printf("received: %02X:%02X:%02X:%02X:%02X:%02X ", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
      Serial.printf("filtered: %02X:%02X:%02X:%02X:%02X:%02X\r\n", sensor_conf.bt_allow[0], sensor_conf.bt_allow[1],
                    sensor_conf.bt_allow[2], sensor_conf.bt_allow[3],
                    sensor_conf.bt_allow[4], sensor_conf.bt_allow[5]);
      if (memcmp(sensor_conf.bt_allow, properties.btaddr, 6) == 0) {  // bt_allow
        myGamepads[0] = gp;
        foundEmptySlot = true;
        Serial.printf("bt_filtered: allowed\r\n");
      } else {
        // gp->disconnect();
        Serial.printf("bt_filtered: gamepad not connected\r\n");
      }

    } else { */
      Serial.printf("un_filtered: connected\r\n");
      myGamepads[0] = gp;
      foundEmptySlot = true;
    //}
  }
  if (!foundEmptySlot) {
    Serial.println(
      "CALLBACK: Gamepad connected, but could not found empty slot");
  } else {
    if (is_lms_esp32_version2 == 1) {
      byte status = lpf2_sensor->get_status();
      neopixel_debug->setPixelColor(0, 10 - 10 * status, 10 * status, 10);
      neopixel_debug->show();
    }
  }
}

void onDisconnectedGamepad(GamepadPtr gp) {
  bool foundGamepad = false;

  if (myGamepads[0] == gp) {
    Serial.printf("CALLBACK: Gamepad is disconnected\r\n");
    myGamepads[0] = nullptr;
    foundGamepad = true;
    if (is_lms_esp32_version2 == 1) {
      byte status = lpf2_sensor->get_status();
      neopixel_debug->setPixelColor(0, 10 - 10 * status, 10 * status, 0);
      neopixel_debug->show();
    }
  }

  if (!foundGamepad) {
    Serial.println(
      "CALLBACK: Gamepad disconnected, but not found in myGamepads");
  }
}






uint8_t old_led = 0, rumble_force, rumble_duration;



//byte colors[11][3] = {{0,0,0},{200,200,255},{255,0,255},{0,0,255},{0,255,255},{0,255,150},{0,255,0},{255,255,0},{255,140,0},{255,0,0},{255,255,255}};
//byte pos[9]={3,4,5,11,12,13,19,20,21};
byte old_values[9] = {};

void neopixel_callback(byte buf[], byte s) {
  // only write to neopixels if something has changed
  if (memcmp(old_values, buf, 9) != 0) {
    memcpy(old_values, buf, 9);

    // Serial.println();

    for (int i = 0; i < 9; i++) {
      byte bright = buf[i] / 16;
      byte colr = int(sensor_conf.lego_colors[buf[i] % 16][0] / 10.0 * bright);
      //                                              normilse brightness to 10
      byte colg = int(sensor_conf.lego_colors[buf[i] % 16][1] / 10.0 * bright);
      byte colb = int(sensor_conf.lego_colors[buf[i] % 16][2] / 10.0 * bright);
      // Serial.printf("led %d,val=0x%02x,b=%d,c=%d (%d,%d,%d)\n", i, buf[i], bright, buf[i] % 16, colr, colg, colb);
      for (int j = 0; j < 8; j++) {
        uint8_t b = sensor_conf.led_mapping[i][j];
        //printf("byte = %d\n",b);
        for (int k = 0; k < 8; k++) {
          if (b & 1) {
            //printf("neopixel: %d %d %d %d\n",j*8+k,colr, colg, colb);
            neopixel_strip->setPixelColor(j * 8 + k, colr, colg, colb);
          }
          b >>= 1;
        }
      }
      //neopixel_strip->setPixelColor(sensor_conf.led_mapping[i][0], colr, colg, colb);
    }
    neopixel_strip->show();
  }
}


void config_sensor() {
  if (sensor_conf.sensor_id == COLOR_SENSOR) {
    delete lpf2_sensor;
    lpf2_sensor = new EV3UARTEmulation(RXD2, TXD2, COLOR_SENSOR, 115200);
    sensor_conf.sensor_id = COLOR_SENSOR;
    lpf2_sensor->create_mode("COLOR\x00\x80\x00\x00\x00\x05\x04", true, DATA16, 8, 2, 0, -100.0f, 100.0f, -100.0f, 100.0f, -100.0f, 100.0f, "PCT", ABSOLUTE, ABSOLUTE);  //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("REFLT\x00\x80\x00\x00\x00\x05\x04", true, DATA16, 8, 3, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "RAW", ABSOLUTE, ABSOLUTE);           //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("AMBI\x00\x80\x00\x00\x00\x05\x04", true, DATA8, 1, 3, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE);            //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("LIGHT\x00\x80\x00\x00\x00\x05\x04", true, DATA8, 3, 3, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE);           //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("RREFL\x00\x80\x00\x00\x00\x05\x04", true, DATA8, 2, 4, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE);           //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("RGB I\x00\x80\x00\x00\x00\x05\x04", true, DATA16, 4, 4, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE);          //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("HSV\x00\x80\x00\x00\x00\x05\x04", true, DATA16, 3, 4, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE);            //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("SHSV\x00\x80\x00\x00\x00\x05\x04", true, DATA16, 4, 4, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE);           //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->get_mode(0)->setCallback(pybricks_neopixel_callback);                                                                                                   // attach call back function to mode 0
    lpf2_sensor->get_mode(1)->setCallback(pybricks_servo_callback);                                                                                                      // attach call back function to mode 1

    Serial.printf("LOG: This sensor is configured as Color Sensor\r\n");
  } else {
    sensor_conf.sensor_id = COLOR_MATRIX;
    delete lpf2_sensor;
    lpf2_sensor = new EV3UARTEmulation(RXD2, TXD2, COLOR_MATRIX, 115200);

    lpf2_sensor->create_mode("LEV O\x00\x80\x00\x00\x00\x05\x04", true, DATA8, 9, 5, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "RAW", ABSOLUTE, ABSOLUTE);  //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("COL O\x00\x80\x00\x00\x00\x05\x04", true, DATA8, 9, 5, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "RAW", ABSOLUTE, ABSOLUTE);  //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("PIX O\x00\x80\x00\x00\x00\x05\x04", true, DATA8, 9, 3, 0, 0.0f, 170.0f, 0.0f, 100.0f, 0.0f, 170.0f, "   ", 0, 0x10);             //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->create_mode("TRANS\x00\x80\x00\x00\x00\x05\x04", true, DATA8, 1, 1, 0, 0.0f, 2.0f, 0.0f, 100.0f, 0.0f, 2.0f, "   ", 0, 0x10);                 //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    lpf2_sensor->get_mode(0)->setCallback(pybricks_neopixel_callback);                                                                                         // attach call back function to mode 0
    lpf2_sensor->get_mode(1)->setCallback(pybricks_servo_callback);                                                                                            // attach call back function to mode 1
    lpf2_sensor->get_mode(2)->setCallback(neopixel_callback);                                                                                                  // attach call back function to mode 0

    Serial.printf("LOG: This sensor is configured as Color Matrix\r\n");
    Serial.printf("LOG: NeoPixel on GPIO %d\r\n", sensor_conf.neopixel_gpio);
    Serial.printf("LOG: NeoPixel has %d leds\r\n", sensor_conf.neopixel_nrleds);
    // reinit neopixel strip
    neopixel_strip->clear();
    neopixel_strip->show();
    delete neopixel_strip;
    neopixel_strip = new Adafruit_NeoPixel(sensor_conf.neopixel_nrleds, sensor_conf.neopixel_gpio);  //nr_leds, pin
    // for (int i = 0; i < 9; i++) { Serial.printf("map[%d]=%d\n", i, sensor_conf.led_mapping[i]); }
    // for (int i = 0; i < 11; i++) {
    //   Serial.printf("color %d=",i);
    //   for (int j = 0; j < 3; j++) {
    //     Serial.printf("%d ", sensor_conf.lego_colors[i][j]);
    //   }
    //   for (int j = 0; j < 3; j++) {
    //     Serial.printf("%d ", lego_colors[i][j]);
    //   }
    //   Serial.println();
    // }
  }

}



// Arduino setup function. Runs in CPU 1
void setup() {
  // disable brownout
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // default mapping:
  if (strcmp(ESP.getChipModel(), "ESP32-PICO-V3-02") == 0) {
    is_lms_esp32_version2 = 1;
    RXD2 = 8;
    TXD2 = 7;
    servo1Pin = 19;
    servo2Pin = 20;
    servo3Pin = 21;
    servo4Pin = 22;
    neopixel_debug->setPixelColor(0, 10, 0, 0);
    neopixel_debug->show();
  } else {
    is_lms_esp32_version2 = 0;
    RXD2 = 18;
    TXD2 = 19;
  }

  Serial.begin(115200);
  Serial.setRxBufferSize(1000);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, sensor_conf);
  // if EEPROM is not already initialized, do so with default values
  if (strcmp(sensor_conf.magic, strMAGIC) != 0) {
    Serial.println("LOG: storing default values in EEprom");
    functDefault(0);
    EEPROM.put(0, sensor_conf);
    EEPROM.commit();
  }

  cmdCallback.addCmd(strHelp, &functHelp);
  cmdCallback.addCmd(strResponse, &functResponse);
  cmdCallback.addCmd(strSet, &functSet);
  cmdCallback.addCmd(strGet, &functGet);
  cmdCallback.addCmd(strShow, &functShow);
  cmdCallback.addCmd(strSave, &functSave);
  cmdCallback.addCmd(strDefault, &functDefault);
  cmdCallback.addCmd(strNeopixel, &functNeopixel);
  cmdCallback.addCmd(strEeprom, &functEeprom);

  myBuffer.setEndChar('\x0d');  // CR instead of LF
  if (response) {
    myBuffer.setEcho(true);
    Serial.println();
    Serial.println("Type you commands. Supported: ");
    //functHelp(0);
  } else {
    myBuffer.setEcho(false);
    Serial.println("OK");
  }

  // apply these values to the sensor
  config_sensor();
  uni_bt_allowlist_set_enabled(sensor_conf.bt_filter==1);
  add_mac_to_allow_list(sensor_conf.bt_allow);
  // uni_bt_allowlist_add_addr(sensor_conf.bt_allow);  

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo1.setPeriodHertz(50);  // Standard 50hz servo
  servo2.setPeriodHertz(50);  // Standard 50hz servo
  servo3.setPeriodHertz(50);  // Standard 50hz servo
  servo4.setPeriodHertz(50);  // Standard 50hz servo
  servo1.attach(servo1Pin, minUs, maxUs);
  servo2.attach(servo2Pin, minUs, maxUs);
  servo3.attach(servo3Pin, minUs, maxUs);
  servo4.attach(servo4Pin, minUs, maxUs);
  // sensor.get_mode(1)->setCallback(servo_callback);  // attach call back function to mode 1
  // //sensor.get_mode(2)->setCallback(i2c_callback);  // attach call back function to mode 1


  connected = lpf2_sensor->reset();
  if (connected == 1) {
    last_status = lpf2_sensor->get_status();
    if (is_lms_esp32_version2 == 1) {
      // green led
      neopixel_debug->setPixelColor(0, 0, 10, 0);
      neopixel_debug->show();
    }
    Serial.printf("LOG: LMS-ESP32 is connected to the SPIKE3 hub.\r\n");
  } else {
    Serial.printf("ERROR: Failure connecting LMS-ESP32 to the SPIKE3 hub. Press RESET button on LMS-ESP32.\r\n");
  }
  Serial.println(version);
  Serial.println((String) "Running on " + ESP.getChipModel());
  Serial.println("Go to https://bluepad.antonsmindstorms.com to configure the LMS-ESP32.\r\n");
  delay(200);
  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();



  Serial.println("LOG: BluePad32 for legacy Robot Inventor, SPIKEv2, SPIKEv3, and Pybricks see https://github.com/antonvh/PUPRemote/blob/main/examples/bluepad");
  Serial.printf("LOG: Firmware: %s\r\n", BP32.firmwareVersion());
  const uint8_t *addr = BP32.localBdAddress();
  Serial.printf("LOG: BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\r\n", addr[0], addr[1], addr[2],
                addr[3], addr[4], addr[5]);
}

int clip(int n, int lower, int upper) {
  return MAX(lower, MIN(n, upper));
}

int refresh_BP32 = 0;
// Arduino loop function. Runs in CPU 1
int update_Cmd = 0;

unsigned long last_reading = 0;
int last_mode = 0;

void loop() {
  byte status = lpf2_sensor->get_status();
  byte gamepad_connected = 0;
  if ((status != last_status) && (is_lms_esp32_version2 == 1)) {
    last_status = status;
    GamepadPtr myGamepad = myGamepads[0];
    if (myGamepad && myGamepad->isConnected())
      gamepad_connected = 1;
    neopixel_debug->setPixelColor(0, 10 - 10 * status, 10 * status, 10 * gamepad_connected);
    neopixel_debug->show();
  }
  refresh_BP32++;
  if (refresh_BP32 == 10) {
    BP32.update();
    refresh_BP32 = 0;
  }
  update_Cmd++;
  if (update_Cmd == 1) {
    cmdCallback.updateCmdProcessing(&myParser, &myBuffer, &Serial);
    update_Cmd = 0;
  }



  // This call fetches all the gamepad info from the NINA (ESP32) module.
  // Just call this function in your main loop.
  // The gamepads pointer (the ones received in the callbacks) gets updated
  // automatically.
  if (connected == 1) {

    lpf2_sensor->heart_beat();
    int mode = lpf2_sensor->get_current_mode();
    if (mode != last_mode) {
      //Serial.printf("new mode %d\r\n", mode);
      last_mode = mode;
    }
    if ((millis() - last_reading > 20)) {  //} || mode != last_mode) {
      //last_mode = mode;
      //if (mode != last_mode) last_mode = mode;
      // for any mode gamepad fields are populated
      // if (mode == 0) {
      if (sensor_conf.sensor_id == COLOR_MATRIX) {
        byte bb[16];
        int8_t a, b;
        memset(bb, 0, 16);
        //for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        GamepadPtr myGamepad = myGamepads[0];
        //Serial.print("#");
        if (myGamepad && myGamepad->isConnected()) {
          //if ((last_mode == 0) or (last_mode == 2)) {  // spke3
          if (1) {
            a = (clip((myGamepad->axisX() + 512), 0, 1023) >> 2);
            b = (clip((myGamepad->axisY() + 512), 0, 1023) >> 2);
            if (a > 127) {
              bb[0] = a - 128;
            } else {
              bb[0] = a + 128;
            }
            if (b > 127) {
              bb[1] = b - 128;
            } else {
              bb[1] = b + 128;
            }
            a = (clip((myGamepad->axisRX() + 512), 0, 1023) >> 2);
            b = (clip((myGamepad->axisRY() + 512), 0, 1023) >> 2);
            if (a > 127) {
              bb[2] = a - 128;
            } else {
              bb[2] = a + 128;
            }
            if (b > 127) {
              bb[3] = b - 128;
            } else {
              bb[3] = b + 128;
            }
            //Serial.printf("a=%d bb[2]=%d\n", a, bb[2]);
            //Serial.printf("b=%d bb[3]=%d\n", b, bb[3]);


            a = (myGamepad->buttons()) & 0xff;
            b = (myGamepad->dpad()) & 0xff;
            bb[4] = a;
            bb[5] = b;
          }  // mde==0
        }
        int nr_bytes = lpf2_sensor->get_mode(mode)->sample_size;
        //sensor.send_data8(bb, nr_bytes);
        lpf2_sensor->send_data8(bb, 16);  // always write 16 bytes (9 real bytes + padding, otherwise pybricks won't accept)
      } else {                            // COLOR SENSOR
        GamepadPtr myGamepad = myGamepads[0];
        if ((last_mode == 0) || (last_mode == 1)) {  // spike3
          short bb[16];
          byte a, b;
          memset(bb, 0, 16);
          if (myGamepad && myGamepad->isConnected()) {
            a = clip((myGamepad->axisX() + 512), 0, 1023) >> 2;
            b = clip((myGamepad->axisY() + 512), 0, 1023) >> 2;
            bb[0] = a + b * 256;
            a = clip((myGamepad->axisRX() + 512), 0, 1023) >> 2;
            b = clip((myGamepad->axisRY() + 512), 0, 1023) >> 2;
            bb[1] = a + b * 256;

            a = (myGamepad->buttons()) & 0xff;
            b = (myGamepad->dpad()) & 0xff;
            bb[2] = a + b * 256;
            bb[3] = bb[0];
            bb[4] = bb[1];
            bb[5] = bb[2];
            bb[6] = bb[0];
            bb[7] = bb[1];
            //for (int i=0; i<3; i++) {
            //  Serial.printf("bb[%d]=%X\n",i,bb[i]);
            // }
          }
          // Serial.printf("\n");
          int nr_bytes = lpf2_sensor->get_mode(mode)->sample_size;
          //sensor.send_data8(bb, nr_bytes);
          lpf2_sensor->send_data16((short *)bb, nr_bytes);
        }
      }


      last_reading = millis();
    }
    vTaskDelay(1);
  }
  vTaskDelay(1);
  //delay(150);
}
