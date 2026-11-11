# An application using the SDK through CMake, the way docs/cmake.md says.
#
#   -DCONSUMER_MODE=subproject   add_subdirectory(<sdk>), options passed through
#   -DCONSUMER_MODE=package      find_package(itscam_sdk) from an install
#
# tools/verify/verify.sh builds it with several option sets and toolchains.

cmake_minimum_required(VERSION 3.10)
project(itscam_consumer LANGUAGES C CXX)

set(CONSUMER_MODE "subproject" CACHE STRING "subproject or package")
set(CONSUMER_CXX_STANDARD 17 CACHE STRING "C++ standard of the application")

if(CONSUMER_MODE STREQUAL "subproject")
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../.." itscam-sdk)
    set(_with_rest ${ITSCAM_SDK_WITH_REST})
    set(_with_c_api ${ITSCAM_SDK_WITH_C_API})
    set(_shared ${ITSCAM_SDK_SHARED})
elseif(CONSUMER_MODE STREQUAL "package")
    find_package(itscam_sdk CONFIG REQUIRED)
    set(_with_rest ${itscam_sdk_WITH_REST})
    set(_with_c_api ${itscam_sdk_WITH_C_API})
    set(_shared ${itscam_sdk_SHARED})
else()
    message(FATAL_ERROR "CONSUMER_MODE: subproject or package")
endif()

# The Windows DLL exports the C API only: C++ applications link the static
# library there (docs/cmake.md)
if(NOT (WIN32 AND _shared))
    add_executable(consumer main.cpp)
    set_target_properties(consumer PROPERTIES
        CXX_STANDARD ${CONSUMER_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    target_link_libraries(consumer PRIVATE itscam::sdk)
    if(_with_rest)
        target_compile_definitions(consumer PRIVATE CONSUMER_WITH_REST)
    endif()
endif()

# A C application, through the C API the wrappers bind
if(_with_c_api AND _with_rest)
    add_executable(consumer_c "${CMAKE_CURRENT_LIST_DIR}/../../smoke/smoke.c")
    set_target_properties(consumer_c PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
    target_link_libraries(consumer_c PRIVATE itscam::sdk)
endif()
