add_library(WS2812 STATIC WS2812.c)

# rest of your project
add_compile_options(
	-Wall
    -Wextra
)

target_include_directories(WS2812 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

pico_generate_pio_header(WS2812 ${CMAKE_CURRENT_LIST_DIR}/WS2812.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(WS2812 pico_stdlib
                        hardware_dma
                        hardware_pio
                        )