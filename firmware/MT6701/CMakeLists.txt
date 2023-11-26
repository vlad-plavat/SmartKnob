add_library(MT6701 STATIC MT6701.c)

# rest of your project
add_compile_options(
	-Wall
    -Wextra
)

target_include_directories(MT6701 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

pico_generate_pio_header(MT6701 ${CMAKE_CURRENT_LIST_DIR}/spi_read.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(MT6701 pico_stdlib
                        hardware_dma
                        hardware_pio
                        )