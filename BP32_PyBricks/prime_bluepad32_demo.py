from pybricks.hubs import PrimeHub
from pybricks.pupdevices import Motor, ColorSensor, UltrasonicSensor, ForceSensor
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch

hub = PrimeHub()
from pupremote import PUPRemoteHub
p=PUPRemoteHub(Port.A)
p.add_command('gpled','hhhhHH','16B')



while True:
    outp=p.call('gpled')
    x1=(512+outp[0])//210
    y1=(512+outp[1])//210
    hub.display.off()
    hub.display.pixel(y1,x1,100)
    x1=(512+outp[2])//210
    y1=(512+outp[3])//210
    hub.display.pixel(y1,x1,70)
    wait(20)