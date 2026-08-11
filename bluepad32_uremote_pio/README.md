# Bluepad32 uRemote - PlatformIO

PlatformIO port of `BluePad32_uRemote/BluePad32_uRemote.ino` for an ESP32
WROVER. The original Arduino sketch is unchanged.

## Versions

- Bluepad32 4.2.0 (latest stable release)
- Arduino-ESP32 3.2.1
- ESP-IDF 5.4.2
- Adafruit NeoPixel 1.15.5
- Click 8.1.8 (project-local compatibility dependency for esptool 5.0)

The component set comes from official Bluepad32 template commit
`d07a9385f46f7215f51fc3eb5e40c5a484cfe102`; its Arduino submodule is pinned
to `ac961f671abd5ae1da0a15fd4bee71ed807c2cf3`.

The Bluepad32, BTstack, Arduino, and NeoPixel sources are vendored under
`components/` so that the selected versions remain reproducible.
The build also uses the project-local Click copy under `tools/python`; it does
not alter PlatformIO's global Python environment.

## Build

Open this directory as the PlatformIO project, or run:

```powershell
platformio run
```

Upload and monitor with:

```powershell
platformio run --target upload
platformio device monitor
```

The default target is `esp-wrover-kit`, matching the WROVER target used by
the Arduino firmware. Change `board` and the upload/monitor ports in
`platformio.ini` if your particular carrier board needs different settings.

The uRemote UART and pin assignments are unchanged from the Arduino sketch.
