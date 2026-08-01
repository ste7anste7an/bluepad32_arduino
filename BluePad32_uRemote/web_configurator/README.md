# BluePad32 Configurator

This Web Serial page configures and tests Bluetooth, NeoPixels, four servos,
and I2C devices through the USB `Serial` interface in
`BluePad32_uRemote.ino`.

Serve the repository root so the shared logo from the LPF2 configurator is
available:

```text
python -m http.server 8000
```

Then open `http://localhost:8000/BluePad32_uRemote/web_configurator/` in a
current Chrome or Edge browser. Web Serial requires a secure context;
localhost is treated as secure.

The page uses these commands:

- `GET BT_CON`
- `GET BT_MAC`
- `GET BT_ALLOW`
- `GET BT_FILTER`
- `GET BT_ALLOW_NEW`
- `GET GAMEPAD`
- `SET BT_ALLOW ...`
- `SET BT_FILTER ...`
- `SET BT_ALLOW_NEW ...`
- `SET BT_CLEAR_ALLOW_LIST`
- `SAVE`
- `GET NP_NR`
- `GET NP_GPIO`
- `SET NP_NR ...`
- `SET NP_GPIO ...`
- `NEOPIXEL SET ...`
- `NEOPIXEL FILL ...`
- `NEOPIXEL CLEAR`
- `GET SERVO`
- `SERVO SET ...`
- `SERVO OFF ...`
- `I2C SCAN`
- `I2C READ ...`
- `I2C READ_REG ...`
- `I2C WRITE ...`
- `I2C WRITE_REG ...`

NeoPixel count and GPIO are runtime-only settings and return to the firmware
defaults after a restart.

I2C registers are entered as decimal values. I2C scan addresses are displayed
with a hexadecimal prefix (for example, `0x33`). I2C write data and all
received data are displayed as hexadecimal bytes.

The live gamepad panel is disabled by default. When enabled, it polls
`GET GAMEPAD` for joystick, button, D-pad, accelerometer, and gyroscope data
while the USB serial connection is open. Disabling the panel stops polling.
