# C++ examples, with ITSCAM_SDK_BUILD_EXAMPLES=ON (see src/examples/Makefile
# for the release builds).  Each example links itscam::sdk, and is built only
# when the library has the clients it uses.

# The Windows DLL exports the C API only (docs/cmake.md)
if(WIN32 AND ITSCAM_SDK_SHARED)
    message(STATUS "itscam_sdk: no C++ examples against the Windows DLL; build the static library")
    return()
endif()

file(GLOB _itscam_examples RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(_source IN LISTS _itscam_examples)
    get_filename_component(_name "${_source}" NAME_WE)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${_source}" _text)
    set(_missing "")
    if(_text MATCHES "ItscamRestClient" AND NOT ITSCAM_SDK_WITH_REST)
        set(_missing "the REST client")
    elseif(_text MATCHES "ItscamCgiClient" AND NOT ITSCAM_SDK_WITH_CGI)
        set(_missing "the CGI client")
    elseif(_text MATCHES "ITSCAM_[A-Za-z]+_?[A-Za-z]*\\(" AND NOT ITSCAM_SDK_WITH_C_API)
        set(_missing "the C API")
    endif()
    if(_missing)
        message(STATUS "itscam_sdk: skipping ${_name}, which needs ${_missing}")
    else()
        add_executable(${_name} "${_source}")
        target_link_libraries(${_name} PRIVATE itscam::sdk)
    endif()
endforeach()
