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
set(CMAKE_PROJECT_NAME TP_MDC)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

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

# ===========================
# User 源代码
# ===========================
file(GLOB_RECURSE USER_SOURCES 
    "User/App/Src/*.c"
    "User/BSP/bsp_dwt/Src/*.c"
    "User/BSP/bsp_gpio/Src/*.c"
    "User/BSP/bsp_uart/Src/*.c"
    "User/Hardware/heart_beat/Src/*.c"
    "User/Hardware/lan8720a/Src/*.c"
    "User/Middleware/SEGGER_RTT/Src/*.c"
    "User/Protocols/modbus/Src/*.c"
    "User/Tasks/Src/*.c"
)

# ===========================
# 添加到可执行文件
# ===========================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${USER_SOURCES}
)

# ===========================
# 头文件路径
# ===========================
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # App
    User/App/Inc
    
    # Common
    User/Common/Inc

    # BSP
    User/BSP/bsp_dwt/Inc
    User/BSP/bsp_gpio/Inc    
    User/BSP/bsp_uart/Inc
    
    # Hardware
    User/Hardware/heart_beat/Inc
    User/Hardware/lan8720a/Inc
    
    # Middleware
    User/Middleware/SEGGER_RTT/Inc
    
    # Protocols
    User/Protocols/modbus/Inc
    
    # Tasks
    User/Tasks/Inc
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
)

# 编译后自动生成 .hex 和 .bin 文件
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/${CMAKE_PROJECT_NAME}.bin
    COMMENT "Generating HEX and BIN firmware"
)