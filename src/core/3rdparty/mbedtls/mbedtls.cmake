# mbedtls.cmake
#
# CMake fragment that builds the vendored mbedTLS sources into an
# OBJECT library so they can be archived into the SDK.  Included from
# core/CMakeLists.txt via `include(.../3rdparty/mbedtls/mbedtls.cmake)`.
#
# Outputs the target `itscam_mbedtls_objects` (OBJECT library) when
# mbedTLS sources are present.  Sources are pre-vendored under
# 3rdparty/mbedtls/; if they have been pruned, the target is created
# empty and a warning is emitted so the SDK still builds (without HTTPS
# support).
#
# Copyright (c) 2026 Pumatronix

set(MBEDTLS_ROOT       "${CMAKE_CURRENT_LIST_DIR}")
set(MBEDTLS_INCLUDE    "${MBEDTLS_ROOT}/include")
set(MBEDTLS_LIBRARY    "${MBEDTLS_ROOT}/library")
set(MBEDTLS_CONFIG_DIR "${MBEDTLS_ROOT}")

file(GLOB MBEDTLS_SOURCES "${MBEDTLS_LIBRARY}/*.c")

if(NOT MBEDTLS_SOURCES)
    message(WARNING
        "mbedTLS sources not found in ${MBEDTLS_LIBRARY}. "
        "The SDK will build without HTTPS support; restore the vendored "
        "tree from git to re-enable it.")
    add_library(itscam_mbedtls_objects INTERFACE)
else()
    add_library(itscam_mbedtls_objects OBJECT ${MBEDTLS_SOURCES})
    set_target_properties(itscam_mbedtls_objects PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    target_include_directories(itscam_mbedtls_objects PUBLIC
        ${MBEDTLS_INCLUDE}
        ${MBEDTLS_CONFIG_DIR})
    target_compile_definitions(itscam_mbedtls_objects PUBLIC
        MBEDTLS_USER_CONFIG_FILE="itscam_mbedtls_config.h")
    if(WIN32)
        target_link_libraries(itscam_mbedtls_objects INTERFACE bcrypt)
    endif()
endif()
