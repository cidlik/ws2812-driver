# Source: https://github.com/enginning/cmake-kernel-module/blob/main/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(ws2812)

execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("Kernel release: ${KERNEL_RELEASE}")

set(KERNEL_HEADERS_DIR /lib/modules/${KERNEL_RELEASE}/build)
message("Kernel headers: ${KERNEL_HEADERS_DIR}")

set(DRIVER_FILE ws2812.ko)
set(KBUILD_CMD $(MAKE) -C ${KERNEL_HEADERS_DIR} M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR} modules)

FILE(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/Kbuild "obj-m := ws2812.o")

add_custom_command(OUTPUT ${DRIVER_FILE}
        COMMAND ${KBUILD_CMD}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ws2812.c VERBATIM)

add_custom_target(driver ALL DEPENDS ${DRIVER_FILE})
add_library(dummy_target ws2812.c)
