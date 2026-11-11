# Cross-compiling for Linux ARMv7 hard-float (ITSCAM450):
#   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-linux-gnueabihf.cmake ...
# Uses arm-linux-gnueabihf-gcc/g++ from ARMHF_TOOLCHAIN_PATH (the builder
# image's Arm GNU-A toolchain is in /opt/cross/armhf/bin) or from PATH.
# Any toolchain whose glibc is 2.23 or newer works (docs/compatibility.md).

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(_prefix "arm-linux-gnueabihf-")
if(DEFINED ENV{ARMHF_TOOLCHAIN_PATH})
    set(_prefix "$ENV{ARMHF_TOOLCHAIN_PATH}/${_prefix}")
elseif(EXISTS "/opt/cross/armhf/bin/arm-linux-gnueabihf-g++")
    set(_prefix "/opt/cross/armhf/bin/${_prefix}")
endif()
set(CMAKE_C_COMPILER "${_prefix}gcc")
set(CMAKE_CXX_COMPILER "${_prefix}g++")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
