cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

set(TARGET_NAME mansour)

set(PICO_NO_PICOTOOL 0)    # this cuases to skip uf2 build
set(PICO_NO_BINARY_INFO 0) # Disables more metadata bloat

project(${TARGET_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(UTIL_SOURCES util/event_dispatcher.c
                 util/sys_timer.c
                 util/soft_timer.c
                 util/mainloop_timer.c
                 util/shell.c
                 util/circ_buffer.c
)

set(APP_SOURCES main.c
                blue_led.c
                ws2812_rgb.c
                shell_if_usb.c
                usb_descriptors.c
                core1.c
                shell_commands.c
                tm1638.c
                tm1638_demo.c
                rotary_encoder.c
                motor_demo.c
                dht11.c
                as5600.c
                mansour_foc.c
)

set(PROJECT_SOURCES
    ${APP_SOURCES}
    ${UTIL_SOURCES}
)

set(PROJECT_LIBRARIES
    pico_stdlib
    tinyusb_device
    tinyusb_board
    pico_multicore
    hardware_irq
    hardware_pio
    hardware_clocks
    hardware_exception
    hardware_dma
    hardware_i2c
    hardware_pwm
    m
    hardware_exception
    hardware_sync)

add_compile_definitions(SYS_CLK_MHZ=200)

add_executable(${TARGET_NAME} ${PROJECT_SOURCES})

pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/ws2812.pio)
pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/tm1638.pio)
pico_generate_pio_header(${TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/dht11.pio)

target_include_directories(${TARGET_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/util)

pico_enable_stdio_usb(${TARGET_NAME} 0)
pico_enable_stdio_uart(${TARGET_NAME} 0)

target_link_libraries(${TARGET_NAME} ${PROJECT_LIBRARIES})

pico_add_extra_outputs(${TARGET_NAME})
