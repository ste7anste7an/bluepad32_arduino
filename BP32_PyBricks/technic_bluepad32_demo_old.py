from pybricks.hubs import TechnicHub
from pybricks.pupdevices import Motor
from pybricks.iodevices import PUPDevice
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch
import ustruct
servos=[0,0,0,0]

servo_cur=0
servo_delta=10

p=PUPDevice(Port.A)

def gamepad():
    a=p.read(0)
    outp=ustruct.unpack('6h',ustruct.pack('12b',*a))
    return outp

def write_servo_led(nr,r,g,b):
    p.write(0,(servos[0],0,servos[1],0,servos[2],0,servos[3],0,nr,r,g,b))

def led(nr,r,g,b):
    write_servo_led(nr,r,g,b)

def showleds():
    write_servo_led(65,0,0,0)

def clearleds():
    write_servo_led(67,0,0,0)

def initled(nr,pin):
    write_servo_led(66,nr,pin,0)


def servo(nr,pos):
    servos[nr]=pos
    p.write(0,(servos[0],0,servos[1],0,servos[2],0,servos[3],0,0,0,0,0))

s=StopWatch()
initled(6,12)
while True:
    s.reset()
    for l in range(6):
        clearleds()
        led(l,30,0,0)
        showleds()
        wait(20)    
        print(gamepad())
    for l in range(4,0,-1):
        clearleds()
        led(l,30,0,0)
        showleds()
        wait(20)
        print(gamepad())
    print(s.time())
    
    servo_cur+=servo_delta
    if servo_cur==180 or servo_cur==0:
        servo_delta=-servo_delta
    servo(1,servo_cur)
    print(servo_cur,servo_delta)
    
