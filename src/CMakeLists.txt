cmake_minimum_required(VERSION 3.0.0)
project(net LANGUAGES C CXX)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)

add_definitions(-D_CONF_POSIX)

file(GLOB COMMON_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c client/mtu/*.c)
# Not ready yet
list(REMOVE_ITEM COMMON_SRC "bus_primary.c")
file(GLOB POSIX_SRC "./hardware/posix/*.c")
file(GLOB RASPBERRY_SRC "./hardware/posix/raspberry/*.c")
add_library(net SHARED ${POSIX_SRC} ${COMMON_SRC} ${RASPBERRY_SRC})

target_compile_options(net PRIVATE -Wno-switch -Wno-duplicate-decl-specifier -Wno-main-return-type)
target_include_directories(net PRIVATE include include/net/posix)
file(GLOB PUBLIC_H "include/net/*.h")
set_target_properties(net PROPERTIES PUBLIC_HEADER "${PUBLIC_H}")

install(TARGETS net
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/net)

# The tests
add_executable(rs485Tests tests/rs485Tests.cpp rs485.c)
target_include_directories(rs485Tests PRIVATE tests include)
target_link_libraries(rs485Tests PRIVATE Catch2WithMain)

add_executable(busClientMtuTests tests/busClientMtuTests.cpp client/mtu/bus_client_mtu.c)
target_include_directories(busClientMtuTests PRIVATE tests include)
target_link_libraries(busClientMtuTests PRIVATE Catch2WithMain)

enable_testing()
add_test(NAME rs485Tests COMMAND $<TARGET_FILE:rs485Tests>)
add_test(NAME busClientMtuTests COMMAND $<TARGET_FILE:busClientMtuTests>)
