cmake_minimum_required(VERSION 3.14...3.22)


set(PICO_SDK_FETCH_FROM_GIT on)

include(cmake/CPM.cmake)

#CPMAddPackage(
#  NAME pico-sdk
#  GIT_TAG 2.2.0
#  GITHUB_REPOSITORY raspberrypi/pico-sdk
#)


include(pico_sdk_import.cmake)

# ---- Project ----

project(pico_net C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()


# Collect source files for firmware
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Firmware executable target
add_executable(${PROJECT_NAME} ${sources} ${headers})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_ENTER_USB_BOOT_ON_EXIT=1
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_unique_id 
    tinyusb_device 
    tinyusb_board
    pico_i2c_slave
    hardware_i2c
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_add_extra_outputs(${PROJECT_NAME})
