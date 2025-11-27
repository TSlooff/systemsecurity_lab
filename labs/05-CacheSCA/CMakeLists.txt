cmake_minimum_required(VERSION 3.5)

project(
  CacheSCALab
  VERSION 1.0
  LANGUAGES C)

include_directories(${PROJECT_BINARY_DIR}/include)
# add_executable(victim.out victim.c)
# target_compile_options(victim.out PRIVATE -O0)
add_executable(attacker.out attacker.c)
target_compile_options(attacker.out PRIVATE -O0)
