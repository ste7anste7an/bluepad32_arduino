from time import sleep_ms
from projects.uartremote import *
ur=UartRemote("A")
while True:
    ack,gamepad=ur.call('gamepad')
    print(gamepad)
    sleep_ms(50)