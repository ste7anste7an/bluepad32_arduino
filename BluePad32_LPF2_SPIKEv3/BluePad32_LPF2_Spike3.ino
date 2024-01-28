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

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// uncomment the following line if building for PyBricks
//#define PYBRICKS 1
#define SPIKE3 1


#include <Bluepad32.h>
//#include "uni_hid_device.h"
#include <stdio.h>

#include <LPF2.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

GamepadPtr myGamepads[BP32_MAX_GAMEPADS];
#define RXD2 18
#define TXD2 19
EV3UARTEmulation sensor(RXD2, TXD2, 0x3d, 115200); // light sensor 0x3d

char bt_allow[6] = {0, 0, 0, 0, 0, 0};
bool bt_filter = false;

int debug = 0; // global debug; if 1 -> print debug.
#define LED_PIN 12
#define LED_COUNT 64

// use pointer allows to dynamically change nrumber of leds or pin
// change strip.begin() to strip->begin(), etc.
// delete object before initiating a new one
Adafruit_NeoPixel* strip = new Adafruit_NeoPixel(LED_COUNT, LED_PIN); //, NEO_GRB + NEO_KHZ800);

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


// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedGamepad(GamepadPtr gp) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myGamepads[i] == nullptr && !foundEmptySlot) {
      Serial.printf("CALLBACK: Gamepad is connected, index=%d\n", i);
      // Additionally, you can get certain gamepad properties like:
      // Model, VID, PID, BTAddr, flags, etc.
      GamepadProperties properties = gp->getProperties();
      char btaddr_str[18]; // Allocate space for the formatted string (six 2-digit hexadecimal numbers plus five colons)
      sprintf(btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]); // Format the address as a string

      Serial.printf("Gamepad model: %s, VID=0x%04x, PID=0x%04x  bt_addr=%s\n",
                    gp->getModelName().c_str(), properties.vendor_id,
                    properties.product_id, btaddr_str);

      if (bt_filter) {
        Serial.printf("received: %02X:%02X:%02X:%02X:%02X:%02X ", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
        Serial.printf("filtered: %02X:%02X:%02X:%02X:%02X:%02X\n", bt_allow[0], bt_allow[1], bt_allow[2], bt_allow[3], bt_allow[4], bt_allow[5]);
        if (memcmp(bt_allow, properties.btaddr, 6) == 0) { // bt_allow
          myGamepads[i] = gp;
          foundEmptySlot = true;
          Serial.printf("bt_filtered: allowed\n");
        } else {
          gp->disconnect();
          Serial.printf("bt_filtered: gamepad disconnected\n");
        }
        break;
      } else {
        Serial.printf("un_filtered: connected\n");
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

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
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
  void btaddress(Arguments args) {
  uint8_t idx;
  unpack(args, &idx);
  if ((idx >= 0) && (idx < BP32_MAX_GAMEPADS)) {
    if (myGamepads[idx]!=nullptr) {
      GamepadPtr myGamepad = myGamepads[idx];
      GamepadProperties properties = myGamepad->getProperties();
      char btaddr_str[18]; // Allocate space for the formatted string (six 2-digit hexadecimal numbers plus five colons)
      sprintf(btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", properties.btaddr[0], properties.btaddr[1], properties.btaddr[2],
                                                           properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
      uartremote.send_command("btaddressack","17s",btaddr_str);
    } else {
      uartremote.send_command("btaddresserr", "B", 0);
    }
  } else {
    uartremote.send_command("btaddresserr", "B", 0);
  }
  }


  void btdisconnect(Arguments args) {
  uint8_t idx;
  unpack(args, &idx);
  if ((idx >= 0) && (idx < BP32_MAX_GAMEPADS)) {
    if (myGamepads[idx]!=nullptr) {
      GamepadPtr myGamepad = myGamepads[idx];
      myGamepad->disconnect();
      uartremote.send_command("btdisconnectack", "B",0);
    } else {
      uartremote.send_command("btdisconnecterr", "B", 0);
    }
  } else {
    uartremote.send_command("btdisconnecterr", "B", 0);
  }
  }


  void btforget(Arguments args) {
  BP32.forgetBluetoothKeys();
  uartremote.send_command("btforget","B",0);
  }

  void btallow(Arguments args) {
  char btaddr_str[18];
  unpack(args, btaddr_str);
  sscanf( btaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X", bt_allow,bt_allow+1,bt_allow+2,bt_allow+3,bt_allow+4,bt_allow+5);
  Serial.printf("btallow: %02X:%02X:%02X:%02X:%02X:%02X  filtered:%d\n",bt_allow[0],bt_allow[1],bt_allow[2],bt_allow[3],bt_allow[4],bt_allow[5],bt_filter);
  uartremote.send_command("allowack","B",0);
  }

  void btfilter(Arguments args) {
  uint8_t filtered;
  unpack(args, &filtered);
  bt_filter=(filtered==1);
  Serial.printf("filtered:%d\n",bt_filter);
  uartremote.send_command("btfilterack","B",0);
  }

  void gamepad(Arguments args) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    GamepadPtr myGamepad = myGamepads[i];
    if (myGamepad && myGamepad->isConnected()) {
        uartremote.send_command("gamepadack","2H4h",myGamepad->buttons(),myGamepad->dpad(),
                                            myGamepad->axisX(),myGamepad->axisY(),
                                            myGamepad->axisRX(),myGamepad->axisRY());
    }
    else {
        uartremote.send_command("gamepadack","2H4h",0,0,0,0,0,0);
    }
  }
  }

  void imu(Arguments args) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
      GamepadPtr myGamepad = myGamepads[i];
      if (myGamepad && myGamepad->isConnected()) {
        uartremote.send_command("imuack", "6I",
                                myGamepad->gyroX(),       // Gyro X
                                myGamepad->gyroY(),       // Gyro Y
                                myGamepad->gyroZ(),       // Gyro Z
                                myGamepad->accelX(),      // Accelerometer X
                                myGamepad->accelY(),      // Accelerometer Y
                                myGamepad->accelZ()       // Accelerometer Z
                               );
      } else {
        uartremote.send_command("imuerr", "6I", 0, 0, 0, 0, 0, 0);
      }
    }
  }


*/

uint8_t old_led = 0, rumble_force, rumble_duration;

#define FILL   0x10
#define ZERO   0x20
#define SET    0x30
#define CONFIG 0x40
#define WRITE  0x80


void neopixel_callback(byte buf[], byte s) {
  // Serial.println();
  byte num_pixels = strip->numPixels();
  byte cmd = buf[0] & 0x7f; // all but bit 7 (write) and strip lower 4 bits.
  byte write_leds = buf[0] & 0x80;
   Serial.printf("neopixel: cmd %02X write_leds %02X\r\n",cmd,write_leds);
  for (int i=0; i<s; i++) Serial.printf("%02X ",buf[i]);
  Serial.printf("\n");
  if (cmd == FILL) {
    for (int i = 0; i < num_pixels; i++ ) {
       strip->setPixelColor(i, buf[1], buf[2], buf[3]);
     }
  }
  else if (cmd == ZERO) {
    for (int i = 0; i < num_pixels; i++ ) {
       strip->setPixelColor(i, 0,0,0);
     }
  }
  else if (cmd == SET) {
    // [SET][nr_leds][start_led][r0,g0,b0][r1,g1,b1][r2,g2,b2][r3,g3,b3][0]
    byte nr_leds =buf[1];
    byte start_led = buf[2];
     Serial.printf("Neopixel SET %d %d num_pixels %d\r\n",nr_leds,start_led,num_pixels);
    if ((nr_leds==0) or (nr_leds>4)) nr_leds=4;
    if (start_led+nr_leds<=num_pixels) {
      for (int i = 0; i < nr_leds; i++ ) {
       // Serial.printf("set_led: %d %d %d %d \r\n",i+start_led, buf[3+i*3], buf[4+i*3], buf[5+i*3]);
       strip->setPixelColor(i+start_led, buf[3+i*3], buf[4+i*3], buf[5+i*3]);
      } 
    }
  }
  else if (cmd == CONFIG) {
    delete strip;
    strip = new Adafruit_NeoPixel(buf[1], buf[2]); //nr_leds, pin
  }
  if (write_leds & WRITE) {
    // Serial.println("Write leds");
    strip->show();
  } 
 
}


void servo_callback(byte buf[], byte s) {
  byte nr_short = int(s / 2);
  short vals[nr_short];
  //Serial.printf("size %d, nr short %d\n",s,nr_short);
  for (int i = 0; i < nr_short; i++) {
    vals[i] = buf[i * 2] + buf[i * 2 + 1] * 256; // in Block SPIKE language only up to 128 can be used:  vals[i]=buf[i*2]+buf[i*2+1]*128;
    //Serial.printf("vals[%d]=%d\n",i,vals[i]);
  }

  // Serial.printf("servo %d %d %d %d\n",vals[0],vals[1],vals[2],vals[3]);
  servo1.write(vals[0]);
  servo2.write(vals[1]);
  servo3.write(vals[2]);
  servo4.write(vals[3]);
}

// Arduino setup function. Runs in CPU 1
void setup() {
  Serial.begin(115200);
#ifdef SPIKE3
  sensor.create_mode("POW", true, DATA16, 8, 4, 0, -100.0f, 100.0f, -100.0f,100.0f, -100.0f, 100.0f, "PCT", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
  sensor.create_mode("SPE", true, DATA32, 1, 4, 0, -100.0f, 100.0f, -100.0f,100.0f, -100.0f, 100.0f, "PCT", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
  sensor.create_mode("POS", true, DATA32, 1, 4, 0, -360.0f, 360.0f, -100.0f,100.0f, -360.0f, 360.0f, "PCT", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
  sensor.create_mode("APOS", true, DATA32, 1, 4, 0, -180.0f, 179.0f, -100.0f,100.0f, -180.0f, 179.0f, "PCT", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad


//#ifdef PYBRICKS
//  sensor.create_mode("GPLED", true, DATA8, 16, 5, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "RAW", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
//  sensor.create_mode("GPSRV", true, DATA8, 16, 5, 0, 0.0f, 512.0f, 0.0f, 512.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
//  sensor.create_mode("GPIIC", true, DATA8, 16, 5, 0, 0.0f, 512.0f, 0.0f, 1024.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad

#else
  sensor.create_mode("GAMEPAD", true, DATA16, 6, 5, 0, 0.0f, 512.0f, 0.0f, 1024.0f, 0.0f, 512.0f, "XYBD", ABSOLUTE, ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
#endif

  sensor.get_mode(0)->setCallback(neopixel_callback);  // attach call back function to mode 0
  sensor.get_mode(1)->setCallback(servo_callback);  // attach call back function to mode 1
  //sensor.get_mode(2)->setCallback(i2c_callback);  // attach call back function to mode 1

  Wire.begin(5, 4); // sda=pin(5), scl=Pin(4)
  strip->begin();
  strip->show();
  // servo's
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo1.setPeriodHertz(50);      // Standard 50hz servo
  servo2.setPeriodHertz(50);      // Standard 50hz servo
  servo3.setPeriodHertz(50);      // Standard 50hz servo
  servo4.setPeriodHertz(50);      // Standard 50hz servo
  servo1.attach(servo1Pin, minUs, maxUs);
  servo2.attach(servo2Pin, minUs, maxUs);
  servo3.attach(servo3Pin, minUs, maxUs);
  servo4.attach(servo4Pin, minUs, maxUs);
  Serial.println("BluePad32 SPIKEv3, see https://github.com/antonvh/PUPRemote/blob/main/examples/bluepad/spike3/README.md");
  Serial.printf("Firmware: %s\r\n", BP32.firmwareVersion());
  const uint8_t *addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\r\n", addr[0], addr[1], addr[2],
                addr[3], addr[4], addr[5]);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);

  // "forgetBluetoothKeys()" should be called when the user performs
  // a "device factory reset", or similar.
  // Calling "forgetBluetoothKeys" in setup() just as an example.
  // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
  // But might also fix some connection / re-connection issues.
  BP32.forgetBluetoothKeys();

  sensor.reset();
  delay(200);


}

int clip(int n, int lower, int upper) {
  return MAX(lower, MIN(n, upper));
}

int refresh_BP32 = 0;
// Arduino loop function. Runs in CPU 1


unsigned long last_reading = 0;
int last_mode=0;
void loop() {
  // This call fetches all the gamepad info from the NINA (ESP32) module.
  // Just call this function in your main loop.
  // The gamepads pointer (the ones received in the callbacks) gets updated
  // automatically.
  refresh_BP32++;
  if (refresh_BP32 == 10) {
    BP32.update();
    refresh_BP32 = 0;
  }
  sensor.heart_beat();
  int mode = sensor.get_current_mode();
  if ((millis() - last_reading > 20) || mode!=last_mode) {
    last_mode = mode;
    if (mode!=last_mode) last_mode=mode;
    // for any mode gamepad fields are populated
    // if (mode == 0) {

      short bb[16];
      byte a,b;
      memset(bb, 0, 16);
      //for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
      GamepadPtr myGamepad = myGamepads[0];

      if (myGamepad && myGamepad->isConnected()) {
        //myGamepad->buttons(),myGamepad->dpad(),
        /*
        bb[0] = ((myGamepad->axisX()) >> 1)& 0xff;
        bb[1] = (myGamepad->axisX()) >> 8;
        bb[2] = (myGamepad->axisY()) & 0xff;
        bb[3] = (myGamepad->axisY()) >> 8;
        bb[4] = (myGamepad->axisRX()) & 0xff;
        bb[5] = (myGamepad->axisRX()) >> 8;
        bb[6] = (myGamepad->axisRY()) & 0xff;
        bb[7] = (myGamepad->axisRY()) >> 8;
        bb[8] = (myGamepad->buttons()) & 0xff;
        bb[9] = (myGamepad->buttons()) >> 8;
        bb[10] = (myGamepad->dpad()) & 0xff;
        bb[11] = (myGamepad->dpad()) >> 8;
*/
        a = clip((myGamepad->axisX()+512),0,1023) >> 2;
        b = clip((myGamepad->axisY()+512),0,1023) >>2;
        bb[0]= a+b*256; 
        a =  clip((myGamepad->axisRX()+512),0,1023)>>2;
        b =  clip((myGamepad->axisRY()+512),0,1023)>>2;
        bb[1] = a+b*256;
        
        a = (myGamepad->buttons()) & 0xff;
        b = (myGamepad->dpad()) & 0xff;
        bb[2]=a+b*256;
        bb[3]=bb[0];
        bb[4]=bb[1];
        bb[5]=bb[2];
        bb[6]=bb[0];
        bb[7]=bb[1];
        //for (int i=0; i<3; i++) {
        //  Serial.printf("bb[%d]=%X\n",i,bb[i]);
       // }
        Serial.printf("\n");
      }
      int nr_bytes = sensor.get_mode(mode)->sample_size;
      //sensor.send_data8(bb, nr_bytes);
    sensor.send_data16((short*)bb, nr_bytes);
    


    last_reading = millis();
  }
  vTaskDelay(1);
  //delay(150);
}
