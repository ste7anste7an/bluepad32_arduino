# BLUEPAD32 Generic for Spike 3 and PyBricks

This version emulates two different sensors: 1) the color sensor (ID=0x3d) and 2) the color matrix (ID=0x40, a 3x3 LED cube that comes with the Spike essential).

### Color Sensor emulation
If you flash this firmware [BluePad32 LPF2 for Spike3](https://firmware.antonsmindstorms.com/) and you connect the LMS-ESP32 with your robot bricks, you should see a new Color Sensor being recognized by the SPIKE app.
In the SPIKE3 app you can use the advanced sensor blocks to read the different colors form a Color Sensor to read out the values of a connected gamepad.

The original Lego Color Sensor sends a 16-byte payload for mode 0. The Spike3 reads this mode 0 values, altough mode 0 is configured as a single byte mode. In order to have the PyBricks hub accepts the values send at mde 0, mode 0 needs to be extended to 8 DATA16 values (in total 16 bytes).  Spike3 never selects anothe  mode than mode 0. Even when reading RGBI values from the sensor (which s defined as mode 5), still mode 0 is used with a 16-byte payload. 

The Bluepad values are encoded in both mode0 and mode 1 as follows:
```
short red =   DPAD*256 + BUTTONS --> | DPAD          | BUTTONS|
short green = LY*256 + LX        --> |Left joystick Y| Left joystick X|
short blue =  RY*256 + RX        --> |Right joystick X| Right joystick X|
```

Spike 3 can not send any values back to the sensor. Pybricks supports two call backs on mode 0 and mode 1 respectively. These calbacks are  `pybricks_neopixel_callback` for mode 0 and `pybricks_servo_callback` for mode 1. When reading the HSV intesities from PyBricks, it selects mode 5, reads the RGBI values and converts them to HSV values internally. That means that mode 5 can not be used to pass raw values due to this conversion.



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

# Internals
## Original Coor Sensor
The original Lego color sensor does not adhere to it's own advertised mode format. Mode 0 is defined for sending a single recognized color (value 0 to 10) to the hub. In practise, the Lego Color Sensor sends a 16-byte messgae on mode 0 to the hub containing a combination of the following parameters:

```
E0 <C> <??> <RL> <RH> <GL> <GH> <BL> <BH> <IL> <IH> <padding>
```

with C the recognized color value between 0 and 10, and R, G, and H the three color intensities represented as a two byte word (L,H), and I the overal intensiy. 
