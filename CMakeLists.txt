cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME sumo)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/rtt/RTT/SEGGER_RTT.c)
    message(FATAL_ERROR "RTT submodule not initialized. Run: git submodule update --init --recursive")
endif()

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    app/drivers/motor_driver.c
    app/drivers/ir_remote.c
    app/drivers/line_sensor.c
    app/drivers/vl53l0x/vl53l0x_platform.c
    app/drivers/vl53l0x/vl53l0x_api.c
    app/drivers/vl53l0x/vl53l0x_api_calibration.c
    app/drivers/vl53l0x/vl53l0x_api_core.c
    app/drivers/vl53l0x/vl53l0x_api_ranging.c
    app/drivers/vl53l0x/vl53l0x_api_strings.c
    app/state.c
    app/ring_buffer.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/app
)

add_library(segger_rtt STATIC
    external/rtt/RTT/SEGGER_RTT.c
    external/rtt/RTT/SEGGER_RTT_printf.c
)

target_include_directories(segger_rtt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/app/config
    ${CMAKE_CURRENT_SOURCE_DIR}/external/rtt/RTT
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    segger_rtt
)

