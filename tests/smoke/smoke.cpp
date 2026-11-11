/*
 *  smoke.cpp -- C++ consumer smoke test, against tests/standin/server.js.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Valid C++14 and C++17: tools/verify builds it with each standard and
 *  each supported compiler, then runs it on every Linux architecture.  The
 *  checks match the other languages' smoke tests (see tests/README.md).
 *
 *  Environment: ITSCAM_TEST_HOST (127.0.0.1), ITSCAM_TEST_PORT (18080),
 *  ITSCAM_TEST_TLS_PORT (18443), ITSCAM_TEST_CA (tests/standin/tls/cert.pem).
 */

#include <atomic>
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

    // ITSCAM_TEST_EXPECT_VERSION: the library under test, so that a stale
    // copy found first on the search path fails instead of passing
    const std::string version = ITSCAM_getVersion();
    const std::string expected = env("ITSCAM_TEST_EXPECT_VERSION", "");
    check(!version.empty() && (expected.empty() || version == expected), "version",
          expected.empty() || version == expected ? version
                                                  : version + ", expected " + expected);

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
        ItscamRestClient rest;
        rest.setBaseUrl(host, port, "http");
        rest.login("admin", "1234");
        auto telemetry = rest.getSystemTelemetry();
        if (!telemetry) {
            check(false, "telemetry read", telemetry.error().message);
        } else {
            const auto& health = telemetry.value().health;
            const bool ok = health && health->status && *health->status == "warning" &&
                            health->issues && health->issues->size() == 1 &&
                            (*health->issues)[0].id == "storageFull";
            check(ok, "telemetry read",
                  health && health->status ? "health " + *health->status : "no health");
        }

        // The stand-in pushes every 200 ms
        std::atomic<int> received{0};
        auto started = rest.startTelemetryStream(
            [&received](const TelemetryUpdate&) { ++received; });
        for (int i = 0; i < 100 && received < 2; ++i) os::sleepForMs(50);
        rest.stopTelemetryStream();
        auto latest = rest.latestTelemetry();
        check(started && received >= 2 && latest && latest.value().sequence >= 2,
              "telemetry stream",
              started ? std::to_string(received.load()) + " documents" : started.error().message);
    }

    {
        // The stand-in's certificate is self-signed: nobody trusts it
        const uint16_t tlsPort = static_cast<uint16_t>(
            std::atoi(env("ITSCAM_TEST_TLS_PORT", "18443").c_str()));
        ItscamRestClient untrusting;
        untrusting.setBaseUrl(host, tlsPort, "https");
        auto refused = untrusting.login("admin", "1234");
        check(!refused, "https refuses an untrusted certificate",
              refused ? "accepted" : refused.error().message);

        ItscamRestClient rest;
        rest.setBaseUrl(host, tlsPort, "https");
        rest.setCaCertFile(env("ITSCAM_TEST_CA", "tests/standin/tls/cert.pem"));
        auto login = rest.login("admin", "1234");
        auto profiles = rest.getProfiles();
        check(login && profiles && profiles.value().size() == 1,
              "https with the certificate trusted",
              login ? (profiles ? "" : profiles.error().message) : login.error().message);

        std::atomic<int> received{0};
        auto started = rest.startTelemetryStream(
            [&received](const TelemetryUpdate&) { ++received; });
        for (int i = 0; i < 100 && received < 2; ++i) os::sleepForMs(50);
        rest.stopTelemetryStream();
        check(started && received >= 2, "telemetry stream over TLS",
              started ? std::to_string(received.load()) + " documents" : started.error().message);
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
