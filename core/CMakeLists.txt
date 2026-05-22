# ITSCAM SDK Core Library CMakeLists.txt
#
# Cross-platform build configuration for ITSCAM Client SDK
# Supports: Linux (x86_64, ARMv7, ARM64) and Windows (x86_64)
#
# Copyright (c) 2026 Pumatronix

cmake_minimum_required(VERSION 3.16)
project(itscam_sdk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_EXAMPLES "Build example applications" OFF)

# Detect platform
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    set(PLATFORM_NAME "windows")
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    set(PLATFORM_NAME "linux")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Set up output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source files
set(SDK_SOURCES
    itscam_client.cpp
    itscam_rest_client.cpp
    itscam_sdk_c.cpp
)

# Platform-specific sources
if(PLATFORM_WINDOWS)
    list(APPEND SDK_SOURCES impl/itscam_sdk_utils_windows.cpp)
else()
    list(APPEND SDK_SOURCES impl/itscam_sdk_utils_linux.cpp)
endif()

# Main library
add_library(itscam_sdk ${SDK_SOURCES})

target_include_directories(itscam_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Export symbols for shared library
if(BUILD_SHARED_LIBS)
    target_compile_definitions(itscam_sdk PRIVATE ITSCAM_SDK_EXPORTS)
    if(PLATFORM_WINDOWS)
        set_target_properties(itscam_sdk PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS OFF
        )
    else()
        set_target_properties(itscam_sdk PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
        )
    endif()
endif()

# Platform-specific libraries
if(PLATFORM_WINDOWS)
    target_link_libraries(itscam_sdk PRIVATE ws2_32)
elseif(PLATFORM_LINUX)
    target_link_libraries(itscam_sdk PRIVATE pthread)
endif()

# nlohmann/json (header-only)
find_path(JSON_INCLUDE_DIR nlohmann/json.hpp
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/build/deps
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty
        /usr/include
        /usr/local/include
)
if(JSON_INCLUDE_DIR)
    target_include_directories(itscam_sdk PRIVATE ${JSON_INCLUDE_DIR})
else()
    message(STATUS "nlohmann/json not found, fetching...")
    include(FetchContent)
    FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
    target_link_libraries(itscam_sdk PRIVATE nlohmann_json::nlohmann_json)
endif()

# cpp-httplib (header-only, for REST client)
find_path(HTTPLIB_INCLUDE_DIR httplib.h
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/build/deps
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty
        /usr/include
        /usr/local/include
)
if(HTTPLIB_INCLUDE_DIR)
    target_include_directories(itscam_sdk PRIVATE ${HTTPLIB_INCLUDE_DIR})
else()
    message(STATUS "cpp-httplib not found, fetching...")
    include(FetchContent)
    FetchContent_Declare(httplib
        URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.14.1.tar.gz
    )
    FetchContent_MakeAvailable(httplib)
    target_link_libraries(itscam_sdk PRIVATE httplib::httplib)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS itscam_sdk
    EXPORT itscam_sdk-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    itscam_client.h
    itscam_rest_client.h
    itscam_sdk.h
    itscam_sdk_c.h
    itscam_sdk_utils.h
    itscam_types.h
    itscam_jpeg_utils.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/itscam
)

install(EXPORT itscam_sdk-targets
    FILE itscam_sdk-config.cmake
    NAMESPACE itscam::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/itscam_sdk
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/itscam_sdk-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_BINARY_DIR}/itscam_sdk-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/itscam_sdk
)
