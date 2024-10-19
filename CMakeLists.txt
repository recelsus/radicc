# vim ft=cmake

# CMake minimum version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(radicc VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add an executable
add_executable(radicc
    src/main.cpp
    src/radiko_recorder.cpp
    src/url_parser.cpp
    src/date_handler.cpp
    src/env_loader.cpp
    src/toml_parser.cpp
    src/arguments_handler.cpp
    src/list_config.cpp
)

# Include directories for header files
target_include_directories(radicc PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Platform and architecture-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS
    message(STATUS "Building for macOS")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        message(STATUS "Targeting ARM64 architecture on macOS")
        # macOS ARM64 specific options (if any)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    else()
        message(STATUS "Targeting x86_64 architecture on macOS")
        # macOS x86_64 specific options (if any)
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux
    message(STATUS "Building for Linux")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        message(STATUS "Targeting x86_64 architecture on Linux")
        # Linux x86_64 specific options (if any)
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        message(STATUS "Targeting ARM64 architecture on Linux")
        # Linux ARM64 specific options (if any)
    else()
        message(STATUS "Targeting other architecture on Linux: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
elseif(WIN32)
    # Windows
    message(STATUS "Building for Windows")
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
        message(STATUS "Targeting x64 architecture on Windows")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "x86")
        message(STATUS "Targeting x86 architecture on Windows")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
        message(STATUS "Targeting ARM64 architecture on Windows")
    endif()
endif()

# If you want to enable debugging symbols
# set(CMAKE_BUILD_TYPE Debug)

# Additional compiler flags, if needed (optional)
# target_compile_options(radicc PRIVATE -Wall -Wextra -pedantic)
