# Lab 5: Cache Side-Channels
In this lab we will perform a capture the flag type exercise using cache side-channels. 

## Scenario
The victim is an encryption service with a secret, randomly generated key byte. It continuously encrypts a plaintext byte using a lookup table (the AES S-box): it reads `sbox[plaintext ^ key]`, just like real table-based AES implementations do. The table lives in shared memory, like a table in a shared crypto library. The attacker can read it, but the table is public (as it should be!) so its contents tell you nothing. However, accessing cached memory is faster than accessing uncached memory. Your goal is to recover the key.

## Materials
- [`victim.c`](./victim.c): the encryption service. It generates a random key each time it starts, lets you choose the plaintext, and checks your key guesses. There is a limit to key guessing so you can not brute force it.
- [`attacker.c`](./attacker.c): a partially complete C program which shares the memory with the victim, which you will complete to implement the cache side-channel and find out which key the victim is currently using.
- [`include/`](./include/): shared constants ([`common.h`](./include/common.h)) and the platform-specific timing code ([`platform.h`](./include/platform.h)).
- [`CMakeLists.txt`](./CMakeLists.txt): This tells CMake how to generate the build files for the victim and attacker programs.

Reading the victim's source code is allowed, but the key is chosen at runtime, so you can only learn it through the side-channel.

## Machine
- This lab has been tested on macOS on Apple Silicon (M1 or newer), and Linux on x86_64. It likely will not work on Windows, so if you have a Windows machine please collaborate with a neighbour.

## The attack: Evict+Reload
The attack in `attacker.c` is called **Evict+Reload**. On x86 you could flush a single cache line from user space (`clflush`), but on ARM you cannot, so instead the attacker evicts *everything* by walking through a large eviction buffer. Each trial of `attack()` consists of:
1. **Evict**: fill the cache with the eviction buffer, so none of the table entries are cached.
2. **Wait**: give the victim time to encrypt, i.e. access its secret-dependent entry.
3. **Reload**: time a read of every entry. Which entry should behave differently, and how?

Some things to understand before you start:
- **Why repeat?** On Apple Silicon the timer (`cntvct_el0`) ticks at only 24 MHz (~42 ns per tick), so a cache hit and a miss differ by just a tick or two (on x86, `rdtsc` is much finer, but noise is still significant). A single measurement is useless, so timings are summed over `TRIALS` runs. Occasionally the OS interrupts a measurement, which is why outliers above `TIMING_CAP` are capped.
- **Why `STRIDE` and shuffling?** Table entries are 64 KB apart and are probed in a random order. Think about what the hardware prefetcher would do otherwise.
- **A note on realism**: the cache works on cache lines (64 or 128 bytes), not bytes. A real S-box is dense, so many entries share one cache line and a real attacker only learns *which line* was accessed, i.e. only the top bits of the index. In this lab every entry gets its own `STRIDE`, so you can recover the whole byte at once.

## Tasks
You will only have to write in the `attacker.c` implementation, specifically only within TODO blocks which are shaped as follows:
 ```
 // BEGIN TODO <task description>
    ...
 // END TODO
 ```
1. **Eviction buffer size**: the buffer must be large enough to evict the shared slots from *all* cache levels. Look up your cache sizes (macOS: `sysctl -a | grep cachesize`, Linux: `lscpu --caches`). Note that not every cache level is reported (e.g. Apple's system level cache), so be generous and experiment: too small gives wrong results, too large is just slow. Expect to need a buffer of a few times your last-level cache size.
2. **Probing**: time the access to each table entry, following the steps in the code comments.
3. **Finding the accessed entry**: decide, from the accumulated timings, which index the victim accessed. Note that this index is not the key itself!

## Compiling
As part of the anaconda environment you have a C compiler and CMake installed, you can compile the programs as follows (when in the current directory):
```
cmake .
cmake --build .
```
This will generate the `victim.out` and `attacker.out` files.

## Running
The victim must be running before you start the attacker, so use two terminals:
```
# Terminal 1: keep this running, stop it with Ctrl+C
./victim.out

# Terminal 2
./attacker.out
```
The victim is controlled by typing commands into its terminal (values can be decimal or hex, e.g. `0x42`):
- `p <byte>`: set the plaintext that the victim encrypts. Set it *before* running the attacker, otherwise your measurements mix two different entries. Note that this construction is therefore a chosen-plaintext attack.
- `k <byte>`: guess the key. You have 3 guesses: after 3 wrong guesses the victim generates a new key, so you will have to run your attack again.

A run of the attacker ends with `[*] Accessed index: <index>`. Before guessing, make sure repeated runs give the same index and that its total timing clearly stands out from the others (try printing `timing_results`). Changing the plaintext is a good sanity check: the accessed index should change accordingly.

## Troubleshooting
- `Could not open shared memory`: the victim is not running.
- Results differ between runs: increase the eviction buffer size, close other applications, and plug in your charger so the attacker runs on a performance core.

## Questions to think about
- Why does this work even though the victim and attacker are separate processes?
- How could the victim be written so that it does not leak its key this way?
- With a real, dense S-box you only learn the top bits of `plaintext ^ key`. How much of the key could you recover then, and what could help you get more?
- What could an attacker do if the plaintext is not known to them?