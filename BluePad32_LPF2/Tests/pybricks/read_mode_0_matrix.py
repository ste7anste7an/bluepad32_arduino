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
while True:
    matrix.on(Color.RED)
    matrix.on(Color.GREEN)
    wait(1000)
    print([sign_to_unsign(i) for i in p.read(2)])