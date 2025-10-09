#include "hal.h"
#include "simpleserial.h"
#include <string.h>
#include <stdlib.h>

static uint16_t toyclock;
static uint8_t key[10];
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

// BEGIN PRESENT

static const uint8_t sbox[] = {0xc, 0x5, 0x6, 0xb, 0x9, 0x0, 0xa, 0xd, 0x3, 0xe, 0xf, 0x8, 0x4, 0x7, 0x1, 0x2};
static const uint8_t permutations[] = 
{0, 16, 32, 48, 1, 17, 33, 49, 2, 18, 34, 50, 3, 19, 35, 51,
4, 20, 36, 52, 5, 21, 37, 53, 6, 22, 38, 54, 7, 23, 39, 55,
8, 24, 40, 56, 9, 25, 41, 57, 10, 26, 42, 58, 11, 27, 43, 59,
12, 28, 44, 60, 13, 29, 45, 61, 14, 30, 46, 62, 15, 31, 47, 63};

void lr61(uint8_t *key) {
    // left-rotates given key with 61 bits
    uint8_t temp[10];
    
    for (uint8_t i = 0; i < 10; i++) {
        uint8_t offset_byte_l = (i + 7) % 10;
        uint8_t offset_byte_r = (i + 8) % 10;

        uint8_t left = key[offset_byte_l] << 5;
        uint8_t right = key[offset_byte_r] >> 3;
        
        temp[i] = left | right;
    }

    memcpy(key, temp, 10);
}

void generate_round_keys(uint8_t *key, uint8_t **round_keys) {
    uint8_t state[10];
    memcpy(state, key, 10);
    memcpy(round_keys[0], state, 8);
    for (uint8_t i = 1; i < 32; i++) {
        // left-rotates key with 61
        lr61(state);

        // left-most 4 bits of state to go into sbox
        uint8_t sbox_in = state[0] >> 4 & 0xf;
        // zero out left-most 4 bits
        state[0] &= 0xf;
        // set left-most 4 bits to sbox output
        state[0] |= sbox[sbox_in] << 4;
        
        state[7] ^= (i >> 1) & 0xf; // index 56 - 63 (19 - 16)
        state[8] ^= ((i & 0x1) << 7); // index 64 (15)
        memcpy(round_keys[i], state, 8);
    }
}

void permutation_layer(uint8_t *state) {
    // TODO PERMUTATION LAYER
    uint8_t tmp[8] = {0}; // this is initialized to all 0's

    for (uint8_t i = 0; i < 64; i++) {
        // extract bit val of index i (with index 0 being the left-most bit) from state
        uint8_t bit_val;
        
        // Set the bit at index 'j' of tmp to bit_val
        uint8_t j = permutations[i];
    }
    
    // overwrites the state
    memcpy(state, tmp, 8);
}

void sbox_layer(uint8_t *state) {
    // TODO SBOX LAYER
    // Set every nibble (4 bits) to its' sbox output
}

void add_round_key(uint8_t *state, uint8_t *rkey) {
    // TODO ADD ROUND KEY
    // XOR the state with the round key (both 64 bits)
}

void PRESENT(uint8_t *state, uint8_t *key) {
    // 64-bit state (plaintext), 80-bit key
    uint8_t **rkeys = malloc(32 * sizeof(uint8_t *));
    for (uint8_t i = 0; i < 32; i++)
        rkeys[i] = malloc(8 * sizeof(uint8_t));
    generate_round_keys(key, rkeys);

    for (uint8_t i = 0; i < 31; i++) {
        add_round_key(state, rkeys[i]);
        sbox_layer(state);
        permutation_layer(state);
    }

    add_round_key(state, rkeys[31]);

    for (int i = 0; i < 32; i++)
        free(rkeys[i]);
    free(rkeys);
}

// END PRESENT

void encrypt(uint8_t *data, uint8_t data_len) {
    // encrypt first 8 bytes
    PRESENT(data, key);
    // encrypt second 8 bytes
    PRESENT(&data[8], key);
}

void send_sensor_data(SensorData *data, uint8_t encrypt_data) {
    uint8_t bytes[sizeof(SensorData)];
    memcpy(bytes, data, sizeof(bytes));
    if (encrypt_data & 0x01) {
        trigger_high();
        encrypt(bytes, sizeof(bytes));
        trigger_low();
    }

    simpleserial_put('s', sizeof(bytes), bytes);
}

uint8_t poll(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t *buf) {
    if (scmd == 0x01) {
        SensorData d;
        poll_sensor(&d);
        send_sensor_data(&d, 1);
    } 
    if (scmd == 0x02) {
        SensorData d;
        poll_sensor(&d);
        send_sensor_data(&d, 0);
    }
    return 0x00;
}

uint8_t mirror(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t *buf) {

    if (scmd & 0x01) {
        simpleserial_put('m', len, buf);
    }
    
    return 0x00;
}

uint8_t vars(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t *buf) {

    if (scmd == 0x01) {
        // get the key
        simpleserial_put('k', sizeof(key), key);
    }
    if (scmd == 0x02) {
        // set the key
        memcpy(key, buf, 10);
    }
    
    return 0x00;
}

uint8_t test(uint8_t cmd, uint8_t scmd, uint8_t len, uint8_t *buf) {

    if (scmd == 0x01) {
        // test left-rotate
        uint8_t bytes[10];
        memcpy(bytes, buf, 10);
        lr61(bytes);
        simpleserial_put('r', sizeof(bytes), bytes);
    }

    if (scmd == 0x02) {
        // test key schedule (can only return 188 bytes of key schedule)
        uint8_t **rkeys = malloc(32 * sizeof(uint8_t *));
        for (int i = 0; i < 32; i++)
            rkeys[i] = malloc(8 * sizeof(uint8_t));
        generate_round_keys(buf, rkeys);

        uint8_t out[32*8];

        for (int i = 0; i < 32; i++)
            memcpy(&out[i*8], rkeys[i], 8);
            
        simpleserial_put('r', 188, out);
        for (int i = 0; i < 32; i++)
            free(rkeys[i]);
        free(rkeys);
    }

    if (scmd == 0x03) {
        // test sbox layer
        uint8_t out[8];
        memcpy(out, buf, 8);
        sbox_layer(out);
        simpleserial_put('s', sizeof(out), out);
        
    }

    if (scmd == 0x04) {
        // test permutation layer
        uint8_t out[8];
        memcpy(out, buf, 8);
        permutation_layer(out);
        simpleserial_put('p', sizeof(out), out);
    }

    if (scmd == 0x05) {
        // test present
        uint8_t state[8];
        uint8_t key[10];
        memcpy(state, buf, 8);
        memcpy(key, &buf[8], 10);
        PRESENT(state, key);
        simpleserial_put('p', sizeof(state), state);
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
    for (int i = 0; i < 10; i++)
        key[i] = (my_rand() >> 8) & 0xff;
    toyclock = my_rand();

    // at most 252 bytes of data can be received in a single command
    // at most 188 bytes of data can be sent
    simpleserial_addcmd(0x01, 252, poll);
    simpleserial_addcmd(0xac, 252, mirror);
    simpleserial_addcmd(0xdc, 252, vars);
    simpleserial_addcmd(0xaa, 252, test);
    
	while(1) {
        simpleserial_get();
        toyclock++;
    }
}
