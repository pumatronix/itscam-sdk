# ItscamSdkOptions.cmake -- options a parent project can set with set().
#
# Copyright (c) 2026 Pumatronix
#
#   itscam_sdk_option(<name> "<doc>" <default>)       a boolean, as option()
#   itscam_sdk_setting(<name> <default> "<doc>")      a string, as set(CACHE)
#
# docs/cmake.md tells a parent project to set(ITSCAM_SDK_...) before
# add_subdirectory().  Plain option() drops such a variable before CMake 3.13
# (policy CMP0077), and set(... CACHE ...) before 3.21 (CMP0126), on the first
# configuration: the SDK then built with its defaults.  These keep the
# parent's value on every CMake version, and still take -D from the command
# line.

macro(itscam_sdk_option _itscam_name _itscam_doc _itscam_default)
    if(DEFINED ${_itscam_name})
        set(_itscam_value "${${_itscam_name}}")
        option(${_itscam_name} "${_itscam_doc}" "${_itscam_value}")
        set(${_itscam_name} "${_itscam_value}")
    else()
        option(${_itscam_name} "${_itscam_doc}" ${_itscam_default})
    endif()
endmacro()

macro(itscam_sdk_setting _itscam_name _itscam_default _itscam_doc)
    if(DEFINED ${_itscam_name})
        set(_itscam_value "${${_itscam_name}}")
        set(${_itscam_name} "${_itscam_value}" CACHE STRING "${_itscam_doc}")
        set(${_itscam_name} "${_itscam_value}")
    else()
        set(${_itscam_name} "${_itscam_default}" CACHE STRING "${_itscam_doc}")
    endif()
endmacro()
