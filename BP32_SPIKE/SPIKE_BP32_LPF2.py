import hub
import time
p=hub.port.A
print(p.info())
while True:
    print(p.device.get(0))
    time.sleep_ms(50)