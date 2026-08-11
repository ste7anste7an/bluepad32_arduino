# BluePad libraries for Pybricks

This document covers the two BluePad Python libraries:

- **PUPRemote/LPF2 `bluepad.py` 2.1.0** from
  [AntonsMindstorms/PUPRemote](https://github.com/AntonsMindstorms/PUPRemote/blob/main/src/bluepad.py),
  read on 11 August 2026;
- **uRemote `bluepad_ur.py` 2.2.0-uremote** in this directory.

Both expose a `BluePad` class and preserve the familiar six-value gamepad
reading, but they are clients for different ESP32 firmware and are not drop-in
transport replacements.

## What is BluePad for Pybricks?

BluePad lets a Pybricks program use a Bluetooth game controller through an
ESP32 or LMS-ESP32 board. Bluepad32 runs on the ESP32 and handles the wireless
controller connection. The ESP32 is connected by cable to a LEGO hub port, and
the Python library presents the controller data to the Pybricks program.

There are two versions:

- **PUPRemote/LPF2 BluePad** makes the ESP32 appear as a LEGO Powered Up
  device. Pybricks accesses it through `PUPDevice`. This version provides the
  established six-field gamepad API and can control NeoPixels and servos
  connected to the ESP32.
- **uRemote BluePad** uses named uRemote commands instead of LPF2 modes. It
  keeps the common gamepad API and adds connection status, raw gamepad data,
  miscellaneous buttons, triggers, controller IMU data, I2C access, Bluetooth
  allow-list management, and explicit servo detach commands. It requires the
  compatible `uremote.py` client in the Pybricks project.

With either version, a Pybricks robot can be driven with the two joysticks,
react to face and direction-pad buttons, move servos, and control status or
decorative NeoPixels. The uRemote version can additionally use supported
controller motion sensors, communicate with I2C devices connected to the
ESP32, inspect the connected controller, and restrict connections to one
Bluetooth address.

## Installing the ESP32 firmware

The web installer is available at
[firmware.antonsmindstorms.com](https://firmware.antonsmindstorms.com/). Connect
the ESP32 or LMS-ESP32 to the computer with a USB data cable and select the
firmware that matches the Python library:

- choose **BluePad LPF2** for the PUPRemote/LPF2 `bluepad.py` library;
- choose **BluePad uRemote** for `bluepad_ur.py` and `uremote.py`.

Click **Flash firmware** in the selected row, choose the board's serial port
when the browser asks, and follow the installer until flashing is complete.
Restart or reconnect the board afterward. A Chromium-based browser with Web
Serial support, such as Chrome or Edge, is normally required. If a serial port
does not appear, check that the USB cable supports data and close programs
that may already have the port open.

The ESP32 firmware and Pybricks library must be from the same variant. An LPF2
library cannot communicate with the uRemote firmware, and `bluepad_ur.py`
cannot communicate with the LPF2 firmware.

## Browser configuration and debugging

Both variants have a USB web configurator. Connect the ESP32 directly to the
computer by USB, open the appropriate page in a Web Serial capable browser,
click **Connect**, and select the board's serial port. Close Arduino Serial
Monitor or any other program using that port first.

### LPF2/PUP configurator

[bluepad.antonsmindstorms.com](https://bluepad.antonsmindstorms.com/) is for the
BluePad LPF2 firmware used with SPIKE and Pybricks. It can:

- load the current configuration from the board;
- select the LEGO device emulation, including Color Sensor or Color Matrix;
- configure the NeoPixel count and GPIO pin;
- edit matrix/pixel mapping and LEGO color mappings;
- inspect the connected controller MAC address and configure Bluetooth
  filtering for one allowed controller;
- try settings temporarily, save them to flash, reset defaults, and test
  NeoPixels.

Use **Send &amp; try config** while testing. Use **Send &amp; save to flash** when the
configuration should survive a restart. The page may ask for a board reboot
after changing the emulated sensor type or when reconnecting to a hub.

### uRemote configurator

[bluepadur.antonsmindstorms.com](https://bluepadur.antonsmindstorms.com/) is for
the BluePad32 uRemote firmware. It provides configuration and live diagnostic
tools for:

- controller connection state, connected and allowed MAC addresses;
- an optional live gamepad display showing buttons, both joysticks,
  acceleration, and gyroscope values;
- the allowed controller, Bluetooth filter, new-connection setting, and saved
  Bluetooth configuration;
- runtime NeoPixel count and GPIO configuration, plus pixel, fill, and clear
  tests;
- all four servo channels, including angle tests and detach controls;
- I2C bus scanning and device read/write tests.

Live gamepad polling is off by default and can be enabled when debugging. In
the uRemote firmware, Bluetooth allow-list settings can be saved, while the
NeoPixel configuration is runtime-only and returns to the firmware defaults
after a restart.

## Which library to use

| ESP32 firmware | Pybricks file | Import | Additional file |
|---|---|---|---|
| BluePad32 PUPRemote/LPF2 | upstream `bluepad.py` | `from bluepad import BluePad` | none; it uses Pybricks `PUPDevice` |
| BluePad32 uRemote | local `bluepad_ur.py` | `from bluepad_ur import BluePad` | upload the compatible `uremote.py` client too |

Create an object in the same way for either transport:

```python
from pybricks.parameters import Port
from bluepad_ur import BluePad       # uRemote firmware

bp = BluePad(Port.A)
```

Only the import changes in the equivalent PUPRemote example.

## Common gamepad result

In both libraries, `bp.gamepad()` returns these six values in this order:

```text
(left_x, left_y, right_x, right_y, buttons, dpad)
```

The four axes are centered at 0 and are approximately -128..127. `buttons`
and `dpad` are bit masks. Both libraries also implement:

```python
bp.btns_pressed(buttons, nintendo=False)
bp.dpad_pressed(dpad, nintendo=False)
```

For the default mapping, face-button names are `X`, `O`, `[]`, and `V`. With
`nintendo=True`, the mapping is `B`, `A`, `Y`, `X`, `L`, `R`, `ZL`, and `ZR`.
Direction names are `L`, `R`, `U`, and `D`.

## Class method comparison

Legend: **same** means the public method name and basic purpose match;
**different** means callers must account for an API or behavior change;
**uRemote only** and **PUPRemote only** indicate an extension in one library.

| Method | Availability | Comparison |
|---|---|---|
| `BluePad(port)` | same | Both take a Pybricks `Port`. PUPRemote creates a `PUPDevice` and detects LPF2 sensor ID 61/64; uRemote creates `uRemote(port)`. |
| `gamepad(...)` | different | Both return the same six fields. PUPRemote accepts `mode=-1, deadzone=5`, clamps small axes to zero, and returns all zeroes when disconnected. uRemote accepts only `mode=-1`, ignores the mode for transport, and applies no deadzone. |
| `btns_pressed(buttons, nintendo=False)` | same | Same decoding and returned list of button names. |
| `dpad_pressed(dpad, nintendo=False)` | same | Same decoding and returned list of direction names. |
| `send(byte_vals)` | different | PUPRemote writes a legacy 9- or 16-byte LPF2 packet. uRemote accepts the old 16-byte NeoPixel packet format only as a compatibility adapter and translates recognized commands into RPC calls. Prefer the named uRemote methods. |
| `neopixel_init(count, pin)` | same purpose | LPF2 sends a configuration packet. uRemote calls `np_cfg`; valid firmware values are count 1..64 and GPIO 0..39. |
| `neopixel_fill(color, write=True)` | different | Both set every pixel. LPF2 honors `write=False` for deferred display. The current uRemote firmware always shows a fill, so its `write` argument is accepted but has no effect. |
| `neopixel_zero(write=True)` | different | Both clear the strip. uRemote always shows immediately; its `write` argument has no effect. |
| `neopixel_set(index, color, write=True)` | same purpose | Both validate against the configured count. uRemote sends the firmware `pix` command but currently always calls it with immediate display; its `write` argument has no effect. |
| `neopixel_set_multi(start, count, rgb_values, write=True)` | different | PUPRemote sends up to four pixels in one packet. uRemote has no multi-pixel firmware RPC and loops over `pix`, with no four-pixel limit. It therefore uses multiple UART round trips and is not atomic. |
| `servo(index, position, ...)` | different | PUPRemote has `zero_is_mid=True`: its default position range is -90..90 and 0 maps to physical 90 degrees. uRemote takes a direct 0..180 angle; 90 is midpoint, and 1000 detaches. It has no `zero_is_mid` parameter. |
| `servos(targets, zero_is_mid=True)` | PUPRemote only | Sends several servo targets. uRemote has no wrapper method, although raw `bp.call("servo", a0, a1, a2, a3)` sets four targets in one command. |
| `servo_off(index=None)` | uRemote only | Detaches one servo or all servos. |
| `call(command, *args)` | uRemote only | Calls any raw firmware RPC and returns its reply. |
| `ping()`, `version()`, `status()`, `connected()` | uRemote only | Device and connection information. |
| `pad(raw=False)` | uRemote only | Reads the packed packet. With `raw=True`, returns all ten bytes including connection, misc buttons, and triggers. |
| `joy(raw=False)`, `joyl(raw=False)`, `joyr(raw=False)` | uRemote only | Reads both, left, or right sticks. Default output is signed; `raw=True` returns firmware values 0..255. |
| `btn()` | uRemote only | Returns `buttons, dpad, misc`; unlike `gamepad()`, the button value can contain all 16 firmware button bits. |
| `imu()` | uRemote only | Returns three gyro and three acceleration values. |
| `i2c_scan()`, `i2c_read(...)`, `i2c_read_reg(...)` | uRemote only | Scans and reads the ESP32 I2C bus. |
| `i2c_write(...)`, `i2c_write_reg(...)` | uRemote only | Writes the ESP32 I2C bus and returns `(error, bytes_written)`. |
| `i2c_read_u8(...)`, `i2c_write_u8(...)` | uRemote only | Single-register convenience methods. |
| `bt_mac()`, `bluetooth_address()` | uRemote only | Reads the connected controller MAC. |
| `bt_allow(...)`, `bt_filter(...)`, `bt_clear()`, `bt_forget()` | uRemote only | Manages the one-address Bluetooth allow list and pairing keys. |
| `save()`, `load()`, `defaults()` | uRemote only | Manages persistent Bluetooth settings. |

### Important disconnection difference

PUPRemote `gamepad()` explicitly detects the firmware's disconnected sentinel
and returns `(0, 0, 0, 0, 0, 0)`. The uRemote firmware sends zero as each raw
axis when disconnected, and `bluepad_ur.py` converts it by subtracting 128.
Consequently, uRemote `gamepad()`, `joy()`, `joyl()`, and `joyr()` can report
`-128` axes while disconnected. Use `bp.connected()` first.

### Important servo conversion

These calls aim at the same physical midpoint:

```python
# PUPRemote bluepad.py: zero_is_mid defaults to True.
bp.servo(0, 0)

# uRemote bluepad_ur.py: direct physical angle.
bp.servo(0, 90)
```

To keep old 0..180 application values with PUPRemote, specify
`zero_is_mid=False`. To migrate a default PUPRemote target to uRemote, add 90
and constrain the result to the valid range.

## Module-level helper comparison

Both files also provide block-style functions backed by a global `BluePad`
instance initialized with `bluepad_init("A")`.

| Helper | PUPRemote | uRemote | Notes |
|---|:---:|:---:|---|
| `bluepad_init(port_letter, nintendo=True)` | yes | yes | uRemote uses `getattr(Port, port_letter)` instead of `eval`. The saved Nintendo flag is not automatically applied by the other helpers in either file. |
| four `get_*_stick_*()` helpers | yes | yes | Convert axes to an approximately -100..100 percentage; vertical signs are inverted. |
| `get_direction_pad()`, `get_buttons()` | yes | yes | Return raw masks from `gamepad()`. |
| `color_convert(...)` | yes | yes | Converts a Pybricks `Color` or RGB tuple and intensity. |
| `set_neopixel(...)`, `init_neopixel(...)`, `fill_neopixel(...)` | yes | yes | uRemote returns the underlying result. Its `fill_neopixel` intensity defaults to 1; upstream PUPRemote requires it. |
| `set_servo(...)` | yes | yes | Be careful: it calls the different servo semantics described above. |
| `gamepad()` | yes | yes | Returns the common six-field reading. |
| `servo_off(...)`, `joyl()`, `joyr()`, `imu()` | no | yes | uRemote extensions. |
| I2C helpers | no | yes | `i2c_scan/read/read_reg/write/write_reg`. |
| Bluetooth helpers | no | yes | `bt_mac`, `bt_allow`, `bt_filter`, `bt_clear`, `bt_forget`, `bt_save`, and `bt_load`. |

The uRemote class also contains `joy()` and several additional methods that do
not have module-level wrappers. Instantiate `BluePad` when the complete API is
needed.

## Pybricks example: PUPRemote/LPF2 firmware

Upload the current upstream `bluepad.py` to the Pybricks project. This example
reads the sticks and buttons while safely handling disconnection. PUPRemote's
library applies its default deadzone of 5.

```python
from pybricks.parameters import Port
from pybricks.tools import wait
from bluepad import BluePad

bp = BluePad(Port.A)

while True:
    lx, ly, rx, ry, buttons, dpad = bp.gamepad()

    # The PUPRemote library returns this complete zero tuple when disconnected.
    if (lx, ly, rx, ry, buttons, dpad) == (0, 0, 0, 0, 0, 0):
        print("centered or disconnected")
    else:
        print("sticks:", lx, ly, rx, ry)
        print("buttons:", bp.btns_pressed(buttons))
        print("dpad:", bp.dpad_pressed(dpad))

    wait(20)
```

The all-zero result is also valid when a connected controller is centered and
no buttons are held, so this transport cannot distinguish those two states
through `gamepad()` alone.

## Pybricks example: uRemote firmware

Upload `bluepad_ur.py` and the compatible `uremote.py` to the Pybricks project.
This example uses the explicit uRemote connection state and reads the extended
button tuple as well as the common six-field value.

```python
from pybricks.parameters import Port
from pybricks.tools import wait
from bluepad_ur import BluePad

bp = BluePad(Port.A)

while True:
    if bp.connected():
        lx, ly, rx, ry, buttons, dpad = bp.gamepad()
        buttons16, dpad8, misc = bp.btn()

        print("sticks:", lx, ly, rx, ry)
        print("buttons:", bp.btns_pressed(buttons16))
        print("dpad:", bp.dpad_pressed(dpad8))
        print("misc mask:", misc)
    else:
        print("waiting for gamepad")

    wait(20)
```

For fewer UART calls per loop, `bp.pad(raw=True)` returns connection state,
sticks, buttons, direction pad, miscellaneous buttons, and both triggers in a
single 10-byte response.

## Migration checklist

When moving a project between these libraries:

1. Flash the matching LPF2/PUPRemote or uRemote ESP32 firmware.
2. Change the import between `bluepad` and `bluepad_ur`.
3. Keep `gamepad()`, `btns_pressed()`, and `dpad_pressed()` for the common path.
4. Add application-side deadzone handling when moving to uRemote if needed.
5. Convert servo targets because the default angle origins differ.
6. Do not rely on deferred NeoPixel writes with the current uRemote wrapper.
7. Use `connected()` in uRemote code before interpreting axis values.
