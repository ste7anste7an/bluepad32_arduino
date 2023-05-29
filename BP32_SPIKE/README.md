from uartremote import *
u=UartRemote('A')
u.call('btaddress','B',0)
u.call("btallow","17s","B8:8A:EC:F2:68:3A")
u.call("btfilter","B",1)
u.call('btaddress','B',0)
u.call('btdisconnect','B',0)
u.call('gamepad')
u.call('imu')

u.call('servo','>BI',2,90)
u.call('neopixel','4B',1,30,0,0)
u.call('neopixel_show')