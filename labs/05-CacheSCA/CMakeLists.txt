cmake_minimum_required(VERSION 3.10)

project(
  CacheSCALab
  VERSION 1.0
  LANGUAGES C)

find_package(Threads REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(victim.out victim.c)
target_compile_options(victim.out PRIVATE -O0)
target_link_libraries(victim.out PRIVATE Threads::Threads)
add_executable(attacker.out attacker.c)
target_compile_options(attacker.out PRIVATE -O0)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(victim.out PRIVATE rt)
  target_link_libraries(attacker.out PRIVATE rt)
endif()
