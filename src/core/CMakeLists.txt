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

cmake_minimum_required(VERSION 3.8)
project(itscam_sdk VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

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

# Set up output directories (only when building standalone, not as add_subdirectory)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# ---------------------------------------------------------------------------
#  Version header (itscam_sdk_version.h)
#  Generated from git metadata by tools/version/gen-version.sh.
#  OUT_HEADER_DIR tells the script to write into the build tree so the
#  source tree stays clean.  Two-phase approach mirrors the Makefile:
#    1. configure time  — run once so the header exists before compilation
#    2. build time      — custom target (ALL, no output file ⇒ always runs)
#                         keeps SHA / date current across rebuilds
# ---------------------------------------------------------------------------
set(_VERSION_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/version/gen-version.sh")
set(_VERSION_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/itscam_sdk_version.h")

find_program(BASH_EXECUTABLE NAMES bash)

# 1 — configure-time bootstrap
if(NOT EXISTS "${_VERSION_HEADER}")
    if(EXISTS "${_VERSION_SCRIPT}" AND BASH_EXECUTABLE)
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E env
                    "OUT_HEADER_DIR=${CMAKE_CURRENT_BINARY_DIR}"
                    "ITSCAM_VERSION_HEADER_ONLY=1"
                    "${BASH_EXECUTABLE}" "${_VERSION_SCRIPT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
            RESULT_VARIABLE _ver_result
            OUTPUT_QUIET
            ERROR_QUIET
        )
    endif()
    if(NOT EXISTS "${_VERSION_HEADER}")
        message(WARNING "[itscam-sdk] Version script unavailable or bash not found; using fallback 0.0.0")
        file(WRITE "${_VERSION_HEADER}"
            "/* Auto-generated fallback — run tools/version/gen-version.sh for real version */\n"
            "#pragma once\n"
            "#define ITSCAM_SDK_VERSION_MAJOR 0\n"
            "#define ITSCAM_SDK_VERSION_MINOR 0\n"
            "#define ITSCAM_SDK_VERSION_PATCH 0\n"
            "#define ITSCAM_SDK_VERSION_STRING \"0.0.0\"\n"
            "#define ITSCAM_SDK_VERSION_FULL \"0.0.0+unknown\"\n"
            "#define ITSCAM_SDK_GIT_SHA \"0000000000000000000000000000000000000000\"\n"
            "#define ITSCAM_SDK_GIT_SHA_SHORT \"0000000\"\n"
            "#define ITSCAM_SDK_BUILD_DATE \"1970-01-01T00:00:00Z\"\n"
        )
    endif()
endif()

# 2 — build-time: regenerate version header when inputs change (avoid full rebuild every build)
find_program(PYTHON3_EXECUTABLE NAMES python3)
set(_VERSION_DEPS "${_VERSION_SCRIPT}")
foreach(_p IN ITEMS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../.git/HEAD"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../.git/index")
    if(EXISTS "${_p}")
        list(APPEND _VERSION_DEPS "${_p}")
    endif()
endforeach()
if(EXISTS "${_VERSION_SCRIPT}" AND BASH_EXECUTABLE AND PYTHON3_EXECUTABLE AND NOT WIN32)
    add_custom_command(
        OUTPUT "${_VERSION_HEADER}"
        COMMAND "${CMAKE_COMMAND}" -E env "OUT_HEADER_DIR=${CMAKE_CURRENT_BINARY_DIR}" "ITSCAM_VERSION_HEADER_ONLY=1" "${BASH_EXECUTABLE}" "${_VERSION_SCRIPT}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
        DEPENDS ${_VERSION_DEPS}
        COMMENT "Generating itscam_sdk_version.h"
        VERBATIM
    )
    add_custom_target(itscam_sdk_version DEPENDS "${_VERSION_HEADER}")
else()
    add_custom_target(itscam_sdk_version COMMENT "[itscam-sdk] Skip build-time version regeneration")
endif()

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

# Require C++17 for the SDK target and any consumer that links against it.
# PUBLIC scope means itscamd (and other consumers) also get C++17 transitively,
# while unrelated targets like jlib are unaffected.
target_compile_features(itscam_sdk PUBLIC cxx_std_17)

# Library depends on version target; binary dir holds the generated header
add_dependencies(itscam_sdk itscam_sdk_version)

target_include_directories(itscam_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
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
    elseif(PLATFORM_LINUX)
        # camera-daemon builds with global -fvisibility=hidden via jlib flags.
        # Override with CMake's visibility preset to export all C++ API symbols.
        set_target_properties(itscam_sdk PROPERTIES
            CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN OFF
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

install(FILES "${_VERSION_HEADER}" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/itscam)

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
