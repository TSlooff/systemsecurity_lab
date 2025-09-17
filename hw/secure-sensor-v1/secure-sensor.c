#include "hal.h"
#include "simpleserial.h"
#include <string.h>

static uint16_t toyclock;
static uint8_t mask;
static uint32_t seed;

uint16_t my_rand(void) {
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0xffff;
}

typedef struct {
    uint16_t device_id;
    int16_t temperature;
    uint16_t humidity;
    uint16_t pressure;
    uint16_t wind_speed;
    uint16_t wind_direction;
    uint16_t noise;
    uint16_t timestamp;
} SensorData;

int32_t sample_gaussian(int32_t mean, int32_t std_dev) {
    const int num_samples = 12;
    
    int64_t sum = 0;
    for (int i = 0; i < num_samples; i++) {
        sum += my_rand();
    }

    int64_t centered_sum = sum - 393210;
    int64_t scaled_val = (centered_sum * std_dev) / (65535);
    return mean + (int32_t)scaled_val;
}

// fixed-point :/ 10000
const uint16_t lut[16] = {
 0, 980, 1951, 2903,
 3827, 4714, 5556, 6344,
 7071, 7730, 8315, 8819,
 9239, 9569, 9808, 9952};

int16_t sine_lut(uint8_t angle_input) {
    uint8_t index = angle_input / 4;
    uint8_t quadrant = index / 16;
    uint8_t quadrant_index = index % 16;

    switch (quadrant) {
        case 0: return lut[quadrant_index];
        case 1: return lut[15 - quadrant_index];
        case 2: return -lut[quadrant_index];
        case 3: return -lut[15 - quadrant_index];
        default: return 0;
    }
}

void poll_sensor(SensorData *data) {
    data->device_id = 0xc1a2;
    data->timestamp = toyclock;
    data->temperature = 1500 + 5 * sine_lut(toyclock & 0xff) / 100 + sample_gaussian(0, 150);
    data->humidity = 7000 - 15 * sine_lut(toyclock & 0xff) / 100 + sample_gaussian(0, 200);
    data->pressure = 1300 + 5 * sine_lut(toyclock & 0xff) / 100 + sample_gaussian(0, 50);
    data->wind_speed = sample_gaussian(600, 150);
    data->wind_direction = sample_gaussian(22000, 4000);
    data->noise = sample_gaussian(6000, 1500);
}

void encrypt(uint8_t *data, uint8_t data_len) {
    for (uint8_t i = 0; i < data_len; i++) {
        data[i] ^= mask;
    }
}

void send_sensor_data(SensorData *data, uint8_t encrypt_data) {
    uint8_t bytes[sizeof(SensorData)];
    memcpy(bytes, data, sizeof(bytes));
    if (encrypt_data & 0x01)
        encrypt(bytes, sizeof(bytes));
    simpleserial_put('s', sizeof(bytes), bytes);
}

uint8_t poll(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t *buf) {
    if (scmd & 0x01) {
        SensorData d;
        poll_sensor(&d);
        send_sensor_data(&d, 1);
    }
    return 0x00;
}

uint8_t mirror(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t *buf) {

    if (scmd & 0x01) {
        simpleserial_put('m', len, buf);
    }
    
    return 0x00;
}

int main(void)
{
    platform_init();
	init_uart();
	trigger_setup();
    
	simpleserial_init();
    
    seed = 42;
    mask = (my_rand() >> 8) & 0xff;
    toyclock = my_rand();

    // at most 252 bytes of data can be received in a single command
    // at most 188 bytes of data can be sent
    simpleserial_addcmd(0x01, 252, poll);
    simpleserial_addcmd(0xac, 252, mirror);
    
	while(1) {
        simpleserial_get();
        toyclock++;
    }
}
