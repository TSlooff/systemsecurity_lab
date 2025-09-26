import struct
import numpy as np
import time

FIXED_POINT_PRECISION = 100
fields = {
    'device_id':       ('<H',  0,  2,    0, 1),
    'temperature':     ('<h',  2,  4,    0, FIXED_POINT_PRECISION),
    'humidity':        ('<H',  4,  6,    0, FIXED_POINT_PRECISION),
    'pressure':        ('<H',  6,  8, 1000, FIXED_POINT_PRECISION),
    'wind_speed':      ('<H',  8, 10,    0, FIXED_POINT_PRECISION),
    'wind_direction':  ('<H', 10, 12,    0, FIXED_POINT_PRECISION),
    'noise':           ('<H', 12, 14,    0, FIXED_POINT_PRECISION),
    'timestamp':       ('<H', 14, 16,    0, 1)
}

# These are the averages when looking at the fixed-point representation of the first 500 samples after a reset
field_averages = {'temperature': np.float64(1494.972),
 'humidity': np.float64(7041.496),
 'pressure': np.float64(1283.548),
 'wind_speed': np.float64(611.458),
 'wind_direction': np.float64(22007.58),
 'noise': np.float64(6045.446)}

def get_field(field:str, bytes_data:np.ndarray, FXP_REPR: bool):
    """
    Extracts 'field' from 'bytes_data' in either fixed-point representation or parsed value depending on 'FP_REPR'
    """
    assert(field in fields)
    field_info = fields[field]
    if len(bytes_data.shape) == 1:
        if FXP_REPR:
            return struct.unpack(field_info[0], bytes_data[field_info[1]:field_info[2]])[0]
        else:
            return field_info[3] + struct.unpack(field_info[0], bytes_data[field_info[1]:field_info[2]])[0] / field_info[4]
    elif len(bytes_data.shape) == 2:
        if FXP_REPR:
            return np.array(list(struct.unpack(field_info[0], bd[field_info[1]:field_info[2]])[0] for bd in bytes_data)).astype(field_info[0])
        else:
            return np.array(list(field_info[3] + struct.unpack(field_info[0], bd[field_info[1]:field_info[2]])[0] / field_info[4] for bd in bytes_data))
    else:
        print("Error: input data of incorrect shape")

def parse_sensor_data(bytes_data: bytes, print_data: bool):
    """
    Takes the (assumed unencrypted!) data as a bytes object and returns the unpacked sensor data as tuple:
    
    (device_id, temperature, humidity, pressure, wind_speed, wind_direction, timestamp)
    """
    assert(len(bytes_data) == 16)
    
    device_id = struct.unpack('H', bytes_data[0:2])[0]
    temperature = struct.unpack('h', bytes_data[2:4])[0] / FIXED_POINT_PRECISION
    humidity = struct.unpack('H', bytes_data[4:6])[0] / FIXED_POINT_PRECISION
    pressure = 1000 + struct.unpack('H', bytes_data[6:8])[0] / FIXED_POINT_PRECISION
    wind_speed = struct.unpack('H', bytes_data[8:10])[0] / FIXED_POINT_PRECISION
    wind_direction = struct.unpack('H', bytes_data[10:12])[0] / FIXED_POINT_PRECISION
    noise = struct.unpack('H', bytes_data[12:14])[0] / FIXED_POINT_PRECISION
    timestamp = struct.unpack('H', bytes_data[14:16])[0]
    if print_data:
        print(f"""Received sensor data from device {device_id} at time {timestamp}:
    Temperature: {temperature:.2f}°C
    Humidity:    {humidity:.2f}%
    Pressure:    {pressure:.2f} hPa
    Wind:        {wind_speed:.2f} m/s at {wind_direction:.2f}°
    Noise:       {noise:.2f}dB""")
    return (device_id, temperature, humidity, pressure, wind_speed, wind_direction, timestamp)

def unmask(data: bytes, mask: int):
    assert(0 <= mask <= 255)
    return bytearray(byte ^ mask for byte in data)

def reset_target(scope):
    scope.io.nrst = 'low'
    time.sleep(0.05)
    scope.io.nrst = 'high_z'
    time.sleep(0.05)
    