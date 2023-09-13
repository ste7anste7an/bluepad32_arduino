# Bluepad32 example for pybricks

Flash the firmware on the ESP32 (use the firmware in this guthub), or go to [firmware.antonsmindstorms.com](https://firmware.antonsmindstorms.com) and choose `firmware_BluePad32_Pybricks`.

## PUPRemote

The BuePad32 LPF2 can be used directory from `PUPDevice` or using our [PUPRemote library](https://github.com/antonvh/PUPRemote). 

```
p=PUPRemoteHub(Port.A)

p.add_command('gmpd',"hhhhHH","HHHHHHHH")

while True:
  (gp_left_x,gp_left_y,gp_right_x,gp_right_y,buttons,dpad)=p.call('gmpd')
```

for writing values to the servo's and LED's, use:
```
p.call('gmpd', servo0, servo1, servo2, servo3, led_val0, led_val1, led_val2,led_val3)
```
