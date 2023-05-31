from pybricks.hubs import TechnicHub
from pybricks.pupdevices import Motor
from pybricks.iodevices import PUPDevice
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch
import ustruct
servos=[0,0,0,0]
p=PUPDevice(Port.A)

def read_gamepad():
    a=p.read(0)
    outp=ustruct.unpack('6h',ustruct.pack('12b',*a))
    return outp

def led(nr,r,g,b):
    p.write(0,(0,0,0,0,0,0,0,0,nr,r,g,b))

def showled():
    p.write(0,(0,0,0,0,0,0,0,0,65,0,0,0))

def servo(nr,pos):
    servos[nr]=pos
    p.write(0,(servos[0],0,servos[1],0,servos[2],0,servos[3],0,0,0,0,0))


while True:

    for l in range(6):
        for i in range(6):
            led(i,0,0,0)
            #wait(2)
        led(l,30,0,0)
        showled()
        print(read_gamepad())
        wait(20)    
    for l in range(4,0,-1):
        for i in range(6):
            led(i,0,0,0)
            #wait(2)
        led(l,30,0,0)
        showled()
        print(read_gamepad())
        wait(20)
