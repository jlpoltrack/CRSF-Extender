cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(rs422_bridge C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

# One source set, two targets: the side is a compile-time constant.
function(add_bridge_target NAME SIDE)
    add_executable(${NAME}
            src/main.c
            src/bridge.c
            src/local_port.c
            src/link_port.c
            src/stats.c
            src/status_led.c)

    target_compile_definitions(${NAME} PRIVATE BRIDGE_SIDE=${SIDE})
    target_include_directories(${NAME} PRIVATE src)
    target_compile_options(${NAME} PRIVATE -Wall -Wextra -O2)

    pico_generate_pio_header(${NAME} ${CMAKE_CURRENT_LIST_DIR}/src/uart_tx.pio)
    pico_generate_pio_header(${NAME} ${CMAKE_CURRENT_LIST_DIR}/src/uart_rx.pio)
    pico_generate_pio_header(${NAME} ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

    target_link_libraries(${NAME}
            pico_stdlib
            pico_multicore
            hardware_pio
            hardware_uart
            hardware_gpio
            hardware_clocks)

    # Whole image runs from SRAM: no XIP cache misses in the cut-through path,
    # and flash is untouched at runtime.
    pico_set_binary_type(${NAME} copy_to_ram)

    pico_enable_stdio_usb(${NAME} 1)
    pico_enable_stdio_uart(${NAME} 0)
    pico_add_extra_outputs(${NAME})
endfunction()

add_bridge_target(rs422_radio  1)   # SIDE_RADIO
add_bridge_target(rs422_module 2)   # SIDE_MODULE
