from pybricks.hubs import PrimeHub
from pybricks.pupdevices import Motor, ColorSensor, UltrasonicSensor, ForceSensor
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch


hub = PrimeHub()
from pupremote import PUPRemoteHub
p=PUPRemoteHub(Port.A)
p.add_command('gpled','hhhhHH','16B')
p.add_command('gpsrv','hhhhHH','8H')



FILL = 0x10
ZERO = 0x20
SET  = 0x30
CONFIG = 0x40
WRITE = 0x80

cur_mode=0

def gamepad():
    if cur_mode==0:
        return p.call('gpled')
    elif cur_mode==1:
        return p.call('gpsrv')
    else:
        return None


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



neopixel_init(24,12) # set 24 pixel NeoPixel on pin GPIO12
neopixel_fill(30,0,0)
wait(300)
neopixel_zero()
wait(300)

st=StopWatch()
i=0
for x in range(1000):
    i+=1
    i%=20
    q=neopixel_set(i%5,3,[i,(i+6)%20,(i+12)%20]*3)
    print(q)
print(st.time())
st=StopWatch()
for x in range(1000):
    q=servo(0,x%180)
    print(q)
print(st.time())
