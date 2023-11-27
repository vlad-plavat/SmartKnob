cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(SmartKnob)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_compile_options(
	-Wall
    -Wextra
)

add_subdirectory(MT6701)
add_subdirectory(WS2812)

add_executable(SmartKnob
    SmartKnob.c
)

target_sources(SmartKnob PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/SmartKnob.c
        #${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

target_include_directories(SmartKnob PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})



#pico_generate_pio_header(SmartKnob ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)
#pico_generate_pio_header(SmartKnob ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
#pico_generate_pio_header(SmartKnob ${CMAKE_CURRENT_LIST_DIR}/spi_read.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(SmartKnob pico_stdlib 
                        hardware_spi
                        hardware_dma
                        hardware_pio
                        hardware_pwm
                        hardware_adc
                        MT6701
                        WS2812
                        #tinyusb_device
                        #tinyusb_board
                        )
pico_enable_stdio_usb(SmartKnob 1)
pico_enable_stdio_uart(SmartKnob 0)
# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(SmartKnob)