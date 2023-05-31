from pybricks.hubs import InventorHub
from pybricks.pupdevices import Motor
from pybricks.iodevices import PUPDevice
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch
import ustruct
hub = InventorHub()

p=PUPDevice(Port.A)

while True:
    a=p.read(0)
    outp=ustruct.unpack('6h',ustruct.pack('12b',*a))
    x1=(512+outp[0])//210
    y1=(512+outp[1])//210
    hub.display.off()
    hub.display.pixel(y1,x1,100)
    x1=(512+outp[2])//210
    y1=(512+outp[3])//210
    hub.display.pixel(y1,x1,70)
    wait(20)