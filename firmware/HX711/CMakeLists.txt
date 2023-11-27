add_library(HX711 STATIC HX711.c)

# rest of your project
add_compile_options(
	-Wall
    -Wextra
)

target_include_directories(HX711 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

        pico_generate_pio_header(HX711 ${CMAKE_CURRENT_LIST_DIR}/HX711.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(HX711 pico_stdlib
                        hardware_dma
                        hardware_pio
                        )