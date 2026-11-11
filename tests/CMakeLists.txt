# Smoke tests, with ITSCAM_SDK_BUILD_TESTS=ON (tests/README.md).
#
# They talk to tests/standin/server.js: start it, then run ctest with
# ITSCAM_TEST_HOST / ITSCAM_TEST_PORT pointing at it (default
# 127.0.0.1:18080), and ITSCAM_TEST_CA at tests/standin/tls/cert.pem.
# tools/verify/verify.sh does all of this.
#
# Each test is registered only when the library has what it uses.

# smoke.cpp: the three clients and ITSCAM_getVersion(); not the Windows DLL,
# which exports the C API only
if(ITSCAM_SDK_WITH_REST AND ITSCAM_SDK_WITH_CGI AND ITSCAM_SDK_WITH_C_API
   AND NOT (WIN32 AND ITSCAM_SDK_SHARED))
    add_executable(itscam_smoke_cpp smoke/smoke.cpp)
    target_link_libraries(itscam_smoke_cpp PRIVATE itscam::sdk)
    add_test(NAME smoke_cpp COMMAND itscam_smoke_cpp)
else()
    message(STATUS "itscam_sdk: no C++ smoke test (needs REST, CGI, the C API, and not the Windows DLL)")
endif()

# smoke.c: the C API of the binary and REST clients
if(ITSCAM_SDK_WITH_C_API AND ITSCAM_SDK_WITH_REST)
    add_executable(itscam_smoke_c smoke/smoke.c)
    set_target_properties(itscam_smoke_c PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)
    target_link_libraries(itscam_smoke_c PRIVATE itscam::sdk)
    add_test(NAME smoke_c COMMAND itscam_smoke_c)
else()
    message(STATUS "itscam_sdk: no C smoke test (needs the C API and REST)")
endif()
