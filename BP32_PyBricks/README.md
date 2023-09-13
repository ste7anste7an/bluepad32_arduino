# Bluepad32 example for pybricks

Flash the firmware on the ESP32 (use the firmware in this guthub), or go to (firmware.antonsmindstorms.com)[https://firmware.antonsmindstorms.com] and choose `firmware_BluePad32_Pybricks`.

## LPF2 fields

The gamepad reading come as single byte values in the following format

|bytes | reading |
|------|--------|
|byte 0, byte 1 | -512 <= Left gamepad X <= 512 |
|byte 1, byte 2 | -512 <= Left gamepad Y <= 512 |
|byte 3, byte 4 | -512 <= Right gamepad X <= 512 |
|byte 5, byte 6 | -512 <= Right gamepad Y <= 512 |
|byte 7, byte 8 | buttons |
| byte 9, byte 10 | Dpad |

For output the following fields are defined:
|bytes | field | reading |
|------|--------|-----|
|byte 0, byte 1 | Servo 0 | |
|byte 1, byte 2 | Servo 1 | |
|byte 3, byte 4 | Servo 2 | |
|byte 5, byte 6 | Servo 3 | |
|byte 7, byte 8 | led_val0| 0< led nr < 64 *|
|byte 9, byte 10 | led_val1|  0 <= red < 256 |
|byte 11, byte 12 | led_val2| 0 <=  green < 256|
|byte 13, byte 14 | led_val3| 0 <=  blue < 256|

* The `led_val0` fields is special. 
| value of `led_val0` | explanation |
|---------------------|-------------|
| 65		      | write NeoPixel values to leds, led_val1, 2 and 3 no meaning |
| 66	 	 | define Neopixel. `led_val1`: number of pixels, `led_val2': phyical GPIO pin to connect Neopixel 9default 12) |
| 67		| clear all neopixels|

