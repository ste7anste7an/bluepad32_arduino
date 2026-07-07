from pybricks.parameters import Port, Color
from pybricks.tools import wait
from bluepad import BluePad

bp = BluePad(Port.A)

print("version:", bp.version())
print("status:", bp.status())

# Old-style gamepad API: lx, ly, rx, ry, buttons, dpad
print("gamepad:", bp.gamepad())
print("left stick:", bp.joyl())
print("right stick:", bp.joyr())
print("buttons/dpad/misc:", bp.btn())
print("imu:", bp.imu())

# NeoPixel and servo calls use the same method names as the old BluePad library.
bp.neopixel_init(16, 12)
bp.neopixel_set(0, (0, 40, 0))
wait(500)
bp.neopixel_zero()

bp.servo(0, 90)
wait(500)
bp.servo_off(0)

# I2C helpers. The firmware uses SDA GPIO 5 and SCL GPIO 4.
addresses = bp.i2c_scan()
print("i2c addresses:", addresses)

# Example reads/writes. Change address/register for your device.
# data = bp.i2c_read(0x40, 2)
# whoami = bp.i2c_read_reg(0x68, 0x75, 1)
# err, written = bp.i2c_write(0x40, bytes([0x01, 0x02]))
# err, written = bp.i2c_write_reg(0x68, 0x6B, bytes([0x00]))

# Bluetooth allow-list helpers.
mac = bp.bt_mac()
print("connected gamepad:", mac)

if mac != "00:00:00:00:00:00":
    bp.bt_allow(mac)       # format: "xx:xx:xx:xx:xx:xx"
    bp.bt_filter(True)     # allow only that controller
    bp.save()              # EEPROM stores only BT settings

print("allowed:", bp.bt_allow())
print("filter enabled:", bp.bt_filter())

# To disable later:
# bp.bt_filter(False)
# bp.bt_clear()
# bp.save()
