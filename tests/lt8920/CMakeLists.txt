cmake_minimum_required(VERSION 3.0.0)
project(lt8920-tests LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

add_executable(lt8920 lt8920.c main.cpp hw.cpp)
