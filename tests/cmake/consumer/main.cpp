// An application linking itscam::sdk (tests/cmake/consumer/CMakeLists.txt).
// Uses only what every option set builds: the binary client, plus the REST
// client when the build has it.  Exits 0 when the library works.

#include <cstdio>

#include "itscam_sdk.h"

int main() {
    std::printf("itscam_sdk %s, C++ %ld\n", ITSCAM_SDK_VERSION_STRING,
                static_cast<long>(__cplusplus));

    // Nothing listens on port 1: the client fails fast and cleans up
    itscam::ItscamClient client;
    auto connected = client.connect("127.0.0.1", 1, 2000);
    if (connected) {
        std::printf("FAIL connected to port 1\n");
        return 1;
    }
    std::printf("binary client: %s\n", connected.error().message.c_str());

#ifdef CONSUMER_WITH_REST
    itscam::ItscamRestClient rest;
    rest.setBaseUrl("127.0.0.1", 1, "http");
    auto profiles = rest.getProfiles(2000);
    if (profiles) {
        std::printf("FAIL REST call to port 1 succeeded\n");
        return 1;
    }
    std::printf("REST client: %s\n", profiles.error().message.c_str());
#endif
    std::printf("consumer: OK\n");
    return 0;
}
