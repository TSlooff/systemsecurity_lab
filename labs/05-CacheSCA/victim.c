#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "common.h"

#if defined(__APPLE__)
#include <sys/ptrace.h>
#elif defined(__linux__)
#include <sys/prctl.h>
#endif

#define MAX_TRIES 3

// AES S-box, a public lookup table (PROBE_SIZE == 256 entries)
static const uint8_t SBOX[PROBE_SIZE] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

static volatile uint8_t plaintext;
static volatile uint8_t secret_key;

void cleanup(int sig) {
    (void)sig;
    printf("[VICTIM] Cleaning up...\n");
    shm_unlink(SHM_NAME);
    exit(0);
}

// make it harder to simply read the key with a debugger
void deny_debugger() {
#if defined(__APPLE__)
    ptrace(PT_DENY_ATTACH, 0, 0, 0);
#elif defined(__linux__)
    prctl(PR_SET_DUMPABLE, 0);
#endif
}

uint8_t random_byte() {
    uint8_t b;
    FILE *f = fopen("/dev/urandom", "rb");
    if (f == NULL || fread(&b, 1, 1, f) != 1) {
        fprintf(stderr, "Error: could not read /dev/urandom\n");
        exit(1);
    }
    fclose(f);
    return b;
}

// parse a byte value (decimal or 0x.. hex), returns -1 if invalid
int parse_byte(const char *s) {
    char *end;
    long v = strtol(s, &end, 0);
    if (end == s || (*end != '\0' && *end != '\n') || v < 0 || v > 255) {
        return -1;
    }
    return (int)v;
}

void *encrypt_loop(void *arg) {
    uint8_t *sbox = arg; // S-box entry i is stored at sbox[i * STRIDE]

    while (1) {
        // Encrypt: secret-dependent table lookup, as in T-table AES implementations
        (void)*(volatile uint8_t *)&sbox[(plaintext ^ secret_key) * STRIDE];
        usleep(10);
    }

    return NULL;
}

void print_help() {
    printf("[VICTIM] Commands:  p <byte>   set the plaintext to encrypt (e.g. p 0x42)\n");
    printf("[VICTIM]            k <byte>   guess the key (%d wrong guesses and a new key is generated)\n", MAX_TRIES);
}

int main() {
    signal(SIGINT, cleanup);
    setvbuf(stdout, NULL, _IOLBF, 0); // also print line by line when redirected
    deny_debugger();

    shm_unlink(SHM_NAME);
    int fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);

    if (fd == -1) {
        fprintf(stderr, "Error: shm_open failed");
        return 1;
    }

    if (ftruncate(fd, MEM_SIZE) == -1) {
        fprintf(stderr, "Error: ftruncate failed");
        return 1;
    }

    uint8_t *shared_mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed");
        return 1;
    }

    // the shared memory holds the public S-box, one entry every STRIDE bytes
    memset(shared_mem, 0, MEM_SIZE);
    for (int i = 0; i < PROBE_SIZE; i++) {
        shared_mem[i * STRIDE] = SBOX[i];
    }

    secret_key = random_byte();
    plaintext = random_byte();

    pthread_t thread;
    if (pthread_create(&thread, NULL, encrypt_loop, shared_mem) != 0) {
        fprintf(stderr, "Error: pthread_create failed");
        return 1;
    }

    printf("[VICTIM] Encryption service started, S-box initialized in shared memory.\n");
    printf("[VICTIM] A random key was generated. Current plaintext: 0x%02x\n", plaintext);
    printf("[VICTIM] Press Ctrl+C to stop and clean up.\n");
    print_help();

    char line[64];
    int tries = 0;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        int value = (line[0] != '\0' && line[1] == ' ') ? parse_byte(&line[2]) : -1;
        if (value == -1 || (line[0] != 'p' && line[0] != 'k')) {
            print_help();
            continue;
        }

        if (line[0] == 'p') {
            plaintext = value;
            printf("[VICTIM] Now encrypting plaintext: 0x%02x\n", plaintext);
        } else if (value == secret_key) {
            printf("[VICTIM] Correct! You recovered the key.\n");
        } else if (++tries == MAX_TRIES) {
            secret_key = random_byte();
            tries = 0;
            printf("[VICTIM] Too many wrong guesses, a new key was generated.\n");
        } else {
            printf("[VICTIM] Wrong, guesses left: %d\n", MAX_TRIES - tries);
        }
    }

    cleanup(0);
    return 0;
}
