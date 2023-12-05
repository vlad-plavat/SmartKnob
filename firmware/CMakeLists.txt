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
add_subdirectory(HX711)
add_subdirectory(Motor)
add_subdirectory(GC9A01)

add_executable(SmartKnob
    SmartKnob.c
)
#pico_set_linker_script(SmartKnob banked_custom.ld)
set_target_properties(SmartKnob PROPERTIES PICO_TARGET_LINKER_SCRIPT 
    ${CMAKE_SOURCE_DIR}/banked_custom.ld)
pico_set_binary_type(SmartKnob blocked_ram)

target_sources(SmartKnob PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/SmartKnob.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

target_include_directories(SmartKnob PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})


# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(SmartKnob pico_stdlib 
                        hardware_spi
                        hardware_dma
                        hardware_pio
                        hardware_pwm
                        hardware_adc
                        pico_multicore
                        MT6701
                        WS2812
                        HX711
                        Motor
                        GC9A01
                        tinyusb_device
                        tinyusb_board
                        )
pico_enable_stdio_usb(SmartKnob 0)
pico_enable_stdio_uart(SmartKnob 0)
# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(SmartKnob)