cmake_minimum_required(VERSION 3.12)
project(ex4 C)

#set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
add_executable(ex4 main.c osqueue.c osqueue.h threadPool.h threadPool.c test.c)