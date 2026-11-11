# C++ examples, with ITSCAM_SDK_BUILD_EXAMPLES=ON (see src/examples/Makefile
# for the release builds).  Each example links itscam::sdk.

file(GLOB _itscam_examples RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(_source IN LISTS _itscam_examples)
    get_filename_component(_name "${_source}" NAME_WE)
    add_executable(${_name} "${_source}")
    target_link_libraries(${_name} PRIVATE itscam::sdk)
endforeach()
