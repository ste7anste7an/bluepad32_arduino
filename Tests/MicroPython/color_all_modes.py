from lpf2 import LPF2, DATA16, ABSOLUTE, DATA8
from time import ticks_ms, time, sleep_ms
import struct

light_sensor_modes = [
    {
        "symbol": "IDX",
        "format": {"datasets": 8, "type": 1, "figures": 2, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 16,
        "name": "COLOR",
        "pct": (0.0, 100.0),
        "map_in": 228,
        "si": (0.0, 10.0),
        "raw": (0.0, 10.0),
    },
    {
        "symbol": "PCT",
        "format": {"datasets": 8, "type": 1, "figures": 3, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 0,
        "name": "REFLT",
        "pct": (0.0, 100.0),
        "map_in": 48,
        "si": (0.0, 100.0),
        "raw": (0.0, 100.0),
    },
    {
        "symbol": "PCT",
        "format": {"datasets": 1, "type": 0, "figures": 3, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 0,
        "name": "AMBI",
        "pct": (0.0, 100.0),
        "map_in": 48,
        "si": (0.0, 100.0),
        "raw": (0.0, 100.0),
    },
    {
        "symbol": "PCT",
        "format": {"datasets": 3, "type": 0, "figures": 3, "decimals": 0},
        "capability": b"@\x00\x00\x00\x05\x04",
        "map_out": 16,
        "name": "LIGHT",
        "pct": (0.0, 100.0),
        "map_in": 0,
        "si": (0.0, 100.0),
        "raw": (0.0, 100.0),
    },
    {
        "symbol": "RAW",
        "format": {"datasets": 2, "type": 1, "figures": 4, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 0,
        "name": "RREFL",
        "pct": (0.0, 100.0),
        "map_in": 16,
        "si": (0.0, 1024.0),
        "raw": (0.0, 1024.0),
    },
    {
        "symbol": "RAW",
        "format": {"datasets": 4, "type": 1, "figures": 4, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 0,
        "name": "RGB I",
        "pct": (0.0, 100.0),
        "map_in": 16,
        "si": (0.0, 1024.0),
        "raw": (0.0, 1024.0),
    },
    {
        "symbol": "RAW",
        "format": {"datasets": 3, "type": 1, "figures": 4, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 0,
        "name": "HSV",
        "pct": (0.0, 100.0),
        "map_in": 16,
        "si": (0.0, 360.0),
        "raw": (0.0, 360.0),
    },
    {
        "symbol": "RAW",
        "format": {"datasets": 4, "type": 1, "figures": 4, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 0,
        "name": "SHSV",
        "pct": (0.0, 100.0),
        "map_in": 16,
        "si": (0.0, 360.0),
        "raw": (0.0, 360.0),
    },
    {
        "symbol": "RAW",
        "format": {"datasets": 4, "type": 1, "figures": 4, "decimals": 0},
        "capability": b"@\x00\x00\x00\x04\x84",
        "map_out": 0,
        "name": "DEBUG",
        "pct": (0.0, 100.0),
        "map_in": 16,
        "si": (0.0, 65535.0),
        "raw": (0.0, 65535.0),
    },
    {
        "symbol": "",
        "format": {"datasets": 7, "type": 1, "figures": 5, "decimals": 0},
        "capability": b"@@\x00\x00\x04\x84",
        "map_out": 0,
        "name": "CALIB",
        "pct": (0.0, 100.0),
        "map_in": 0,
        "si": (0.0, 65535.0),
        "raw": (0.0, 65535.0),
    },
]
light_sensor_id = 61


def mode_convert(lms_modes):
    result = []
    for lms_mode in lms_modes:
        result.append(
            LPF2.mode(
                name=lms_mode["name"].encode("UTF-8"), #+ b"\x00"+ lms_mode["capability"],
                size=lms_mode["format"]["datasets"],
                data_type=lms_mode["format"]["type"],
                writable=1 if lms_mode["map_out"] else 0,
                format=f"{lms_mode['format']['figures']}.{lms_mode['format']['decimals']}",
                raw_range=lms_mode["raw"],
                percent_range=lms_mode["pct"],
                si_range=lms_mode["si"],
                symbol=lms_mode["symbol"],
                functionmap=[lms_mode["map_in"], lms_mode["map_out"]],
                view=True,
            )
        )
    return result


single_mode_ls = [
    LPF2.mode(
        "GAMEPAD",
        6,
        DATA16,
        format="5.0",
        symbol="XYBD",
        raw_range=(0.0, 512.0),
        percent_range=(0.0, 1024.0),
        si_range=(0.0, 512.0),
    )
]

single_mode_ds = [
    LPF2.mode(
        "DISTL",
        1,
        DATA16,
        format="5.1",
        symbol="CM",
        raw_range=(0.0, 250.0),
        percent_range=(0.0, 100.0),
        si_range=(0.0, 2500.0),
        functionmap=[0, 145],
    )
]

def clamp(value, min_value, max_value):
    return max(min_value, min(max_value, value))

def saturate(value):
    return clamp(value, 0.0, 1.0)

def hue_to_rgb(h):
    r = abs(h * 6.0 - 3.0) - 1.0
    g = 2.0 - abs(h * 6.0 - 2.0)
    b = 2.0 - abs(h * 6.0 - 4.0)
    return saturate(r), saturate(g), saturate(b)

def hsl_to_rgb(h, s, l):
    # Takes hue in range 0-359, 
    # Saturation and lightness in range 0-99
    h /= 359
    s /= 100
    l /= 100
    r, g, b = hue_to_rgb(h)
    c = (1.0 - abs(2.0 * l - 1.0)) * s
    r = (r - 0.5) * c + l
    g = (g - 0.5) * c + l
    b = (b - 0.5) * c + l
    rgb = tuple([round(x*255) for x in (r,g,b)])
    return rgb

sensor_emu = LPF2(mode_convert(light_sensor_modes), 61, debug=False)# ,rx=8,tx=7) # Connects but no hearbeat.
sensor_emu.current_mode=0

d=0
c=0
old_mode=0
while 1:
    c+=1
    if sensor_emu.current_mode!=old_mode:
        old_mode=sensor_emu.current_mode
        print("mode changed to ",old_mode)
    if c>10:
        c=0
        d += 1
        print(d)        
        d%=1024
    if sensor_emu.current_mode in [0,1,2]: # idx
        #if sensor_emu.special_color_mode==0:
            # change mode sizes
            sensor_emu.modes[sensor_emu.current_mode][8]=16
            sensor_emu.modes[sensor_emu.current_mode][9]=4
            sensor_emu.send_payload(struct.pack("8H", d*257,1024-d,*[(d+i)*257 for i in range(4)],0,0))
        #else:
        #    sensor_emu.send_payload(struct.pack("B", d%10))
    #elif sensor_emu.current_mode==1: # reflx
    #    sensor_emu.send_payload(struct.pack("B", 10+d%10))
    #elif sensor_emu.current_mode==2: # ambi
    #    sensor_emu.send_payload(struct.pack("B", 20+d%10))
    elif sensor_emu.current_mode==3: # light
        #sensor_emu.send_payload(struct.pack("3B", 30+d%10,31+d%10,32+d%10))
        # no value from sensor to hub
        pass
    elif sensor_emu.current_mode==4: #     RREFL
        sensor_emu.send_payload(struct.pack("2H", d+100,d+101))
    elif sensor_emu.current_mode==5:
        #r,g,b= hsl_to_rgb(d,d+10,d+20)
        sensor_emu.send_payload(struct.pack("4H", d*257,(d+10)*257,(d+20)*257,(d+30)*257))
    elif sensor_emu.current_mode==7:
        sensor_emu.send_payload(struct.pack("4H", d+1,d+2,d+3,d+4))
        
    data_in = sensor_emu.heartbeat()
    #print(d)
    if data_in:
        print(f"\nReceived: {data_in[0]} on mode {data_in[1]}")
    sleep_ms(20)