cmake_minimum_required(VERSION 3.8)
project(untitled1)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES OSA1.3.c threads3.c littleThread.h)
add_executable(untitled1 ${SOURCE_FILES})