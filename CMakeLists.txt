cmake_minimum_required(VERSION 3.14...3.22)

# ---- Project ----

# Note: update this to your new project's name and version
#project(
#  PicoWClient
#  VERSION 1.0
#  LANGUAGES CXX C ASM
#)

# Initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(WIFI_SSID $ENV{WIFI_SSID})
set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})
set(TEST_TCP_SERVER_IP $ENV{TEST_TCP_SERVER_IP})

set(PICO_SDK_FETCH_FROM_GIT on)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(cmake/CPM.cmake)
include(pico_sdk_import.cmake)


project(my_project)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()


# Collect source files for firmware
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Firmware executable target
add_executable(main ${sources} ${headers})

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
