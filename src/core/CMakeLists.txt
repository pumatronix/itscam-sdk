# ITSCAM SDK core library (libitscam_sdk) -- CMake build.
#
# Copyright (c) 2026 Pumatronix
#
# Normally included by the repository's top-level CMakeLists.txt; it also
# works on its own (add_subdirectory(itscam-sdk/src/core) or cmake -S
# src/core).  Options and targets are documented in docs/cmake.md.
#
# Keep the sources and flags in step with src/core/Makefile, which builds
# the release artefacts: `make verify` builds and tests both.

cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED ITSCAM_SDK_VERSION)
    include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ItscamSdkVersion.cmake")
endif()
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(itscam_sdk VERSION ${ITSCAM_SDK_VERSION} LANGUAGES C CXX)
    set(ITSCAM_SDK_IS_TOP_LEVEL ON)
elseif(NOT DEFINED ITSCAM_SDK_IS_TOP_LEVEL)
    set(ITSCAM_SDK_IS_TOP_LEVEL OFF)
endif()

# ----------------------------------------------------------------------------
#  Options
# ----------------------------------------------------------------------------

# A parent project gets a static library unless it asks otherwise (or sets
# BUILD_SHARED_LIBS); a standalone build makes the shared one, like make.
if(DEFINED BUILD_SHARED_LIBS)
    set(_itscam_shared_default ${BUILD_SHARED_LIBS})
else()
    set(_itscam_shared_default ${ITSCAM_SDK_IS_TOP_LEVEL})
endif()
option(ITSCAM_SDK_SHARED "Build libitscam_sdk as a shared library" ${_itscam_shared_default})
option(ITSCAM_SDK_WITH_REST "Include the REST client (ItscamRestClient)" ON)
option(ITSCAM_SDK_WITH_CGI "Include the CGI client (ItscamCgiClient)" ON)
option(ITSCAM_SDK_WITH_TLS "HTTPS for the REST and CGI clients (vendored mbedTLS)" ON)
option(ITSCAM_SDK_WITH_C_API "Include the C API the language wrappers bind" ON)
option(ITSCAM_SDK_STATIC_RUNTIME
    "Link libstdc++ and libgcc into the shared library and keep their symbols local" ON)
option(ITSCAM_SDK_INSTALL "Generate install rules and the itscam_sdk package" ${ITSCAM_SDK_IS_TOP_LEVEL})
set(ITSCAM_SDK_CXX_STANDARD 17 CACHE STRING "C++ standard the SDK sources compile with (14 or 17)")
set_property(CACHE ITSCAM_SDK_CXX_STANDARD PROPERTY STRINGS 14 17)

if(NOT ITSCAM_SDK_CXX_STANDARD MATCHES "^(14|17)$")
    message(FATAL_ERROR "ITSCAM_SDK_CXX_STANDARD must be 14 or 17, not '${ITSCAM_SDK_CXX_STANDARD}'")
endif()
if(NOT WIN32 AND NOT (UNIX AND NOT APPLE))
    message(FATAL_ERROR "itscam_sdk builds for Linux and Windows (MinGW) only")
endif()
if(WIN32 AND NOT MINGW)
    message(FATAL_ERROR "itscam_sdk builds for Windows with MinGW-w64 only (see docs/compatibility.md)")
endif()

# ----------------------------------------------------------------------------
#  Sources (src/core/Makefile: SOURCES_COMMON, SOURCES_NETWORK, *_PLATFORM)
# ----------------------------------------------------------------------------

set(_itscam_sources
    itscam_client.cpp
    impl/itscam_callback_queue.cpp)
if(ITSCAM_SDK_WITH_REST OR ITSCAM_SDK_WITH_CGI)
    list(APPEND _itscam_sources
        impl/itscam_http_transport.cpp
        impl/itscam_multipart.cpp)
endif()
if(ITSCAM_SDK_WITH_REST)
    list(APPEND _itscam_sources itscam_rest_client.cpp)
endif()
if(ITSCAM_SDK_WITH_CGI)
    list(APPEND _itscam_sources itscam_cgi_client.cpp)
endif()
if(ITSCAM_SDK_WITH_C_API)
    list(APPEND _itscam_sources c_api/itscam_sdk_c.cpp)
    if(ITSCAM_SDK_WITH_REST)
        list(APPEND _itscam_sources c_api/itscam_rest_client_c.cpp)
    endif()
    if(ITSCAM_SDK_WITH_CGI)
        list(APPEND _itscam_sources c_api/itscam_cgi_client_c.cpp)
    endif()
endif()
if(WIN32)
    list(APPEND _itscam_sources
        impl/itscam_sdk_utils_windows.cpp
        impl/itscam_os_windows.cpp
        impl/itscam_os_windows_fs.cpp)
else()
    list(APPEND _itscam_sources
        impl/itscam_sdk_utils_linux.cpp
        impl/itscam_os_linux.cpp
        impl/itscam_os_linux_fs.cpp)
endif()

# ----------------------------------------------------------------------------
#  mbedTLS, linked into the library
# ----------------------------------------------------------------------------

set(ITSCAM_SDK_HAS_TLS OFF)
if(ITSCAM_SDK_WITH_TLS AND (ITSCAM_SDK_WITH_REST OR ITSCAM_SDK_WITH_CGI))
    if(NOT TARGET itscam_mbedtls_objects)
        include("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mbedtls/mbedtls.cmake")
    endif()
    if(MBEDTLS_SOURCES)
        set(ITSCAM_SDK_HAS_TLS ON)
    endif()
endif()

# ----------------------------------------------------------------------------
#  Library
# ----------------------------------------------------------------------------

if(ITSCAM_SDK_SHARED)
    set(_itscam_kind SHARED)
    set(_itscam_link PRIVATE)
else()
    # A static library's users link its system libraries themselves
    set(_itscam_kind STATIC)
    set(_itscam_link PUBLIC)
endif()

if(ITSCAM_SDK_HAS_TLS)
    add_library(itscam_sdk ${_itscam_kind} ${_itscam_sources} $<TARGET_OBJECTS:itscam_mbedtls_objects>)
    # httplib.h includes the mbedTLS headers: the C++ sources must see the
    # configuration the C sources were built with (mbedtls.cmake)
    target_compile_definitions(itscam_sdk PRIVATE CPPHTTPLIB_MBEDTLS_SUPPORT
        MBEDTLS_USER_CONFIG_FILE="itscam_mbedtls_config.h")
    target_include_directories(itscam_sdk PRIVATE ${MBEDTLS_INCLUDE} ${MBEDTLS_CONFIG_DIR})
else()
    add_library(itscam_sdk ${_itscam_kind} ${_itscam_sources})
endif()
add_library(itscam::sdk ALIAS itscam_sdk)

set_target_properties(itscam_sdk PROPERTIES
    EXPORT_NAME sdk
    OUTPUT_NAME itscam_sdk
    CXX_STANDARD ${ITSCAM_SDK_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    VERSION ${ITSCAM_SDK_VERSION}
    SOVERSION ${ITSCAM_SDK_VERSION_MAJOR})
# The public headers need C++14
target_compile_features(itscam_sdk INTERFACE cxx_std_14)

target_include_directories(itscam_sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>
        $<BUILD_INTERFACE:${ITSCAM_SDK_VERSION_HEADER_DIR}>
        $<INSTALL_INTERFACE:include/itscam>
        $<INSTALL_INTERFACE:include/itscam/3rdparty>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/c_api)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(itscam_sdk PRIVATE -Wall -Wextra)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    # GCC 7.1 notes that it changed how ARM passes some arguments; nothing
    # crosses the library's boundary that way, and the map keeps the
    # templates concerned inside it
    target_compile_options(itscam_sdk PRIVATE -Wno-psabi)
endif()

find_package(Threads REQUIRED)
target_link_libraries(itscam_sdk ${_itscam_link} Threads::Threads)

if(WIN32)
    # MinGW: Win32 threading shims instead of libstdc++ <thread>
    target_compile_definitions(itscam_sdk PRIVATE WIN32 _WIN32 ITSCAM_MINGW_WIN32_THREADS)
    target_link_libraries(itscam_sdk ${_itscam_link} ws2_32 bcrypt crypt32)
    if(ITSCAM_SDK_SHARED)
        target_compile_definitions(itscam_sdk PRIVATE ITSCAM_SDK_BUILDING)
    else()
        target_compile_definitions(itscam_sdk PUBLIC ITSCAM_SDK_STATIC)
    endif()
endif()

# Only the C API and the itscam namespace are exported (itscam_sdk.map)
if(ITSCAM_SDK_SHARED AND NOT WIN32)
    target_link_libraries(itscam_sdk PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/itscam_sdk.map")
    set_property(TARGET itscam_sdk APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/itscam_sdk.map")
endif()

# The shared library carries its own C++ runtime: it then needs only the C
# library, whatever libstdc++ the application's toolchain has
if(ITSCAM_SDK_SHARED AND ITSCAM_SDK_STATIC_RUNTIME AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(itscam_sdk PRIVATE -static-libstdc++ -static-libgcc)
    if(NOT WIN32)
        target_link_libraries(itscam_sdk PRIVATE -Wl,--exclude-libs,ALL)
    endif()
endif()

# ----------------------------------------------------------------------------
#  Install and package
# ----------------------------------------------------------------------------

if(ITSCAM_SDK_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    set(_itscam_cmake_dir "${CMAKE_INSTALL_LIBDIR}/cmake/itscam_sdk")

    install(TARGETS itscam_sdk EXPORT itscam_sdk-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # The layout of the release tarball's cpp/include, under include/itscam
    file(GLOB _itscam_headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
    list(REMOVE_ITEM _itscam_headers "${CMAKE_CURRENT_SOURCE_DIR}/itscam_sdk_version.h")
    file(GLOB _itscam_c_headers "${CMAKE_CURRENT_SOURCE_DIR}/c_api/*.h")
    install(FILES ${_itscam_headers} "${ITSCAM_SDK_VERSION_HEADER_DIR}/itscam_sdk_version.h"
        DESTINATION include/itscam)
    install(FILES ${_itscam_c_headers} DESTINATION include/itscam/c_api)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nlohmann"
                      "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/nonstd"
        DESTINATION include/itscam/3rdparty)

    install(EXPORT itscam_sdk-targets NAMESPACE itscam:: DESTINATION ${_itscam_cmake_dir})
    configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/../../cmake/itscam_sdkConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/itscam_sdkConfig.cmake"
        INSTALL_DESTINATION ${_itscam_cmake_dir})
    # A minor release may break callers while the major is 0, so a request
    # for 0.9 accepts 0.9.x only (docs/compatibility.md)
    configure_file("${CMAKE_CURRENT_LIST_DIR}/../../cmake/itscam_sdkConfigVersion.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/itscam_sdkConfigVersion.cmake" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/itscam_sdkConfig.cmake"
                  "${CMAKE_CURRENT_BINARY_DIR}/itscam_sdkConfigVersion.cmake"
        DESTINATION ${_itscam_cmake_dir})
endif()

message(STATUS "itscam_sdk ${ITSCAM_SDK_VERSION}: ${_itscam_kind}, C++${ITSCAM_SDK_CXX_STANDARD}, "
    "REST ${ITSCAM_SDK_WITH_REST}, CGI ${ITSCAM_SDK_WITH_CGI}, TLS ${ITSCAM_SDK_HAS_TLS}, "
    "C API ${ITSCAM_SDK_WITH_C_API}")
