#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdio.h>

// --- Timing & barriers (architecture specific) ---
#if defined(__aarch64__)

// TIMING_CAP: anything slower than this is treated as noise (e.g. OS interrupt)
// cntvct_el0 ticks at 24MHz on Apple Silicon (~42ns per tick)
#define TIMING_CAP 200

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

#elif defined(__x86_64__)

// rdtsc ticks at a constant rate (~base frequency, e.g. 2.7GHz), not at the current core clock
#define TIMING_CAP 1000

static inline void memory_barrier() {
    asm volatile ("mfence" : : : "memory");
    asm volatile ("lfence" : : : "memory");
}

static inline uint64_t get_time() {
    uint32_t lo, hi;
    memory_barrier();
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    memory_barrier();
    return ((uint64_t)hi << 32) | lo;
}

#else
#error "Unsupported architecture: only aarch64 and x86_64 are supported."
#endif

// --- Scheduling (OS specific) ---
#if defined(__APPLE__)
#include <pthread.h>
#include <sys/qos.h>

// ask macOS to schedule us on a performance core
static inline void use_performance_core() {
    if (pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0) != 0) {
        fprintf(stderr, "Warning: Could not set QoS class to run on performance core.\n");
    }
}

#else

static inline void use_performance_core() {}

#endif

#endif
