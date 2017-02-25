cmake_minimum_required(VERSION 3.6)
project(sandbox)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        server.c client.c)

add_executable(sandbox ${SOURCE_FILES})