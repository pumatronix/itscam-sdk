# Smoke tests, with ITSCAM_SDK_BUILD_TESTS=ON (tests/README.md).
#
# They talk to tests/standin/server.js: start it, then run ctest with
# ITSCAM_TEST_HOST / ITSCAM_TEST_PORT pointing at it (default
# 127.0.0.1:18080).  tools/verify/verify.sh does all of this.

add_executable(itscam_smoke_cpp smoke/smoke.cpp)
target_link_libraries(itscam_smoke_cpp PRIVATE itscam::sdk)
add_test(NAME smoke_cpp COMMAND itscam_smoke_cpp)

if(ITSCAM_SDK_WITH_C_API AND ITSCAM_SDK_WITH_REST)
    add_executable(itscam_smoke_c smoke/smoke.c)
    set_target_properties(itscam_smoke_c PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
    target_link_libraries(itscam_smoke_c PRIVATE itscam::sdk)
    add_test(NAME smoke_c COMMAND itscam_smoke_c)
endif()
