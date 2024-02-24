# BLUEPAD32 Spike3

This version emulates two different sensors: 1) the color sensor (ID=0x3d) and 2) the color matrix (ID=0x40, a 3x3 LED cube that comes with the Spike essential).

### Color Sensor emulation
If you flash this firmware [BluePad32 LPF2 for Spike3](https://firmware.antonsmindstorms.com/) and you connect the LMS-ESP32 with your robot bricks, you should see a new Color Sensor being recognized by the SPIKE app.
In the SPIKE3 app you ca use the advanced sensor blocks to read the different colors form a Color Sensor to read out the values of a connected gamepad.

### Color Matrix 

The same firmware can also be configured to emulate a Color Matrix.

Now you can use the default SPIKE3 World Block or Python to program the colors of your own NeoPixels. Because you probably want to make changes to the default configuration you can use the Serial Command Line or whe Web Configurator, as described below.

## Serial command line

In this version a command line via the Serial Port is implemented that allows you to change settings and safe the settings in EEPROM (which is implememted on the ESP32 as a region in Flash memory).
use for instance this [online serial terminal](https://googlechromelabs.github.io/serial-terminal/) to configure your sensor.

## Compiling
To compile this firmware, you need to install the Libraries: AdaFruit Neopixels and CmdParser. The library fiels FPF2 that are present in this directory are slightly changed from the LPF2 library as found in the Arduino Library in this repository. So, you should include `LPF2.cpp' and `LPF.h' within the directory of the Arduino source code.

## Web configurator

With this version comes an online web configuration tool (https://bluepad.ste7an.nl).