cmake_minimum_required(VERSION 3.0.0)
project(lt8920-tests LANGUAGES C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

add_executable(lt8920 lt8920.c main.cpp hw.cpp gpio.cpp radio.c)

# The tests
add_executable(lt8920Tests test/lt8920_test.cpp)
target_link_libraries(lt8920Tests PRIVATE Catch2WithMain)

enable_testing()
add_test(NAME lt8920Tests COMMAND $<TARGET_FILE:lt8920Tests>)
