from pybricks.hubs import TechnicHub
from pybricks.pupdevices import Motor
from pybricks.parameters import Button, Color, Direction, Port, Side, Stop
from pybricks.robotics import DriveBase
from pybricks.tools import wait, StopWatch

hub = TechnicHub()

from bluepad import BluePad

bp=BluePad(Port.A)

bp.neopixel_init(24,12) # set 24 pixel NeoPixel on pin GPIO12
bp.neopixel_fill((30,0,0),write=True)
wait(300)
bp.neopixel_zero()
wait(300)

st=StopWatch()
i=0
for x in range(1000):
    i+=1
    lednr=i%24
    c=i%20
    bp.neopixel_set_multi(lednr,3,[c,(c+6)%20,(c+12)%20]*3)
    q=bp.gamepad()
    (gplx,gply,gprx,gpry,buttons,dpad)=q
    if 'Y' in bp.btns_pressed(buttons,nintendo=True):
        lednr+=8
        print('Y')
print(st.time())

    
print(st.time())
st=StopWatch()
for x in range(100):
    q=bp.servo(2,x%180)
    print(q)
print(st.time())

while 1:
    (gplx,gply,gprx,gpry,buttons,dpad)=bp.gamepad()
    x1=(512+gplx)//210
    y1=(512+gply)//210
    #hub.display.off()
    #hub.display.pixel(y1,x1,100)
    x1=(512+gprx)//210
    y1=(512+gpry)//210
    #hub.display.pixel(y1,x1,70)
    btns=bp.btns_pressed(buttons,nintendo=True)
    print(bp.gamepad())
    if len(btns)>0:
        print(btns)
    wait(20)
