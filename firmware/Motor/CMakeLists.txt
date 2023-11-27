add_library(Motor STATIC Motor.c)

# rest of your project
add_compile_options(
	-Wall
    -Wextra
)

target_include_directories(Motor PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})


# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(Motor pico_stdlib
                        hardware_pwm
                        )