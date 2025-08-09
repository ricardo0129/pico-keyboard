cmake_minimum_required(VERSION 3.13...3.27)
# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(WIFI_SSID $ENV{WIFI_SSID})
set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})
set(TEST_TCP_SERVER_IP $ENV{TEST_TCP_SERVER_IP})

set(PICO_SDK_FETCH_FROM_GIT on)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(my_project)
enable_testing()

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(main
    src/main.c
)

target_compile_definitions(main PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    TEST_TCP_SERVER_IP=\"${TEST_TCP_SERVER_IP}\"
    PICO_ENTER_USB_BOOT_ON_EXIT=1
)

# Add pico_stdlib library which aggregates commonly used features
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/..
)


target_link_libraries(main
    pico_stdlib
    pico_cyw43_arch_lwip_poll
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_enable_stdio_usb(main 1)
pico_add_extra_outputs(main)

add_test(NAME main_test
    COMMAND main
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
)
