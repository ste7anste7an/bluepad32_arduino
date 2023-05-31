# Flash firmware

## firmwares
The following firmwares are available:
- `firmware_BP32_LPF2_LEGO.bin`: GamePad32 as an emulates LPF2 sensor for official Lego app
- `firmware_BP32_LPF2_PyBricks.bin`: GamePad32 as an emulated LPF2 sensor for PyBricks
- `firmware_BP32_UartRemote.bin`: GamePad32 to be used with the UartRemote library with official Lego apps.

## using webbrowser

Go to the [online ESP flashing tool](https://espressif.github.io/esptool-js/). Connect to the ESP32 by pressing the Connect button. Select the correct com port. When flashing this firmware for the first time, you can erase the flash now.
Select the firmware you want to flash from this directory and set the Flash Adress to `0x0`. Flash the image.

## using esptool

You can use the standard `esptool` to flash with the following command:

```
esptool.exe  --chip esp32 --port COM4 --baud 921600 write_flash -z 0x0 firmware_name>
```
