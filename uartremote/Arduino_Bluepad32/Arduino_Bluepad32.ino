#include <Bluepad32.h>
#include "uni_hid_device.h"
#include <stdio.h>

#include <UartRemote.h>
GamepadPtr myGamepads[BP32_MAX_GAMEPADS];
UartRemote uartremote;
char bt_allow[6] = {0, 0, 0, 0, 0, 0};
bool bt_filter=false;

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

void connected(Arguments args) {
  uni_hid_device_t* g_dev_ptr;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    GamepadPtr myGamepad = myGamepads[i];
    if (myGamepad && myGamepad->isConnected()) {

      g_dev_ptr = uni_hid_device_get_instance_for_idx(i);
      Serial.printf("idx=%d:\n", i);
      for (int ii = 0; ii < 6; ii++) {
        Serial.printf("%02X:", g_dev_ptr->conn.btaddr[ii]);
      }
      Serial.println();


      uartremote.send_command("connectedack", "B", 1);
    } else {
      uartremote.send_command("connectedack", "B", 0);
    }
  }
}

void btaddress(Arguments args) {
  uni_hid_device_t* g_dev_ptr;
  uint8_t idx;
  uint8_t bt_addr[6];
  unpack(args, &idx);
  if ((idx >= 0) && (idx < BP32_MAX_GAMEPADS)) {
    g_dev_ptr = uni_hid_device_get_instance_for_idx(idx);
    memcpy(bt_addr, g_dev_ptr->conn.btaddr, 6);
    uartremote.send_command("btaddressack", "6B", bt_addr[0], bt_addr[1], bt_addr[2], bt_addr[3], bt_addr[4], bt_addr[5]);
  } else {
    uartremote.send_command("btaddresserr", "6B", 0, 0, 0, 0, 0, 0);
  }
}

void btdisconnect(Arguments args) {
  uint8_t idx;
  unpack(args, &idx);
  if ((idx >= 0) && (idx < BP32_MAX_GAMEPADS)) {
    GamepadPtr myGamepad = myGamepads[idx];
    myGamepad->disconnect();
    uartremote.send_command("btdisconnectack", "B",0);
  } else {
    uartremote.send_command("btdisconnecterr", "B", 0);
  }
}

void btallow(Arguments args) {
  uint8_t bt0, bt1, bt2, bt3, bt4, bt5,filtered;
  unpack(args, &bt0, &bt1, &bt2, &bt3, &bt4, &bt5, &filtered);
  bt_allow[0] = bt0; bt_allow[1] = bt1; bt_allow[2] = bt2;
  bt_allow[3] = bt3; bt_allow[4] = bt4; bt_allow[5] = bt5;
  bt_filter=(filtered==1);
  Serial.printf("btallow: %02X:%02X:%02X:%02X:%02X:%02X  filtered:%d\n",bt_allow[0],bt_allow[1],bt_allow[2],bt_allow[3],bt_allow[4],bt_allow[5],bt_filter);
  uartremote.send_command("allowack","B",0);
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
  // Arduino setup function. Runs in CPU 1
  void setup() {
    Serial.begin(115200);
    uartremote.add_command("connected", &connected);
    uartremote.add_command("btaddress", &btaddress);
      uartremote.add_command("btdisconnect", &btdisconnect);
     uartremote.add_command("btallow", &btallow);
    uartremote.add_command("gamepad", &gamepad);
    uartremote.add_command("imu", &imu);


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
  }

  // Arduino loop function. Runs in CPU 1


  int refresh_BP32 = 0;
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
    /*
       Serial.printf(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: "
        "%4d, %4d, brake: %4d, throttle: %4d, misc: 0x%02x, gyro x:%6d y:%6d "
        "z:%6d, accel x:%6d y:%6d z:%6d\n",
        i,                        // Gamepad Index
        myGamepad->dpad(),        // DPAD
        myGamepad->buttons(),     // bitmask of pressed buttons
        myGamepad->axisX(),       // (-511 - 512) left X Axis
        myGamepad->axisY(),       // (-511 - 512) left Y axis
        myGamepad->axisRX(),      // (-511 - 512) right X axis
        myGamepad->axisRY(),      // (-511 - 512) right Y axis
        myGamepad->brake(),       // (0 - 1023): brake button
        myGamepad->throttle(),    // (0 - 1023): throttle (AKA gas) button
        myGamepad->miscButtons(), // bitmak of pressed "misc" buttons
        myGamepad->gyroX(),       // Gyro X
        myGamepad->gyroY(),       // Gyro Y
        myGamepad->gyroZ(),       // Gyro Z
        myGamepad->accelX(),      // Accelerometer X
        myGamepad->accelY(),      // Accelerometer Y
        myGamepad->accelZ()       // Accelerometer Z
      );
    */
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
