cmake_minimum_required(VERSION 3.8)
project(ex4)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

set(SOURCE_FILES osqueue.c osqueue.h  threadPool.c threadPool.h liz.c)
add_executable(ex4 ${SOURCE_FILES} liz.c)





