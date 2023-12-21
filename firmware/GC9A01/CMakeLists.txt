add_library(GC9A01 STATIC GC9A01.c)

# rest of your project
add_compile_options(
	-Wall
    -Wextra
)

target_include_directories(GC9A01 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

        pico_generate_pio_header(GC9A01 ${CMAKE_CURRENT_LIST_DIR}/GC9A01.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(GC9A01 pico_stdlib
                        hardware_dma
                        hardware_pio
                        hardware_pwm
                        )