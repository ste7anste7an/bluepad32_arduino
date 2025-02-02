from pybricks.hubs import TechnicHub
from pybricks.parameters import Color, Port
from pybricks.pupdevices import ColorLightMatrix
from pybricks.tools import wait
import ustruct

# Set up all devices.
technic_hub = TechnicHub()
matrix = ColorLightMatrix(Port.A)
from pybricks.iodevices import PUPDevice
p=PUPDevice(Port.A)


def sign_to_unsign(a):
    return ustruct.unpack('B',ustruct.pack('b',a))[0]

# The main program starts here.
for i in range(100):
    matrix.on(Color.RED)
    wait(500)
    matrix.on(Color.GREEN)
    wait(500)
    q= p.read(2)
    print(q[:4],sign_to_unsign(q[4]),sign_to_unsign(q[5]))