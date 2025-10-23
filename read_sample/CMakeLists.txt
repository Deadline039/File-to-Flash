cmake_minimum_required(VERSION 3.10)
project(test C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

add_executable(test test.c flash_fs.c)