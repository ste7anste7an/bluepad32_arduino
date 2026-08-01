# BluePad32 visual gamepad

Standalone Web Serial page for estimating gamepad roll, pitch, and relative
heading from the existing `GET GAMEPAD` text command.

Serve the repository root:

```text
python -m http.server 8000
```

Open:

```text
http://localhost:8000/BluePad32_uRemote/visual_gamepad/
```

The page polls at about 20 Hz. It integrates the three gyroscope axes using the
measured sample interval. A complementary filter blends integrated roll and
pitch toward accelerometer-derived gravity angles. The gyro-weight slider
controls that blend.

The default gyro conversion is 65536 raw units per degree/second. Adjust the
field when a controller uses a different scale. Calibration averages 60 still
samples to estimate gyro bias.

Heading is relative to the last zero and can drift. Absolute compass heading
cannot be recovered from accelerometer and gyroscope data without a
magnetometer or another external heading reference.
