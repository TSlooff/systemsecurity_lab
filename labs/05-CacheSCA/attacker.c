#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/qos.h>
#include "common.h"

// BEGIN TODO set this to an appropriate buffer size for your laptop's cache (number indicates MB of buffer)
#define EVICTION_BUFFER_SIZE (1) 
// END TODO

#define EVICTION_ELEMENTS ((EVICTION_BUFFER_SIZE * 1024 * 1024) / sizeof(void*))
#define TRIALS 100

uint8_t *probe_array; 
void **eviction_buffer;

static inline void memory_barrier() {
    asm volatile ("dsb ish" : : : "memory");
    asm volatile ("isb" : : : "memory");
}

static inline uint64_t get_time() {
    uint64_t val;
    memory_barrier();
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));
    memory_barrier();
    return val;
}

static inline void force_read(volatile uint8_t *p) {
    asm volatile("" : : "r" (*p) : "memory");
}

static inline uint8_t* get_probe_address(int i) {
    return &probe_array[i];
}

void setup_eviction() {
    printf("[*] Allocating %dMB Eviction Buffer...\n", EVICTION_BUFFER_SIZE);
    eviction_buffer = (void**)malloc(EVICTION_BUFFER_SIZE * 1024 * 1024);
    
    for (size_t i = 0; i < EVICTION_ELEMENTS - 1; i++) {
        eviction_buffer[i] = (void*)&eviction_buffer[i+1];
    }
    eviction_buffer[EVICTION_ELEMENTS - 1] = (void*)&eviction_buffer[0];

    // Fisher-Yates Shuffle with larger stride
    for (size_t i = 0; i < EVICTION_ELEMENTS - 1; i += 64) {
        size_t j = i + (rand() % (EVICTION_ELEMENTS - i));
        void *temp = eviction_buffer[i];
        eviction_buffer[i] = eviction_buffer[j];
        eviction_buffer[j] = temp;
    }
}

// this function makes sure the entire cache is filled with our eviction buffer
void evict_cache() {
    // pointer chasing
    void **p = eviction_buffer;
    for (size_t i = 0; i < EVICTION_ELEMENTS; i++) {
        p = (void**)*p;
    }
    // avoid compiler optimization
    asm volatile("" : : "r" (p) : "memory");
}

// --- ATTACK ---
void attack() {
    // initialize variables
    uint64_t timing_results[PROBE_SIZE] = {0}; // stores the sum of cache timing for each index
    
    // list of indices which will be shuffled to evade prefetcher
    int indices[PROBE_SIZE];
    for(int i=0; i<PROBE_SIZE; i++) 
        indices[i] = i;

    printf("[*] Attacking Shared Memory...\n");

    for (int t = 0; t < TRIALS; t++) {
        // 0. randomize indices order to evade prefetcher
        for (int i = 0; i < PROBE_SIZE - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (PROBE_SIZE - i) + 1);
            int t = indices[j]; indices[j] = indices[i]; indices[i] = t;
        }

        // 1. Evict cache & wait for victim process
        evict_cache();
        usleep(100);

        // 2. Probe cache
        // NOTE: an index of `i` corresponds to a memory access of `i * STRIDE`
        for (int k = 0; k < PROBE_SIZE; k++) {
            // BEGIN TODO implement probing
            // You will need `get_probe_address` and `get_time` and `force_read` functions.
            
            // i) take index to probe

            // ii) get the correct probe address for the given index
            
            // iii) time memory access
            
            // iv) cap timing outliers (>200) and add timing to `timing_results` for this index
            
            // END TODO
        }
        printf("\r[*] Progress: %d/%d", t+1, TRIALS);
        fflush(stdout);
    }
    printf("\n");

    // BEGIN TODO Find the index most likely to be the secret
    int min_index = -1;
    
    // END TODO
    
    printf("[*] Secret: %d\n", min_index);
}

int main() {
    pthread_t current_thread = pthread_self();
    int ret = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);

    if (ret != 0) {
        fprintf(stderr, "Warning: Could not set QoS class to run on performance core.\n");
    }

    srand(time(NULL));

    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd == -1) {
        fprintf(stderr, "Error: Could not open shared memory '%s'.\n", SHM_NAME);
        fprintf(stderr, "Make sure ./victim is running first.\n");
        return 1;
    }

    probe_array = mmap(NULL, MEM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (probe_array == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed.\n");
        return 1;
    }

    setup_eviction();
    attack();

    return 0;
}