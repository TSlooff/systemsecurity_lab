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
    
    device_id = fields['device_id'][3] + struct.unpack(fields['device_id'][0], bytes_data[fields['device_id'][1]:fields['device_id'][2]])[0]
    temperature = fields['temperature'][3] + struct.unpack(fields['temperature'][0], bytes_data[fields['temperature'][1]:fields['temperature'][2]])[0] / fields['temperature'][4]
    humidity = fields['humidity'][3] + struct.unpack(fields['humidity'][0], bytes_data[fields['humidity'][1]:fields['humidity'][2]])[0] / fields['humidity'][4]
    pressure = fields['pressure'][3] + struct.unpack(fields['pressure'][0], bytes_data[fields['pressure'][1]:fields['pressure'][2]])[0] / fields['pressure'][4]
    wind_speed = fields['wind_speed'][3] + struct.unpack(fields['wind_speed'][0], bytes_data[fields['wind_speed'][1]:fields['wind_speed'][2]])[0] / fields['wind_speed'][4]
    wind_direction = fields['wind_direction'][3] + struct.unpack(fields['wind_direction'][0], bytes_data[fields['wind_direction'][1]:fields['wind_direction'][2]])[0] / fields['wind_direction'][4]
    noise = fields['noise'][3] + struct.unpack(fields['noise'][0], bytes_data[fields['noise'][1]:fields['noise'][2]])[0] / fields['noise'][4]
    timestamp = fields['timestamp'][3] + struct.unpack(fields['timestamp'][0], bytes_data[fields['timestamp'][1]:fields['timestamp'][2]])[0]
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
    