from pybricks.hubs import TechnicHub
from pybricks.pupdevices import Motor
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch
import ustruct

hub = TechnicHub()

from pybricks.iodevices import PUPDevice
p=PUPDevice(Port.A)

def sign_to_unsign(a):
    return ustruct.unpack('H',ustruct.pack('h',a))
cnt=0
d=0
while 1:
    cnt+=1
    if cnt>20:

        cnt=0
        d+=1
        d%=256
        p.write(0,([(d+i)*257 for i in range(8)]))
    q=p.read(0)
    [c,ref,r,g,b,i,_,_]=[hex(sign_to_unsign(i)) for i in q]
    print([c,ref,r,g,b,i,_,_])
    print([hex(sign_to_unsign(i)) for i in p.read(1)])