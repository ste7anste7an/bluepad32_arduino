# Implemented UartRemote commands in the BluePad32_UartRemote firmware

This is a firmware that supported BluePad32. Furthermore, the following commands are integrated in the firmware:

```
    uartremote.add_command("connected",&connected);
    uartremote.add_command("gamepad",&gamepad);
    uartremote.add_command("gamepad",&gamepad);
    uartremote.add_command("btaddress", &btaddress);
    uartremote.add_command("btdisconnect", &btdisconnect);
    uartremote.add_command("btallow", &btallow);
    uartremote.add_command("btforget", &btallow);
    uartremote.add_command("btfilter", &btfilter);    
    uartremote.add_command("i2c_scan", &i2c_scan);
    uartremote.add_command("i2c_read", &i2c_read);
    uartremote.add_command("i2c_read_reg", &i2c_read_reg);
    uartremote.add_command("neopixel",&neopixel);
    uartremote.add_command("neopixel_show",&neopixel_show);
    uartremote.add_command("neopixel_init",&neopixel_init);
    uartremote.add_command("servo",&servo);
```

## Calling from Lego SPIKE Prime or Robot Invertor

**`ur.call('connected')`**

Check whether a gamepad is connected. Returns 1 when connected

**'ur.call('gamepad')`**

Returns the status of the gamepad with 6 parameters: `myGamepad->buttons(),myGamepad->dpad(),myGamepad->axisX(),myGamepad->axisY(),myGamepad->axisRX(),myGamepad->axisRY())`

**'ur.call('btaddress','B',idx)`**

Returns the Bluetooth address of the controller connected as index `idx` as a string in the format `'AA:BB:CC:11:22:33'`

**'ur.call('btdisconnect','B',idx)`**

Disconnectes the controller connect to index `idx`.

**'ur.call('btallow','17s',bluetooth_address)`**

Confgures the bluetooth address `bluetooth_address` (given as a string in the format `'AA:BB:CC:11:22:33'`) to be used as a filter for controllers to be connected. Dependoing on the `btfilter` setting, the filter will be active.

**'ur.call('btfilter','B',filter_active)`**

Activaes the bluetooth filter. Values are `0` (not active) or `1` (active). 

**`ur.call('i2c_scan')`**

Returns the addresses of connected i2c devices. Note: returns a byte array.

**`ur.call('i2c_read','2B',address,len)`**

Reads `len` bytes from i2c device (connected to the Grove port) at address `adress`

**`ur.call("neopixel","4B",led_nr,red,green,blue)`**

Sets led number `led_nr` to color `(red,greem,blue)`. Use led_show to display the leds.

**`ur.call('neopixel_show)`**

Shows current led configuration.

**`ur.call('neopixel_init','2B',number_leds,pin)`**

Initates NeoPixel with `number_leds` leds on Pin `pin`.

**`ur.call('servo','>Bi',servo_nr,angle)`**

Sets servo number `servo_nr` to position `pos`. Mapping is servo 1,2,3, and 4 on pins 21,22,23, and 25. The possible `angle` is usually between `0` and `180`.
