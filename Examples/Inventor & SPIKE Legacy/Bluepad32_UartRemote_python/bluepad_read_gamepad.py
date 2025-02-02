# Test script for a direct gamepad connection to the SPIKE legacy or MINDSTORMS hub.
# Using an LMS-ESP32 board. Get the board here:
# https://antonsmindstorms.com/product/wifi-python-esp32-board-for-mindstorms/

# The LMS-ESP32 runs our fork of bluepad with uartremote.
# Flash you LMS_ESP32 directly from:https://firmware.antonsmindstorms.com/
# choose:  BluePad32 UartRemote for LMS micropython projects
#
# add the mpy_robot_tools to your LegoMindstroms Inventor or SPIKE Legacy.
# copy the code in https://github.com/antonvh/mpy-robot-tools/blob/master/Installer/install_mpy_robot_tools.py
# in an emty Python project and run the program. The libraries are now installed
# on the hub.

from projects.mpy_robot_tools.uartremote import UartRemote

ur = UartRemote('A')

while 1:
    ack, pad = ur.call('gamepad')
    print(ack, pad)