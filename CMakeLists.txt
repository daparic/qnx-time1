cmake_minimum_required(VERSION 3.21)

project(time1 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared compile options for all targets in this project
add_compile_options(-Wall -Wextra -O2)

add_executable(time1   time1.c)
add_executable(another another.c)

# QNX kernel calls are in libc; -lm needed for some SDP variants
target_link_libraries(time1   PRIVATE m)
target_link_libraries(another PRIVATE m)
