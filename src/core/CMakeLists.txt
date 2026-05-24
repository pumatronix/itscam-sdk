# ITSCAM SDK Core Library CMakeLists.txt
#
# Cross-platform build configuration for ITSCAM Client SDK.
# Supports: Linux (x86_64, ARMv7, ARM64) and Windows (x86_64).
#
# HTTPS is provided by mbedTLS (vendored under 3rdparty/mbedtls/) and
# statically linked into libitscam_sdk.  If the mbedTLS tree is pruned,
# the SDK still builds but without HTTPS support (a warning is emitted).
#
# Copyright (c) 2026 Pumatronix

cmake_minimum_required(VERSION 3.16)
project(itscam_sdk VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
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
    itscam_cgi_client.cpp
    impl/itscam_http_transport.cpp
    impl/itscam_multipart.cpp
    c_api/itscam_sdk_c.cpp
    c_api/itscam_rest_client_c.cpp
    c_api/itscam_cgi_client_c.cpp
)

# Platform-specific sources
if(PLATFORM_WINDOWS)
    list(APPEND SDK_SOURCES
        impl/itscam_sdk_utils_windows.cpp
        impl/itscam_os_windows.cpp)
else()
    list(APPEND SDK_SOURCES
        impl/itscam_sdk_utils_linux.cpp
        impl/itscam_os_linux.cpp)
endif()

# ----------------------------------------------------------------------------
#  mbedTLS (statically linked into the SDK)
# ----------------------------------------------------------------------------

include(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mbedtls/mbedtls.cmake)

# ----------------------------------------------------------------------------
#  Main library
# ----------------------------------------------------------------------------

if(MBEDTLS_SOURCES)
    add_library(itscam_sdk ${SDK_SOURCES} $<TARGET_OBJECTS:itscam_mbedtls_objects>)
    target_compile_definitions(itscam_sdk PRIVATE CPPHTTPLIB_MBEDTLS_SUPPORT)
    target_include_directories(itscam_sdk PRIVATE
        ${MBEDTLS_INCLUDE} ${MBEDTLS_CONFIG_DIR})
else()
    add_library(itscam_sdk ${SDK_SOURCES})
endif()

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
    target_link_libraries(itscam_sdk PRIVATE ws2_32 bcrypt)
elseif(PLATFORM_LINUX)
    target_link_libraries(itscam_sdk PRIVATE pthread)
endif()

# nlohmann/json (header-only, vendored under 3rdparty/nlohmann)
target_include_directories(itscam_sdk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

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
    itscam_cgi_client.h
    itscam_sdk.h
    itscam_sdk_utils.h
    itscam_types.h
    itscam_jpeg_utils.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/itscam
)

install(FILES
    c_api/itscam_sdk_c.h
    c_api/itscam_rest_client_c.h
    c_api/itscam_cgi_client_c.h
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
