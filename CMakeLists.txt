# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/darkice/Code/pi-pico/pico-sdk")
set(PICO_BOARD "pico_w")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(led C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_compile_options(-Wall)

# Add executable. Default name is the project name, version 0.1

add_executable(led led.c dhcpserver/dhcpserver.c dnsserver/dnsserver.c httpserver/httpserver.c led_effects/led_effects.c)

pico_generate_pio_header(led ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_include_directories(led PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )

pico_set_program_name(led "led")
pico_set_program_version(led "0.1")

pico_enable_stdio_uart(led 0)
pico_enable_stdio_usb(led 1)


target_link_libraries(led pico_cyw43_arch_lwip_poll pico_stdlib hardware_pio pico_multicore)

pico_add_extra_outputs(led)

