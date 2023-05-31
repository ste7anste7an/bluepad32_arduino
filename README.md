# Arduino BluePad32 LMS-ESP32 firmware

This repository contains two versions of the BluePad32 for running on the LMS-ESP32 board. One version using our own UartRemote protocol (running over a UART connection  between the LMS-ESP32 baord and the Lego robots. The other version emulates the LPF2 protocol and presents itself as a special sensor from the Lego robot perspective.

## BluePad32_UartRemote

This firmware uses the UartRemote protocol to communicate between the BluePad32 module and a Lego robot (such as Lego SPIKE prime, Lego Robotic Inventor or Lego EV3). This version has more functions compared to the LPF2 version. We added functions to request the BlueTooth adress of the currently connected Game Controller. Furthermore, some functions are added to whitelist specific Bluetooth addresses that are allowed to connect to the  BluePad32 firmware. This facilitates the connection in an enviroment where there are multiple Controllers and multiple LMS-ESP32 boards.
Look in the BluePad32_UartRemote directory for the implemented UartRemote commands in this firmware.

## BluePad32_LPF2

This firmware emulates a LPF2 (Lego Power Function) sensor and is as such compatible with PyBricks and with teh Block languae of both Spike Prime (version2) and Lego Mindstorms app. When the LMS-ESP32 module is connected to a Lego robot, the lego robot effectively notices that a 'special' sensor is connected. Using low-leverl PUPDevice function on the PyBricks and using special Debug commands for reading and writing low-level sensor values in the Lego Block language, we are able to disclose the BluePad32 fucntions to these  environments.

# Setting up BluePad32 for Arduino
This projects heavely leans on the superb [BluePad32 library](). BluePad32 runs on a seperate core and deals with all the special Bluetooth protocols for a wide collection game controllers and aims to unify their capabaility on a generic GameController object. Lately, the BluePad32 library became available as a special hardware Component in the Arduino IDE environment. This makes integration with other Arduino libraries very easy (such as Servo and NeoPixel libraries).
For setting up the required Arduino enviromemnt we refer to the [BluePad32 Arduino documentation](https://github.com/ricardoquesada/bluepad32/blob/main/docs/plat_arduino.md).

## Lego Arduino Libraries
Here you find the two Arduino libraries that support the two versions of the BluePad32 firmware. You can install these libraries in your local Arduino environemnt by choosing: Add Library -> Install from Zip file, and select the zip file of the specific library. This project depends on two other libraries, namely teh Adafruit_Neopixel and the ESP32Servo library. These libraries can be installed through the Arduino Library manager. Alternatevely, you can also use VS Code Studio Platform IO to compile these projects.

# Pre build firmware
In the firmware directory, you will find the latest versions of the pre-build firmware for both LPF2 and UartRemote versions of BluePad32.

