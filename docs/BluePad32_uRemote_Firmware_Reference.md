# BluePad32 uRemote firmware reference

`BluePad32_uRemote.ino` turns an ESP32 or LMS-ESP32 into a Bluepad32 gamepad,
NeoPixel, servo, I2C, and Bluetooth bridge. A Pybricks hub calls the named
commands below over the uRemote UART connection.

## Connection and hardware

The firmware uses `Serial2` at **115200 baud, 8 data bits, no parity, one stop
bit**.

| Board | ESP32 RX | ESP32 TX |
|---|---:|---:|
| Generic ESP32 | GPIO 18 | GPIO 19 |
| LMS-ESP32 v2 / ESP32-PICO-V3-02 | GPIO 8 | GPIO 7 |

RX and TX are named from the ESP32's point of view. The hub's transmit line
therefore connects to ESP32 RX, and the hub's receive line connects to ESP32
TX. The I2C bus uses SDA GPIO 5 and SCL GPIO 4.

The default NeoPixel configuration is 16 pixels on GPIO 12. Servo outputs are:

| Servo | Generic ESP32 / LMS-ESP32 v1 | LMS-ESP32 v2 |
|---:|---:|---:|
| 0 | GPIO 21 | GPIO 19 |
| 1 | GPIO 22 | GPIO 20 |
| 2 | GPIO 23 | GPIO 21 |
| 3 | GPIO 25 | GPIO 22 |

Only the first connected gamepad is exposed by the commands.

## Calling a command from Pybricks

Upload the Pybricks `uremote.py` client with the project, then create a client
on the hub port connected to the ESP32:

```python
from pybricks.parameters import Port
from uremote import uRemote

ur = uRemote(Port.A)
print(ur.call("ver"))
print(ur.call("joyl"))
```

Commands return one value directly or a tuple/list of values, depending on the
number of results and the uRemote client version. The higher-level
`bluepad_ur.py` library normalizes these differences.

## Device information

| Command | Arguments | Result | Function |
|---|---|---|---|
| `ping` | none | ESP32 `millis()` value | Checks communication and reports time since boot in milliseconds. |
| `ver` | none | string | Returns the BluePad32 uRemote build string and Bluepad32 firmware version. |
| `status` | none | `connected, pixel_count, pixel_gpio, bt_filter` | Reports gamepad connection state and current runtime configuration. Boolean states are returned as `0` or `1`. |

## Gamepad commands

Joystick values sent by the firmware are unsigned bytes from 0 to 255. A
connected, centered stick is approximately 128. The higher-level library
subtracts 128 and presents approximately -128 to 127.

| Command | Arguments | Result | Function |
|---|---|---|---|
| `pad` | none | one 10-byte object | Returns the complete compact gamepad packet described below. |
| `joy` | none | `lx, ly, rx, ry` | Reads both analog sticks as four 0..255 integers. |
| `joyl` | none | `lx, ly` | Reads the left stick as two 0..255 integers. |
| `joyr` | none | `rx, ry` | Reads the right stick as two 0..255 integers. |
| `btn` | none | `buttons, dpad, misc` | Reads a 16-bit button mask, 8-bit direction-pad mask, and 8-bit miscellaneous-button mask. |
| `imu` | none | `gyro_x, gyro_y, gyro_z, accel_x, accel_y, accel_z` | Reads the controller motion sensors. Availability and units depend on the controller and Bluepad32 parser. |

The `pad` byte layout is:

| Offset | Field | Value |
|---:|---|---|
| 0 | connected | `1` when a gamepad is connected, otherwise `0` |
| 1 | left X | 0..255 |
| 2 | left Y | 0..255 |
| 3 | right X | 0..255 |
| 4 | right Y | 0..255 |
| 5 | buttons | low 8 bits of the button mask |
| 6 | dpad | low 8 bits of the direction-pad mask |
| 7 | misc | low 8 bits of the miscellaneous-button mask |
| 8 | brake | trigger value, 0..255 |
| 9 | throttle | trigger value, 0..255 |

When no controller is connected, `pad` contains ten zero bytes; `joy`, `joyl`,
`joyr`, `btn`, and `imu` return numeric zeroes. Check `status`, byte 0 of raw
`pad`, or `BluePad.connected()` before interpreting axes. In particular,
`bluepad_ur.py` converts an unsigned zero axis to `-128`.

Button bits follow Bluepad32's standardized masks. The friendly
`btns_pressed()` and `dpad_pressed()` helpers in `bluepad_ur.py` decode their
common face-button and direction bits.

## NeoPixel commands

Color components are integers from 0 to 255. NeoPixel configuration is
runtime-only and is not stored in EEPROM.

| Command | Arguments | Result | Function |
|---|---|---|---|
| `np_cfg` | optional `count`, optional `gpio` | `count, gpio` | Rebuilds the strip using 1..64 pixels and GPIO 0..39. Omitted values retain their current setting. The strip is cleared. |
| `pix` | `index, r, g, b`, optional `show` | `1` | Sets one pixel. `show` defaults to true; false defers transmission. An out-of-range index is ignored but still returns `1`. |
| `fill` | `r, g, b` | `1` | Sets every configured pixel and immediately shows the result. |
| `clear` | none | `1` | Clears and immediately shows the strip. |

## Servo commands

| Command | Arguments | Result | Function |
|---|---|---|---|
| `servo` | `index, angle` | `1` | Sets servo 0..3. Normal angles are clipped by the firmware to 0..180 degrees. Angle `1000` detaches the output. |
| `servo` | `angle0, angle1, angle2, angle3` | `1` | Sets all four servo outputs in one call. |
| `servo_off` | optional `index` | `1` | Detaches one servo, or all four if the index is omitted. |

## I2C commands

Addresses are 7-bit I2C addresses. Read lengths are clamped to 0..128 bytes.
Data may be supplied as one `bytes` object or as individual numeric byte
arguments, subject to the uRemote argument limit.

| Command | Arguments | Result | Function |
|---|---|---|---|
| `i2c_scan` | none | `count, addresses` | Scans the bus; `addresses` is a bytes object containing the detected addresses. |
| `i2c_read` | `address, length` | bytes | Reads raw bytes from a device. The returned object may be shorter than requested. |
| `i2c_read_reg` | `address, register, length` | bytes | Writes one register byte without a stop, then reads bytes from the device. |
| `i2c_write` | `address, data` | `error, bytes_written` | Writes raw data. |
| `i2c_write_reg` | `address, register, data` | `error, bytes_written` | Writes a register byte followed by data. |

For writes, `error == 0` means that the Arduino `Wire` transaction was
acknowledged. `bytes_written` is the number of data bytes submitted; it does
not include the register byte in `i2c_write_reg`.

## Bluetooth and persistent settings

The allow list contains one Bluetooth address. MAC strings must contain six
two-digit hexadecimal bytes separated by colons, for example
`"AA:BB:CC:DD:EE:FF"`.

| Command | Arguments | Result | Function |
|---|---|---|---|
| `bt_mac` | none | MAC string | Returns the connected controller address, or `00:00:00:00:00:00` when disconnected. |
| `bt_allow` | none | MAC string | Reads the configured allowed address. |
| `bt_allow` | MAC string | MAC string | Replaces the allowed address and applies it to the runtime allow list. |
| `bt_filter` | none | `0` or `1` | Reads whether allow-list filtering is enabled. |
| `bt_filter` | boolean/number | `0` or `1` | Disables or enables allow-list filtering immediately. |
| `bt_clear` | none | `1` | Clears the runtime allowed address and disables filtering. |
| `bt_forget` | none | `1` | Erases the Bluetooth pairing keys managed by Bluepad32. |
| `save` | none | `1` | Stores the allowed address and filter flag in EEPROM. |
| `load` | none | `1` | Reloads those settings from EEPROM and applies them. |
| `defaults` | none | `1` | Resets the settings in RAM and applies them; call `save` to persist the reset. |

Changing `bt_allow`, `bt_filter`, or `bt_clear` does not write EEPROM until
`save` is called. `bt_forget` affects pairing keys, which are separate from
the uRemote allow-list settings.

## Errors and protocol limits

Bad argument counts, invalid ranges, and unknown command names produce a
uRemote error response. The Python client's `call()` normally raises or
reports that remote error according to the installed uRemote version.

The bundled Arduino uRemote implementation has these limits:

- command name: at most 31 bytes;
- arguments per frame: at most 8;
- one argument: at most 128 bytes;
- complete frame: at most 255 bytes.

The wire format supports byte arrays (`A`), booleans (`B`), decimal numbers
(`N`), and strings (`S`). Applications should normally use `uremote.py` or
`bluepad_ur.py` instead of constructing frames directly.

## Minimal raw-command example

```python
from pybricks.parameters import Port
from pybricks.tools import wait
from uremote import uRemote

ur = uRemote(Port.A)

while True:
    connected, pixel_count, pixel_pin, filtering = ur.call("status")
    if connected:
        lx, ly, rx, ry = ur.call("joy")
        buttons, dpad, misc = ur.call("btn")
        print(lx, ly, rx, ry, buttons, dpad, misc)
    wait(20)
```

