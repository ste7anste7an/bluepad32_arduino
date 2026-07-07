# BluePad32 uRemote conversion

This folder contains a uRemote-based conversion of `BluePad32_LPF2.ino`.
It removes the LPF2 UART sensor emulation and exposes the useful gamepad, LED, servo, Bluetooth, and I2C functions as named uRemote commands.

The color-sensor packet commands, matrix packet commands, matrix cell mapping, LEGO color palette, and logical `sensor()` mode commands have been removed.

## Files

- `BluePad32_uRemote.ino` - Arduino sketch for the ESP32/LMS-ESP32 BluePad32 board.
- `uRemote.h` / `uRemote.cpp` - copied from the supplied Arduino uRemote library for convenience. `UREMOTE_MAX_ARG_LEN` is set to 128 so I2C scans and larger I2C byte transfers fit in one reply.
- `Pybricks_BtAllowList.py` - Pybricks example showing joystick reads and Bluetooth allow-list setup.

## UART

The sketch starts uRemote on `Serial2` at 115200 baud.

Default ESP32 pins:

- RX: GPIO 18
- TX: GPIO 19

LMS-ESP32 v2 / ESP32-PICO-V3-02 pins:

- RX: GPIO 8
- TX: GPIO 7

## I2C

The sketch initializes I2C with:

```cpp
Wire.begin(5, 4);  // SDA = GPIO 5, SCL = GPIO 4
```

I2C commands are intended for small device-control calls from Pybricks, SPIKE, EV3, or MicroBlocks.

## Commands

Gamepad reads:

- `ping()` - returns `millis()`.
- `ver()` - firmware string.
- `status()` - connected, NeoPixel count, NeoPixel pin, BT filter.
- `pad()` - one 10-byte array: connected, lx, ly, rx, ry, buttons, dpad, misc, brake, throttle.
- `joy()` - lx, ly, rx, ry as 0..255 numbers.
- `joyl()` - left joystick only: lx, ly as 0..255 numbers.
- `joyr()` - right joystick only: rx, ry as 0..255 numbers.
- `btn()` - buttons, dpad, misc as numbers.
- `imu()` - gyroX, gyroY, gyroZ, accelX, accelY, accelZ. Returns six zeroes when no gamepad is connected.

NeoPixel:

- `pix(n, r, g, b)` - set one pixel.
- `fill(r, g, b)` - fill all pixels.
- `clear()` - clear all pixels.
- `np_cfg(count, gpio)` - set NeoPixel count and pin for the current runtime only. This is not saved to EEPROM.

Servos:

- `servo(n, angle)` - set servo 0..3 to 0..180 degrees. Use angle `1000` to detach that servo.
- `servo(a0, a1, a2, a3)` - set all four servos.
- `servo_off()` - detach all servos.
- `servo_off(n)` - detach one servo.

I2C:

- `i2c_scan()` - returns `count, addresses`, where `addresses` is a byte array of detected 7-bit addresses.
- `i2c_read(address, len)` - reads `len` bytes from `address`; returns a byte array. `len` is clamped to 0..128.
- `i2c_read_reg(address, reg, len)` - writes register byte `reg`, then reads `len` bytes; returns a byte array.
- `i2c_write(address, data)` - writes bytes to `address`; returns `error, bytes_written`. `data` can be one bytes object or multiple numeric byte arguments.
- `i2c_write_reg(address, reg, data)` - writes register byte `reg` followed by bytes; returns `error, bytes_written`. `data` can be one bytes object or multiple numeric byte arguments.

For write commands, `error == 0` means the I2C transaction was acknowledged.

Bluetooth:

- `bt_mac()` - current connected gamepad MAC address, returned as `XX:XX:XX:XX:XX:XX`.
- `bt_allow("xx:xx:xx:xx:xx:xx")` - set the one allowed Bluetooth address. Hex digits may be upper or lower case, but colons are required.
- `bt_allow()` - read the allowed Bluetooth address.
- `bt_filter(0_or_1)` - set Bluetooth allow-list filtering.
- `bt_filter()` - read filtering state.
- `bt_clear()` - clear the allow list and disable filtering.
- `bt_forget()` - forget Bluetooth keys.

EEPROM:

EEPROM stores only Bluetooth settings:

- allowed Bluetooth address
- allow-list filter enabled/disabled

Commands:

- `save()` - save Bluetooth settings to EEPROM.
- `load()` - reload Bluetooth settings from EEPROM and apply them.
- `defaults()` - reset Bluetooth settings in RAM to defaults. Call `save()` afterward to persist the reset.

## Pybricks example: gamepad, IMU, I2C scan, and Bluetooth allow list

This example reads the currently connected controller MAC address, stores it as the allowed address, enables the filter, and saves those Bluetooth settings to EEPROM.

```python
from pybricks.parameters import Port
from pybricks.tools import wait
from uremote import uRemote

ur = uRemote(Port.A)


def first(value):
    # Some uRemote versions return a single value directly, while others
    # return a tuple/list even when there is only one response value.
    if isinstance(value, (tuple, list)):
        return value[0]
    return value


print("version:", ur.call("ver"))
print("status:", ur.call("status"))

# Read controller values.
print("all sticks:", ur.call("joy"))
print("left stick:", ur.call("joyl"))
print("right stick:", ur.call("joyr"))
print("buttons:", ur.call("btn"))
print("imu:", ur.call("imu"))

# Scan I2C devices on SDA GPIO 5 / SCL GPIO 4.
count, addresses = ur.call("i2c_scan")
print("i2c devices:", count, list(addresses))

# Example I2C register read/write calls. Change address/register for your device.
# whoami = ur.call("i2c_read_reg", 0x68, 0x75, 1)
# err, written = ur.call("i2c_write_reg", 0x68, 0x6B, bytes([0x00]))

# Use the currently connected gamepad as the only allowed controller.
mac = first(ur.call("bt_mac"))
print("connected gamepad:", mac)

if mac != "00:00:00:00:00:00":
    print("setting allow list")
    ur.call("bt_allow", mac)
    ur.call("bt_filter", True)
    ur.call("save")

print("allowed:", ur.call("bt_allow"))
print("filter:", ur.call("bt_filter"))

# LED test.
ur.call("pix", 0, 0, 40, 0)
wait(500)
ur.call("clear")
```

## Pybricks example: disable and clear the allow list

```python
from pybricks.parameters import Port
from uremote import uRemote

ur = uRemote(Port.A)

ur.call("bt_filter", False)
ur.call("bt_clear")
ur.call("save")

print("allowed:", ur.call("bt_allow"))
print("filter:", ur.call("bt_filter"))
```

## Notes

- `bt_allow()` expects a single string in the form `xx:xx:xx:xx:xx:xx`; the older six-number form has been removed.
- `joyl()` and `joyr()` replace the earlier `jou_l()` and `jou_r()` names.
- `np_cfg()` is runtime-only because EEPROM is reserved for Bluetooth settings.
- The removed commands are: `col`, `refl`, `rgb`, `hsv`, `matpkt`, `mat`, `map`, `pal`, and `sensor`.
