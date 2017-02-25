cmake_minimum_required(VERSION 3.6)
project(fifo)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES src/main.c)
add_executable(fifo ${SOURCE_FILES})