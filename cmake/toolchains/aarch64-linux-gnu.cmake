# Cross-compiling for Linux ARM64 (ITSCAM600, ITSCAM450b):
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake ...
# Uses aarch64-linux-gnu-gcc/g++ from ARM64_TOOLCHAIN_PATH (the builder
# image's Arm GNU-A toolchain is in /opt/cross/arm64/bin) or from PATH.
# Any toolchain whose glibc is 2.23 or newer works (docs/compatibility.md).

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(_prefix "aarch64-linux-gnu-")
if(DEFINED ENV{ARM64_TOOLCHAIN_PATH})
    set(_prefix "$ENV{ARM64_TOOLCHAIN_PATH}/${_prefix}")
elseif(EXISTS "/opt/cross/arm64/bin/aarch64-linux-gnu-g++")
    set(_prefix "/opt/cross/arm64/bin/${_prefix}")
endif()
set(CMAKE_C_COMPILER "${_prefix}gcc")
set(CMAKE_CXX_COMPILER "${_prefix}g++")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
