set(srcs
    "main.c"
    "sketch.cpp"
    "uRemote.cpp")

set(requires
    "Adafruit_NeoPixel"
    "arduino"
    "bluepad32"
    "bluepad32_arduino"
    "btstack")

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "."
    REQUIRES "${requires}")
