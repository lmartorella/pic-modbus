cmake_minimum_required(VERSION 3.0.0)
project(net VERSION 0.1.0)

add_definitions(-D_CONF_POSIX)

file(GLOB COMMON_SRC "*.c")
file(GLOB POSIX_SRC "./hardware/posix/*.c")
file(GLOB RASPBERRY_SRC "./hardware/posix/raspberry/*.c")
add_library(net SHARED ${POSIX_SRC} ${COMMON_SRC} ${RASPBERRY_SRC})

target_compile_options(net PRIVATE -Wno-switch -Wno-duplicate-decl-specifier -Wno-main-return-type)
target_include_directories(net PRIVATE .)
set_target_properties(net PROPERTIES PUBLIC_HEADER *.h)

install(TARGETS net
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CPack)
include(GNUInstallDirs)
