# Bluepad32 example for pybricks

Flash the firmware on the ESP32 (use the firmware in this guthub), or go to [firmware.antonsmindstorms.com](https://firmware.antonsmindstorms.com) and choose `firmware_BluePad32_Pybricks`.

## PUPRemote

The BuePad32 LPF2 can be used directory from `PUPDevice` or using our [PUPRemote library](https://github.com/antonvh/PUPRemote). 

```
from pupremote import PUPRemoteHub
p=PUPRemoteHub(Port.A)
p.add_command('gpled','hhhhHH','16B')
p.add_command('gpsrv','hhhhHH','8H')


while True:
  (gp_left_x,gp_left_y,gp_right_x,gp_right_y,buttons,dpad)=p.call('gpled')
```

for writing values to the servo's and LED's, use:
```
FILL = 0x10
ZERO = 0x20
SET  = 0x30
CONFIG = 0x40
WRITE = 0x80

cur_mode=0
def neopixel_init(nr_leds,pin):
    global cur_mode
    leds=[0]*16
    leds[0]=CONFIG
    leds[1]=nr_leds
    leds[2]=pin
    r=p.call('gpled',*leds)
    cur_mode=0
    return reversed

def neopixel_fill(r,g,b,write=True):
    global cur_mode
    leds=[0]*16
    leds[0]=FILL|WRITE if write else FILL
    leds[1:4]=[r,g,b]
    r=p.call('gpled',*leds)
    cur_mode=0
    return r

def neopixel_zero(write=True):
    global cur_mode
    leds=[0]*16
    leds[0]=ZERO|WRITE if write else FILL
    r=p.call('gpled',*leds)
    cur_mode=0
    return r

def neopixel_set(start_led,nr_led,led_arr,write=True):
    global cur_mode
    leds=[0]*16
    leds[0]=SET|WRITE if write else FILL
    leds[1]=nr_led
    leds[2]=start_led
    if len(led_arr)==3*nr_led:
        leds[3:3+nr_led*3]=led_arr
        r=p.call('gpled',*leds)
    else:
        print("error neopixel_set: led_nr does not correspons with led_arr")
        r=None
    cur_mode=0
    return r

def servo(servo_nr,pos):
    global cur_mode
    s=[0]*4
    s[0]=x%180
    r=p.call('gpsrv',*s)
    cur_mode=1
    return r

```
