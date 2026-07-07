from pybricks.parameters import Port
from pybricks.tools import wait
from uremote import uRemote

ur = uRemote(Port.A)


def first(value):
    # Some uRemote versions return a single value directly, while others
    # return a tuple/list even when there is only one response value.
    if isinstance(value, (tuple, list)):
        return value[0]
    return value


print("version:", ur.call("ver"))
print("status:", ur.call("status"))

print("all sticks:", ur.call("joy"))
print("left stick:", ur.call("joyl"))
print("right stick:", ur.call("joyr"))
print("buttons:", ur.call("btn"))
print("imu:", ur.call("imu"))

# I2C is on SDA GPIO 5 / SCL GPIO 4.
count, addresses = ur.call("i2c_scan")
print("i2c devices:", count, list(addresses))

# Example I2C calls. Change address/register for your device.
# data = ur.call("i2c_read", 0x40, 2)
# data = ur.call("i2c_read_reg", 0x68, 0x75, 1)
# err, written = ur.call("i2c_write", 0x40, bytes([0x01, 0x02]))
# err, written = ur.call("i2c_write_reg", 0x68, 0x6B, bytes([0x00]))

# Use the currently connected gamepad as the only allowed controller.
mac = first(ur.call("bt_mac"))
print("connected gamepad:", mac)

if mac != "00:00:00:00:00:00":
    print("setting allow list")
    ur.call("bt_allow", mac)       # format: "xx:xx:xx:xx:xx:xx"
    ur.call("bt_filter", True)     # enable allow-list filtering
    ur.call("save")                # EEPROM stores only BT settings

print("allowed:", ur.call("bt_allow"))
print("filter:", ur.call("bt_filter"))

# LED test.
ur.call("pix", 0, 0, 40, 0)
wait(500)
ur.call("clear")

# To disable the allow list again, uncomment these lines:
# ur.call("bt_filter", False)
# ur.call("bt_clear")
# ur.call("save")
