cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "Release")

set(PICO_BOARD "pico_w")

include(pico-sdk/pico_sdk_init.cmake)

project(led)

pico_sdk_init()

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(led led.c algorithms/algorithms.c dhcpserver/dhcpserver.c dnsserver/dnsserver.c httpserver/httpserver.c led_effects/led_effects.c storage/storage.c)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

pico_generate_pio_header(led ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_include_directories(led PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )

target_link_libraries(led pico_cyw43_arch_lwip_poll pico_stdlib pico_multicore hardware_pio hardware_flash)

pico_add_extra_outputs(led)
