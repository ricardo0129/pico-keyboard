cmake_minimum_required(VERSION 3.14...3.22)

set(PICO_SDK_FETCH_FROM_GIT on)

include(cmake/CPM.cmake)

include(pico_sdk_import.cmake)

project(pico_keyboard C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# ---- Collect source files ----
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE common_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp")
file(GLOB_RECURSE parent_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/parent/*.cpp")
file(GLOB_RECURSE child_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/child/*.cpp")

# ---- Parent executable ----
add_executable(parent ${parent_sources} ${common_sources} ${headers})

target_compile_definitions(parent PRIVATE
    PICO_ENTER_USB_BOOT_ON_EXIT=1
)

target_link_libraries(parent
    pico_stdlib
    pico_unique_id 
    tinyusb_device 
    tinyusb_board
    pico_i2c_slave
    hardware_i2c
)

target_include_directories(parent PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_include_directories(parent PUBLIC
    ${PROJECT_SOURCE_DIR}/include/common
    ${PROJECT_SOURCE_DIR}/include/parent
)


pico_enable_stdio_usb(parent 1)
pico_add_extra_outputs(parent)

# ---- Child executable ----
add_executable(child ${child_sources} ${common_sources} ${headers})

target_compile_definitions(child PRIVATE
    PICO_ENTER_USB_BOOT_ON_EXIT=1
)

target_link_libraries(child
    pico_stdlib
    pico_unique_id 
    tinyusb_device 
    tinyusb_board
    pico_i2c_slave
    hardware_i2c
)

target_include_directories(child PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_include_directories(child PUBLIC
    ${PROJECT_SOURCE_DIR}/include/common
    ${PROJECT_SOURCE_DIR}/include/child
)

pico_enable_stdio_usb(child 1)
pico_add_extra_outputs(child)
