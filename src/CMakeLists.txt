cmake_minimum_required(VERSION 3.0.0)
project(pic-modbus-tests LANGUAGES C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

# The tests
add_executable(rs485Tests tests/rs485_tests.cpp tests/sys.cpp rs485.c crc.c)
target_include_directories(rs485Tests PRIVATE tests include)
target_link_libraries(rs485Tests PRIVATE Catch2WithMain)

add_executable(busClientTests tests/rtu_client_tests.cpp tests/crc16.cpp rtu_client.c crc.c)
target_include_directories(busClientTests PRIVATE tests include)
target_link_libraries(busClientTests PRIVATE Catch2WithMain)

enable_testing()
add_test(NAME rs485Tests COMMAND $<TARGET_FILE:rs485Tests>)
add_test(NAME busClientTests COMMAND $<TARGET_FILE:busClientTests>)
