#include <stdio.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <Bluepad32.h>

#include <UartRemote.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

#define MAX_GAMEPADS 1
// storage for gamepads
GamepadPtr myGamepads[MAX_GAMEPADS];

// initialize uartremote
UartRemote uartremote;

// default values for rx and tx uart for LMS-ESP32v1
byte RXD2 = 18;
byte TXD2 = 19;
byte is_lms_esp32_version2 = 0;  // filled by calling ESP.getChipModel()
char bt_allow[6] = { 0, 0, 0, 0, 0, 0 };
bool bt_filter = false;
int debug = 0;  // global debug; if 1 -> print debug.

#define LED_PIN 12
#define LED_COUNT 64

// use pointer allows to dynamically change nrumber of leds or pin
// change strip.begin() to strip->begin(), etc.
// delete object before initiating a new one
Adafruit_NeoPixel* strip = new Adafruit_NeoPixel(LED_COUNT, LED_PIN);  //, NEO_GRB + NEO_KHZ800);

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

char version[] = "BluePad32 UartRemote version 20250104";
// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedGamepad(GamepadPtr gp) {
  bool foundEmptySlot = false;
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    if (myGamepads[i] == nullptr && !foundEmptySlot) {
      Serial.printf("CALLBACK: Gamepad is connected, index=%d\n", i);
      // Additionally, you can get certain gamepad properties like:
      // Model, VID, PID, BTAddr, flags, etc.
      GamepadProperties properties = gp->getProperties();
      char btaddr_str[18];                                                                                                                                                                       // Allocate space for the formatted string (six 2-digit hexadecimal numbers plus five colons)
      sprintf(btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);  // Format the address as a string
      Serial.printf("\nGamepad model: %s, VID=0x%04x, PID=0x%04x  bt_addr=%s\n",
                    gp->getModelName().c_str(), properties.vendor_id,
                    properties.product_id, btaddr_str);
      // if bt_filter then check whether this gamepad is allowed to connect, if not, disconnect
      if (bt_filter) {
        if ((debug & 1) == 1) {
          Serial.printf("\nreceived: %02X:%02X:%02X:%02X:%02X:%02X ", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
          Serial.printf("filtered: %02X:%02X:%02X:%02X:%02X:%02X\n", bt_allow[0], bt_allow[1], bt_allow[2], bt_allow[3], bt_allow[4], bt_allow[5]);
        }
        if (memcmp(bt_allow, properties.btaddr, 6) == 0) {  // is bt_allow equal to  current btaddr that tries to connect
          myGamepads[i] = gp;
          foundEmptySlot = true;
          Serial.printf("\nbt_filtered: allowed\n");
        } else {
          gp->disconnect();
          Serial.printf("\nbt_filtered: gamepad disconnected\n");
        }
        break;
      } else {
        Serial.printf("\nun_filtered: connected\n");
        myGamepads[i] = gp;
        foundEmptySlot = true;
      }
    }
  }
  if (!foundEmptySlot) {
    Serial.println(
      "CALLBACK: Gamepad connected, but could not found empty slot");
  }
}

void onDisconnectedGamepad(GamepadPtr gp) {
  bool foundGamepad = false;

  for (int i = 0; i < MAX_GAMEPADS; i++) {
    if (myGamepads[i] == gp) {
      Serial.printf("CALLBACK: Gamepad is disconnected from index=%d\n", i);
      myGamepads[i] = nullptr;
      foundGamepad = true;
      break;
    }
  }

  if (!foundGamepad) {
    Serial.println(
      "CALLBACK: Gamepad disconnected, but not found in myGamepads");
  }
}


/*
  // ur.call('debug','B',1)
*/
void setdebug(Arguments args) {
  unpack(args, &debug);
  if (debug != 0) {
    Serial.printf("Debug enabled\n");
  } else {
    Serial.printf("Debug disabled\n");
  }
  uartremote.send_command("debugack", "B", 0);
}

void connected(Arguments args) {
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    GamepadPtr myGamepad = myGamepads[i];
    if (myGamepad && myGamepad->isConnected()) {
      uartremote.send_command("connectedack", "B", 1);
    } else {
      uartremote.send_command("connectedack", "B", 0);
    }
  }
}

// get bluetooth address of gamepad with index idx
/*
  // ur.call('gamepad','B',idx)
*/
void btaddress(Arguments args) {
  uint8_t idx;
  unpack(args, &idx);
  if ((idx >= 0) && (idx < MAX_GAMEPADS)) {
    if (myGamepads[idx] != nullptr) {  // check whether the gamepad entry exists
      GamepadPtr myGamepad = myGamepads[idx];
      GamepadProperties properties = myGamepad->getProperties();
      char btaddr_str[18];  // Allocate space for the formatted string (six 2-digit hexadecimal numbers plus five colons plus '\x00')
      sprintf(btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
              properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
      uartremote.send_command("btaddressack", "17s", btaddr_str);  // only 17, tailer '\x00' is not send
    } else {
      uartremote.send_command("btaddresserr", "B", 0);
    }
  } else {
    uartremote.send_command("btaddresserr", "B", 0);
  }
}

// disconnect gamepad with index idx
/*
  // ur.call('btdisconnect','B',idx)
*/

void btdisconnect(Arguments args) {
  uint8_t idx;
  unpack(args, &idx);
  if ((idx >= 0) && (idx < MAX_GAMEPADS)) {
    if (myGamepads[idx] != nullptr) {
      GamepadPtr myGamepad = myGamepads[idx];
      myGamepad->disconnect();
      uartremote.send_command("btdisconnectack", "B", 0);
    } else {
      uartremote.send_command("btdisconnecterr", "B", 0);
    }
  } else {
    uartremote.send_command("btdisconnecterr", "B", 0);
  }
}

// calls BT32.forgetBluetoothKeys();
/*
  // ur.call('btforget')
*/
void btforget(Arguments args) {
  BP32.forgetBluetoothKeys();
  uartremote.send_command("btforget", "B", 0);
}

// defines the bluetooth adddress of allowed gamepad; must set btfilter seperately
/*
   ur.call('btallow','17s',b'AA:BB:CC:DD:EE:FF'))
   ur.call('bifiltered','B',1)
*/
void btallow(Arguments args) {
  char btaddr_str[18];
  unpack(args, btaddr_str);
  sscanf(btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", bt_allow, bt_allow + 1, bt_allow + 2, bt_allow + 3, bt_allow + 4, bt_allow + 5);
  Serial.printf("btallow: %02X:%02X:%02X:%02X:%02X:%02X  filtered:%d\n", bt_allow[0], bt_allow[1], bt_allow[2], bt_allow[3], bt_allow[4], bt_allow[5], bt_filter);
  uartremote.send_command("allowack", "B", 0);
}

// enables filtering on bluetooth address
/*
   ur.call('btallow','17s',b'AA:BB:CC:DD:EE:FF'))
   ur.call('bifiltered','B',1)
*/
void btfilter(Arguments args) {
  uint8_t filtered;
  unpack(args, &filtered);
  bt_filter = (filtered == 1);
  Serial.printf("filtered:%d\n", bt_filter);
  uartremote.send_command("btfilterack", "B", 0);
}

// returns gamepad readings in format (left_x,left_y,right_x,right_y,keys,dpad)
/*
   (ack,readings)=ur.call('gmepad)
   (left_x,left_y,right_x,right_y,keys,dpad)=readings
*/
void gamepad(Arguments args) {
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    GamepadPtr myGamepad = myGamepads[i];
    if (myGamepad && myGamepad->isConnected()) {
      uartremote.send_command("gamepadack", "2H4h", myGamepad->buttons(), myGamepad->dpad(),
                              myGamepad->axisX(), myGamepad->axisY(),
                              myGamepad->axisRX(), myGamepad->axisRY());
    } else {
      uartremote.send_command("gamepadack", "2H4h", 0, 0, 0, 0, 0, 0);
    }
  }
}

// the imu should be activated only on wii-mote controllers when pressing <A> while connecting
// This does not seem to work

void imu(Arguments args) {
  for (int i = 0; i < MAX_GAMEPADS; i++) {
    GamepadPtr myGamepad = myGamepads[i];
    if (myGamepad && myGamepad->isConnected()) {
      uartremote.send_command("imuack", "6I",
                              myGamepad->gyroX(),   // Gyro X
                              myGamepad->gyroY(),   // Gyro Y
                              myGamepad->gyroZ(),   // Gyro Z
                              myGamepad->accelX(),  // Accelerometer X
                              myGamepad->accelY(),  // Accelerometer Y
                              myGamepad->accelZ()   // Accelerometer Z
      );
    } else {
      uartremote.send_command("imuerr", "6I", 0, 0, 0, 0, 0, 0);
    }
  }
}

// returns the I2C device addresses that are connected to I2C port with sda=pin(5), scl=Pin(4)
uint8_t scan_i2c(uint8_t addresses[]) {
  byte error, address;  //variable for error and I2C address
  int nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      addresses[nDevices] = address;
      nDevices++;
    }
  }

  return nDevices;
}

/*
   ur.call('i2c_scan')
*/
void i2c_scan(Arguments args) {
  uint8_t addresses[128], nDevices;
  nDevices = scan_i2c(addresses);
  char format[7] = {};
  sprintf(format, "B%ds", nDevices);  // create variable format string
  uartremote.send_command("i2c_scanack", format, nDevices, addresses);
}


void i2c_read(Arguments args) {
  /*
    call("i2c_read","2B",address,len)
  */
  uint8_t address, len;
  uint8_t buf[128];
  char format[7] = {};
  unpack(args, &address, &len);
  Wire.requestFrom(address, len);  // request 6 bytes from slave device #2
  for (int i = 0; i < len; i++) {
    char c = Wire.read();  // receive a byte as character
    buf[i] = c;
  }

  sprintf(format, "%ds", len);  // create variable format string
  uartremote.send_command("i2c_readack", format, buf);
}

void i2c_read_reg(Arguments args) {
  /*
    call("i2c_read_reg","2B",address,reg,len)

    returns: bytes: buf
  */

  uint8_t address, len, reg;
  uint8_t buf[128];
  char format[7] = {};
  unpack(args, &address, &reg, &len);
  Wire.beginTransmission(address);  // Get the slave's attention, tell it we're sending a command byte
  Wire.write(reg);                  //  The command byte, sets pointer to register with address of 0x32
  Wire.requestFrom(address, len);   // Tell slave we need to read 1byte from the current register
  for (int i = 0; i < len; i++) {
    char c = Wire.read();  // receive a byte as character
    buf[i] = c;
  }
  Wire.endTransmission();
  sprintf(format, "%ds", len);  // create variable format string
  uartremote.send_command("i2c_read_regack", format, buf);
}

void neopixel(Arguments args) {
  /*
    call("neopixel","4B",nr,r,g,b)
  */
  uint8_t led_nr, red, green, blue;
  unpack(args, &led_nr, &red, &green, &blue);
  if ((debug & 1) == 1) {
    Serial.printf("\nneopixel nr %d, rgb (%d, %d, %d)\n", led_nr, red, green, blue);
  }
  strip->setPixelColor(led_nr, red, green, blue);
  uartremote.send_command("neopixelack", "B", 0);
}


void neopixel_init(Arguments args) {
  /*
    call("neopixel_init","2B",nr_leds,pin)
  */
  uint8_t nr_leds, pin;
  unpack(args, &nr_leds, &pin);
  delete strip;
  strip = new Adafruit_NeoPixel(nr_leds, pin);
  uartremote.send_command("neopixel_initack", "B", 0);
}

void neopixel_show(Arguments args) {
  /*
    call("neopixel_show")
  */
  strip->show();
  if (debug == 1) {
    Serial.printf("\nShow neoixels\n");
  }
  uartremote.send_command("neopixel_showack", "B", 0);
}

void servo(Arguments args) {
  /*
    call("servo",">BI",servo_nr,servo_pos)
  */
  uint8_t servo_nr;
  uint32_t servo_pos;
  unpack(args, &servo_nr, &servo_pos);
  if ((debug & 1) == 1) {
    Serial.printf("servo_nr %d, pos%d\n", servo_nr, servo_pos);
  }
  switch (servo_nr) {
    case 1:
      servo1.write(servo_pos);
      break;
    case 2:
      servo2.write(servo_pos);
      break;
    case 3:
      servo3.write(servo_pos);
      break;
    case 4:
      servo4.write(servo_pos);
      break;
    default:
      break;
  }
  uartremote.send_command("servoack", "B", 0);
}




// Arduino setup function. Runs in CPU 1
void setup() {
  // check LMS-ESP32 version
  if (strcmp(ESP.getChipModel(), "ESP32-PICO-V3-02") == 0) {
    is_lms_esp32_version2 = 1;
    RXD2 = 8;
    TXD2 = 7;
    servo1Pin = 19;
    servo2Pin = 20;
    servo3Pin = 21;
    servo4Pin = 22;
    //neopixel_debug->setPixelColor(0, 10, 0, 0);
    //neopixel_debug->show();
  } else {
    is_lms_esp32_version2 = 0;
    RXD2 = 18;
    TXD2 = 19;
  }
  uartremote.init(RXD2, TXD2);
  Serial.begin(115200);
  Wire.begin(5, 4);  // sda=pin(5), scl=Pin(4)
  strip->begin();
  strip->show();
  // servo's
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
  uartremote.add_command("debug", &setdebug);
  uartremote.add_command("connected", &connected);
  uartremote.add_command("btaddress", &btaddress);
  uartremote.add_command("btdisconnect", &btdisconnect);
  uartremote.add_command("btallow", &btallow);
  uartremote.add_command("btforget", &btforget);
  uartremote.add_command("btfilter", &btfilter);
  uartremote.add_command("gamepad", &gamepad);
  uartremote.add_command("imu", &imu);
  uartremote.add_command("i2c_scan", &i2c_scan);
  uartremote.add_command("i2c_read", &i2c_read);
  uartremote.add_command("i2c_read_reg", &i2c_read_reg);
  uartremote.add_command("neopixel", &neopixel);
  uartremote.add_command("neopixel_show", &neopixel_show);
  uartremote.add_command("neopixel_init", &neopixel_init);
  uartremote.add_command("servo", &servo);
  Serial.printf("\n====================\nBluePad32 UartRemote\n====================\n(c) 2025 Ste7an www.antonsmindstorms.com\n\n");

  Serial.println(version);
  Serial.println((String) "Running on " + ESP.getChipModel());
  Serial.printf("https://github.com/ste7anste7an/bluepad32_arduino\n\n");
  //Serial.printf("Bluepad32 Firmware: %s\n", BP32.firmwareVersion());
  //const uint8_t *addr = BP32.localBdAddress();
  //Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2],
  //              addr[3], addr[4], addr[5]);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();
}

// Arduino loop function. Runs in CPU 1


int refresh_BP32 = 0;
void loop() {
  // This call fetches all the gamepad info from the NINA (ESP32) module.
  // Just call this function in your main loop.
  // The gamepads pointer (the ones received in the callbacks) gets updated
  // automatically.
  refresh_BP32++;
  if (refresh_BP32 == 10) {  // refresh only every 10 loops
    BP32.update();
    refresh_BP32 = 0;
  }
  // check for commands on uartremote
  if (uartremote.available() > 0) {
    int error = uartremote.receive_execute();
    if (error == 1) {
      printf("error in receiving command\n");
    }
  }
  delay(1);
  // It is safe to always do this before using the gamepad API.
  // This guarantees that the gamepad is valid and connected.

  // Another way to query the buttons, is by calling buttons(), or
  // miscButtons() which return a bitmask.
  // Some gamepads also have DPAD, axis and more.
  if ((debug & 2) == 2) {  // more verbose
    for (int i = 0; i < MAX_GAMEPADS; i++) {
      GamepadPtr myGamepad = myGamepads[i];
      if (myGamepad && myGamepad->isConnected()) {
        Serial.printf(
          "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: "
          "%4d, %4d, brake: %4d, throttle: %4d, misc: 0x%02x\n",
          i,                        // Gamepad Index
          myGamepad->dpad(),        // DPAD
          myGamepad->buttons(),     // bitmask of pressed buttons
          myGamepad->axisX(),       // (-511 - 512) left X Axis
          myGamepad->axisY(),       // (-511 - 512) left Y axis
          myGamepad->axisRX(),      // (-511 - 512) right X axis
          myGamepad->axisRY(),      // (-511 - 512) right Y axis
          myGamepad->brake(),       // (0 - 1023): brake button
          myGamepad->throttle(),    // (0 - 1023): throttle (AKA gas) button
          myGamepad->miscButtons()  // bitmak of pressed "misc" buttons
          /* myGamepad->gyroX(),       // Gyro X
            myGamepad->gyroY(),       // Gyro Y
            myGamepad->gyroZ(),       // Gyro Z
            myGamepad->accelX(),      // Accelerometer X
            myGamepad->accelY(),      // Accelerometer Y
            myGamepad->accelZ()       // Accelerometer Z
          */
        );
      }
    }
    // You can query the axis and other properties as well. See Gamepad.h
    // For all the available functions.

    // The main loop must have some kind of "yield to lower priority task" event.
    // Otherwise the watchdog will get triggered.
    // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
    // Detailed info here:
    // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

    vTaskDelay(1);
    //delay(150);
  }
}
