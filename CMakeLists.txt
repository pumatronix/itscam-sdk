# ITSCAM Client SDK -- CMake build.
#
# Copyright (c) 2026 Pumatronix
#
# Builds libitscam_sdk with the same sources and flags as src/core/Makefile,
# for any toolchain CMake is given.  Three ways to use it (docs/cmake.md):
#
#   Standalone   mkdir -p build/cmake && cd build/cmake && cmake ../.. && cmake --build .
#   Subproject   add_subdirectory(itscam-sdk) or FetchContent, then
#                target_link_libraries(app PRIVATE itscam::sdk)
#   Installed    cmake --install, then find_package(itscam_sdk CONFIG)
#
# Every option is ITSCAM_SDK_*, so a parent project can set them before
# add_subdirectory() without clashing with its own.

cmake_minimum_required(VERSION 3.10)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/ItscamSdkVersion.cmake")
project(itscam_sdk VERSION ${ITSCAM_SDK_VERSION} LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(ITSCAM_SDK_IS_TOP_LEVEL ON)
else()
    set(ITSCAM_SDK_IS_TOP_LEVEL OFF)
endif()

add_subdirectory(src/core)

option(ITSCAM_SDK_BUILD_EXAMPLES "Build the C++ examples (src/examples)" OFF)
option(ITSCAM_SDK_BUILD_TESTS "Build the C and C++ smoke tests (tests/smoke)" OFF)

if(ITSCAM_SDK_BUILD_EXAMPLES)
    add_subdirectory(src/examples)
endif()
if(ITSCAM_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
