from pybricks.hubs import TechnicHub
from pybricks.pupdevices import Motor
from pybricks.iodevices import PUPDevice
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch
import ustruct
servos=[0,0,0,0]
p=PUPDevice(Port.A)

def gamepad():
    a=p.read(0)
    outp=ustruct.unpack('6h',ustruct.pack('12b',*a))
    return outp

def led(nr,r,g,b):
    p.write(0,(0,0,0,0,0,0,0,0,nr,r,g,b))

def showleds():
    p.write(0,(0,0,0,0,0,0,0,0,65,0,0,0))

def clearleds():
    p.write(0,(0,0,0,0,0,0,0,0,67,0,0,0))

def initled(nr,pin):
    p.write(0,(0,0,0,0,0,0,0,0,66,nr,pin,0))


def servo(nr,pos):
    servos[nr]=pos
    p.write(0,(servos[0],0,servos[1],0,servos[2],0,servos[3],0,0,0,0,0))

s=StopWatch()
initled(6,12) # we use 6 neopixels connected to pin 12
while True:
    s.reset()
    for l in range(6):
        clearleds()     # clear all leds
        led(l,30,0,0)   # set led <l> to (r,g,b)=(30,0,0)
        showleds()
        wait(20)    
    for l in range(4,0,-1):
        clearleds()     # clear all leds
        led(l,30,0,0)   # set led <l> to (r,g,b)=(30,0,0)
        showleds()
        wait(20)
    print(s.time())
    print(gamepad())
