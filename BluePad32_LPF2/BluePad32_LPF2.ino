/*
 * 
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






#include <Bluepad32.h>
#include "uni_hid_device.h"
#include <stdio.h>

#include <LPF2.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>

GamepadPtr myGamepads[BP32_MAX_GAMEPADS];
#define RXD2 18
#define TXD2 19
EV3UARTEmulation sensor(RXD2, TXD2, 62, 115200);

char bt_allow[6] = {0, 0, 0, 0, 0, 0};
bool bt_filter=false;

int debug=0; // global debug; if 1 -> print debug.
#define LED_PIN 12
#define LED_COUNT 64

// use pointer allows to dynamically change nrumber of leds or pin
// change strip.begin() to strip->begin(), etc.
// delete object before initiating a new one
Adafruit_NeoPixel* strip = new Adafruit_NeoPixel(LED_COUNT,LED_PIN); //, NEO_GRB + NEO_KHZ800);

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
          Serial.printf("filtered: %02X:%02X:%02X:%02X:%02X:%02X\n",bt_allow[0],bt_allow[1],bt_allow[2],bt_allow[3],bt_allow[4],bt_allow[5]);
         if (memcmp(bt_allow,properties.btaddr,6)==0) { // bt_allow
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

  uint8_t old_led=0,rumble_force,rumble_duration;


void servo_neo_callback(byte buf[],byte s) {
byte nr_short=int(s/2);
  short vals[nr_short];
  // Serial.printf("size %d, nr short %d\n",s,nr_short);
  for (int i=0; i<nr_short; i++) {
    vals[i]=buf[i*2]+buf[i*2+1]*256; // in Block SPIKE language only up to 128 can be used:  vals[i]=buf[i*2]+buf[i*2+1]*128; 
    Serial.printf("vals[%d]=%d\n",i,vals[i]);
  }

   Serial.printf("servo %d %d %d %d\n",vals[0],vals[1],vals[2],vals[3]);
  servo1.write(vals[0]);
  servo2.write(vals[1]);
  servo3.write(vals[2]);
  servo4.write(vals[3]);
 
  if (buf[8]==65) { // write led
     strip->show();
  } else if (buf[8]==66) { // init led
     delete strip;
     strip=new Adafruit_NeoPixel(buf[9],buf[10]); //nr_leds, pin
  } else if (buf[8]==67) { // clear all leds led
      for (int i=0; i<strip->numPixels(); i++ ) {
        strip->setPixelColor(i,0,0,0);
      }
  } else {
      strip->setPixelColor(buf[8],buf[9],buf[10],buf[11]);
  }

}

void neopixel_callback(byte buf[],byte s) {
byte nr_short=int(s/2);
  short vals[nr_short];
  // Serial.println();
  for (int i=0; i<nr_short; i++) {
    vals[i]=buf[i*2]+buf[i*2+1]*128;
    Serial.printf("vals[%d]=%d\n",i,vals[i]);
  }

  if (vals[0]==65) { // write led
     strip->show();
  } else if (vals[0]==66) { // init led
      delete strip;
     strip=new Adafruit_NeoPixel(vals[1],vals[2]); //nr_leds, pin
     
  }  else {
      strip->setPixelColor(vals[0],vals[1],vals[2],vals[3]);
  }
}


  // Arduino setup function. Runs in CPU 1
  void setup() {
    Serial.begin(115200);
    //sensor.create_mode("GAMEPAD", true, DATA8, 12, 5, 0,0.0f,512.0f,0.0f,1024.0f,0.0f,100.0f,"RAW",ABSOLUTE,ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    sensor.create_mode("GAMEPAD", true, DATA16, 6, 5, 0,0.0f,512.0f,0.0f,1024.0f,0.0f,512.0f,"XYBD",0,ABSOLUTE); //map in and map out unit = "XYBD" = x, y, buttons, d-pad
    
    sensor.get_mode(0)->setCallback(servo_neo_callback);  // attach call back function to mode 0
   
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
    
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t *addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2],
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

int refresh_BP32=0;
  // Arduino loop function. Runs in CPU 1


unsigned long last_reading = 0;
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
    if (millis() - last_reading > 20) {
      int mode=sensor.get_current_mode();
      if (mode==0) {
       
        short bb[8];
 //for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    GamepadPtr myGamepad = myGamepads[0];
 
        if (myGamepad && myGamepad->isConnected()) {
          //myGamepad->buttons(),myGamepad->dpad(),
          bb[0]=myGamepad->axisX();
          bb[1]=myGamepad->axisY();
          bb[2]=myGamepad->axisRX();
          bb[3]=myGamepad->axisRY();
          bb[4]=myGamepad->buttons();
          bb[5]=myGamepad->dpad();
          bb[6]=0;
          bb[7]=0;
          //Serial.printf("gp:%d %d \n", bb[4],bb[5]);
        }
        sensor.send_data16(bb,8);
        } 
      
      
      last_reading = millis();
    }
    vTaskDelay(1);
    //delay(150);
  }
