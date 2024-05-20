from lpf2 import LPF2, DATA16, ABSOLUTE, DATA8
from time import ticks_ms, time, sleep_ms
import struct

matrix_sensor_modes = [
{'symbol': 'PCT', 'format': {'datasets': 9, 'type': 0, 'figures': 1, 'decimals': 0}, 'capability': b'\x80\x00\x00\x00\x05\x04', 'map_out': 80, 'name': 'LEV O', 'pct': (-100.0, 100.0), 'map_in': 0, 'si': (-9.0, 9.0), 'raw': (-9.0, 9.0)},
{'symbol': 'PCT', 'format': {'datasets': 1, 'type': 0, 'figures': 2, 'decimals': 0}, 'capability': b'\x80\x00\x00\x00\x05\x04', 'map_out': 68, 'name': 'COL O', 'pct': (0.0, 100.0), 'map_in': 0, 'si': (0.0, 10.0), 'raw': (0.0, 10.0)},
{'symbol': '   ', 'format': {'datasets': 9, 'type': 0, 'figures': 3, 'decimals': 0}, 'capability': b'\x80\x00\x00\x00\x05\x04', 'map_out': 16, 'name': 'PIX O', 'pct': (0.0, 100.0), 'map_in': 0, 'si': (0.0, 170.0), 'raw': (0.0, 170.0)},
{'symbol': '   ', 'format': {'datasets': 1, 'type': 0, 'figures': 1, 'decimals': 0}, 'capability': b'\x80\x00\x00\x00\x05\x04', 'map_out': 16, 'name': 'TRANS', 'pct': (0.0, 100.0), 'map_in': 0, 'si': (0.0, 2.0), 'raw': (0.0, 2.0)},

]
light_sensor_id = 64


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


sensor_emu = LPF2(mode_convert(matrix_sensor_modes), 64, debug=False)# ,rx=8,tx=7) # Connects but no hearbeat.
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
    if sensor_emu.current_mode in [9]: # idx
        #if sensor_emu.special_color_mode==0:
            # change mode sizes
            sensor_emu.modes[sensor_emu.current_mode][8]=16
            sensor_emu.modes[sensor_emu.current_mode][9]=4
            sensor_emu.send_payload(struct.pack("9B", *[(i+d)%256 for i in range(9)]))
    if sensor_emu.current_mode in [2]: # idx
        #if sensor_emu.special_color_mode==0:
            # change mode sizes
            #sensor_emu.modes[sensor_emu.current_mode][8]=16
            #sensor_emu.modes[sensor_emu.current_mode][9]=4
            sensor_emu.send_payload(struct.pack("9B", *[(i+d)%256 for i in range(9)]))
    
            #sensor_emu.send_payload(struct.pack("8H", d*257,1024-d,*[(d+i)*257 for i in range(4)],0,0))
        #else:
        #    sensor_emu.send_payload(struct.pack("B", d%10))
    #elif sensor_emu.current_mode==1: # reflx
    #    sensor_emu.send_payload(struct.pack("B", 10+d%10))
    #elif sensor_emu.current_mode==2: # ambi
    #    sensor_emu.send_payload(struct.pack("B", 20+d%10))

    data_in = sensor_emu.heartbeat()
    #print(d)
    if data_in:
        print(f"\nReceived: {data_in[0]} on mode {data_in[1]}")
    sleep_ms(20)