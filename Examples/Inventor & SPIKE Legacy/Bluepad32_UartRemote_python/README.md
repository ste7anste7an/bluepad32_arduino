# Bluepad32 UartRemote for Mindstorms Inventor and SPIKE Prime Legacy Python

## Install mpy_robot_tool on hub

Before using these examples, you should install the `mpy_robot_tool` libraries on the hub. These contain the uartRemote library necessary to communicate with Bluepad32 UartRemote.

## Simple gamepad example

The example `bluepad_read_gamepad.py` shows how to read all values for joystick and buttons from a connected gamepad.

## commands available in Bleupad32 uartremote

```
ur.call('reset')
```

Resets the Bluepad32 firmware

```
ur.call('debug','B',1)
```

Enables debug on UART

```
(ack, gamepad_vals) = ur.call('gamepad')
```

reads `values=(buttons,dpad, left_x,left_y,right_x,right_y)` from gamepad

```
(ack,connected)=ur.call('connected')
```

Returns the connection state of Bluetooth Gamepad

```
ur.call("btaddress","B",0)
```

Returns bluetooth address of connected game controller.

```
ur.call('btdisconnect','B',0)
```

Disconnects current game controller.

```
ur.call('btforget')
```

Forgets current blueooth address from white list.

```
ur.call('btallow','17s',b'AA:BB:CC:DD:EE:FF'))
```

Whitelists game controller with given bluetooth address.

```
ur.call('btfiltered','B',1)
```

Enables whitelisting blueooth addresses lof game controllers.

```
ur.call('i2c_scan')
```

Returns bytearray with results of an I2C scan.

```
ur.call("i2c_read","2B",address,len)
```

Reads `len` bytes from i2C device with address `address`.

```
ur.call("i2c_read_reg","3B",address,reg,len)
```

Reads `len` bytes from i2C device with address `address` from register `reg`.

```
ur.call("neopixel_init","2B",nr_leds,pin)
```

Initializes neopixel of `nr_leds` on GPIO pin `pin`.

```
ur.call("neopixel","4B",nr,r,g,b)
```

Set pixel `nr` to color `(r,g,b)'. Not shown until calling `neoixel_show`.

```
ur.call("neopixel_show")
```

Shows neopixel with current colors.

```
ur.call("servo",">BI",servo_nr,servo_pos)
```

Turns servo motor `servo_nr` to an angle `servo_pos`.
