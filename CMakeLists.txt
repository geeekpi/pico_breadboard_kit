# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/taterli/pico/pico-sdk")

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)


if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(hello_world C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

# LVGL
include_directories(./) # 注意有先后顺序,否则LVGL找不到配置.
add_subdirectory(./components/lvgl)
include_directories(./components/lvgl)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE=1)

add_executable(hello_world lv_port_disp.c main.c )

pico_set_program_name(hello_world "hello_world")
pico_set_program_version(hello_world "0.1")

pico_enable_stdio_uart(hello_world 1)
pico_enable_stdio_usb(hello_world 0)

# Add the standard library to the build
target_link_libraries(hello_world
        pico_stdlib
        hardware_spi
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
        pico_multicore
        lvgl
        )

# Add the standard include files to the build
target_include_directories(hello_world PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

target_compile_definitions(hello_world PRIVATE
    mainRUN_FREE_RTOS_ON_CORE=1
    PICO_STACK_SIZE=0x1000
    PICO_STDIO_STACK_BUFFER_SIZE=64 # use a small printf on stack buffer
)

pico_add_extra_outputs(hello_world)

