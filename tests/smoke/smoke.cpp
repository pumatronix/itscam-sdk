/*
 *  smoke.cpp -- C++ consumer smoke test, against tests/standin/server.js.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Valid C++14 and C++17: tools/verify builds it with each standard and
 *  each supported compiler, then runs it on every Linux architecture.  The
 *  checks match the other languages' smoke tests (see tests/README.md).
 *
 *  Environment: ITSCAM_TEST_HOST (127.0.0.1), ITSCAM_TEST_PORT (18080).
 */

#include <cstdio>
#include <cstdlib>
#include <string>

#include "itscam_sdk.h"
#include "itscam_jpeg_utils.h"
#include "c_api/itscam_sdk_c.h"

using namespace itscam;

static int failures = 0;

static void check(bool ok, const char* name, const std::string& detail = "") {
    std::printf("%s %s%s%s\n", ok ? "PASS" : "FAIL", name,
                detail.empty() ? "" : ": ", detail.c_str());
    if (!ok) ++failures;
}

static std::string env(const char* name, const char* fallback) {
    const char* v = std::getenv(name);
    return v && *v ? v : fallback;
}

int main() {
    const std::string host = env("ITSCAM_TEST_HOST", "127.0.0.1");
    const uint16_t port =
        static_cast<uint16_t>(std::atoi(env("ITSCAM_TEST_PORT", "18080").c_str()));
    std::printf("C++ smoke (__cplusplus %ld) against %s:%u\n",
                static_cast<long>(__cplusplus), host.c_str(), port);

    const std::string version = ITSCAM_getVersion();
    check(!version.empty(), "version", version);

    {
        ItscamRestClient rest;
        rest.setBaseUrl(host, port, "http");
        auto login = rest.login("admin", "1234");
        check(static_cast<bool>(login), "rest login",
              login ? "" : login.error().message);

        auto profiles = rest.getProfiles();
        if (!profiles) {
            check(false, "rest typed get", profiles.error().message);
        } else {
            const auto& list = profiles.value();
            const bool ok = list.size() == 1 && list[0].id == 3 && list[0].name &&
                            *list[0].name == "C\xC3\xA2mera S\xC3\xA3o Jo\xC3\xA3o";
            check(ok, "rest typed get",
                  list.empty() ? "no profile"
                               : "id " + std::to_string(list[0].id) + ", name '" +
                                     list[0].name.value_or("<unset>") + "'");
        }

        auto missing = rest.httpGet("/api/does-not-exist");
        check(!missing && missing.error().code == Error::InvalidParameter,
              "rest 404 maps to InvalidParameter",
              missing ? "succeeded" : missing.error().message);
    }

    {
        ItscamCgiClient cgi;
        cgi.setBaseUrl(host, port, "http");
        auto images = cgi.getSnapshot(SnapshotCgiRequest{});
        if (!images || images.value().size() != 1) {
            check(false, "cgi snapshot",
                  images ? "image count " + std::to_string(images.value().size())
                         : images.error().message);
        } else {
            const auto tags = parseJpegCommentTags(images.value()[0].data);
            const auto plates = extractPlateRecognitions(tags);
            check(plates.size() == 1 && plates[0].plate == "ABC1D23",
                  "cgi snapshot + jpeg plate",
                  plates.empty() ? "no plate" : plates[0].plate);
        }
    }

    {
        // Nothing listens on port 1: fails fast and leaves no thread behind
        ItscamClient client;
        auto connected = client.connect(host, 1, 2000);
        check(!connected, "binary connect to a closed port fails",
              connected ? "connected" : connected.error().message);
    }

    std::printf("%s\n", failures ? "C++ smoke: FAILED" : "C++ smoke: OK");
    return failures ? 1 : 0;
}
