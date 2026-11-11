# ItscamSdkVersion.cmake -- the SDK version for a CMake build.
#
# Copyright (c) 2026 Pumatronix
#
# Sets ITSCAM_SDK_VERSION (MAJOR.MINOR.PATCH), ITSCAM_SDK_VERSION_FULL and
# ITSCAM_SDK_VERSION_HEADER_DIR, the directory holding itscam_sdk_version.h:
#
#   1. src/core/itscam_sdk_version.h, when tools/version/gen-version.sh
#      (`make version`) wrote it: the Makefile build and this one agree.
#   2. Otherwise one generated in the build tree from `git describe` (the
#      v<MAJOR>.<MINOR>.<PATCH> tags), so configuring never writes into the
#      source tree, which may be a read-only submodule.
#   3. 0.0.0 outside a git checkout without a generated header.

set(_itscam_root "${CMAKE_CURRENT_LIST_DIR}/..")
set(_itscam_header "${_itscam_root}/src/core/itscam_sdk_version.h")

if(EXISTS "${_itscam_header}")
    file(STRINGS "${_itscam_header}" _itscam_lines REGEX "^#define ITSCAM_SDK_VERSION_(STRING|FULL) ")
    foreach(_line IN LISTS _itscam_lines)
        if(_line MATCHES "^#define ITSCAM_SDK_VERSION_STRING \"([0-9]+\\.[0-9]+\\.[0-9]+)\"")
            set(ITSCAM_SDK_VERSION "${CMAKE_MATCH_1}")
        elseif(_line MATCHES "^#define ITSCAM_SDK_VERSION_FULL \"(.*)\"$")
            set(ITSCAM_SDK_VERSION_FULL "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    get_filename_component(ITSCAM_SDK_VERSION_HEADER_DIR "${_itscam_header}" DIRECTORY)
endif()

if(NOT ITSCAM_SDK_VERSION)
    set(_itscam_generate TRUE)
    set(ITSCAM_SDK_VERSION "0.0.0")
    set(ITSCAM_SDK_VERSION_FULL "0.0.0 (unknown)")
    set(ITSCAM_SDK_GIT_SHA "unknown")
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v[0-9]*" --abbrev=12 --dirty
            WORKING_DIRECTORY "${_itscam_root}"
            OUTPUT_VARIABLE _itscam_describe OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _itscam_rc ERROR_QUIET)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
            WORKING_DIRECTORY "${_itscam_root}"
            OUTPUT_VARIABLE _itscam_sha OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(_itscam_rc EQUAL 0 AND _itscam_describe MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)")
            set(ITSCAM_SDK_VERSION "${CMAKE_MATCH_1}")
            string(REGEX REPLACE "^v" "" ITSCAM_SDK_VERSION_FULL "${_itscam_describe}")
        endif()
        if(_itscam_sha)
            set(ITSCAM_SDK_GIT_SHA "${_itscam_sha}")
        endif()
    endif()
endif()

string(REPLACE "." ";" _itscam_parts "${ITSCAM_SDK_VERSION}")
list(GET _itscam_parts 0 ITSCAM_SDK_VERSION_MAJOR)
list(GET _itscam_parts 1 ITSCAM_SDK_VERSION_MINOR)
list(GET _itscam_parts 2 ITSCAM_SDK_VERSION_PATCH)

if(_itscam_generate)
    set(ITSCAM_SDK_GIT_SHA_SHORT "${ITSCAM_SDK_GIT_SHA}")
    string(LENGTH "${ITSCAM_SDK_GIT_SHA}" _itscam_len)
    if(_itscam_len GREATER 12)
        string(SUBSTRING "${ITSCAM_SDK_GIT_SHA}" 0 12 ITSCAM_SDK_GIT_SHA_SHORT)
    endif()
    string(TIMESTAMP ITSCAM_SDK_BUILD_DATE "%Y-%m-%dT%H:%M:%SZ" UTC)
    set(ITSCAM_SDK_VERSION_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/itscam_sdk_generated")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/itscam_sdk_version.h.in"
                   "${ITSCAM_SDK_VERSION_HEADER_DIR}/itscam_sdk_version.h" @ONLY)
endif()
