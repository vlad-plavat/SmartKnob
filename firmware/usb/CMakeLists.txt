add_library(usb STATIC usb_descriptors.c)

# rest of your project
add_compile_options(
	-Wall
    -Wextra
)

target_include_directories(usb PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

target_sources(usb PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        )

target_include_directories(usb PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})


# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(usb pico_stdlib
                        tinyusb_device
                        tinyusb_board
                        )