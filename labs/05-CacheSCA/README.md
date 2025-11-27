# Lab 5: Cache Side-Channels
In this lab we will perform a capture the flag type exercise using cache side-channels. 

## Materials
- [`victim.out`](./victim.out): an executable program which access a secret-dependent location of a shared memory.
- [`attacker.c`](./attacker.c): a partially complete C program which shares the memory with `victim.out`, which you will complete to implement the cache side-channel and retrieve the secret index of `victim.out`.
- [`CMakeLists.txt`](./CMakeLists.txt): This tells CMake how to generate the build files for the attacker program.

## Tasks
You will only have to write in the `attacker.c` implementation, specifically only within TODO blocks which are shaped as follows:
 ```
 // BEGIN TODO <task description>
    ...
 // END TODO
 ```

## Compiling
As part of the anaconda environment you have a C compiler and CMake installed, you can compile the attacker program as follows (when in the current directory):
```
cmake .
cmake --build .
```
This will generate an `attacker.out` file, which you can execute as follows: `./attacker.out`.