# BLUEPAD32 Spike3


This version emulates two different sensors: 1) the color sensor (ID=0x3d) and 2) the color matrix (ID=0x40, a 3x3 LED cube that comes with the Spike essential).

## Serial command line

Int his version a command line via the Serial Port is implemented that allows you to change settings and safe the settings in EEPROM (which is implememted on the ESP32 as a region in Flash memory).
use for instance this [online serial terminal](https://googlechromelabs.github.io/serial-terminal/) to configure your sensor.


## Web configurator

With this version comes an online web configuration tool (https://bluepad.ste7an.nl).