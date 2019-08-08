cmake_minimum_required(VERSION 3.14)
project(portMapping C)

set(CMAKE_C_STANDARD 99)

add_executable(portMapping main.c sock.c serve.c)