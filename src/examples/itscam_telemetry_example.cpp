/*
 *  itscam_telemetry_example.cpp
 *
 *  ITSCAM Client SDK - system telemetry example.
 *
 *  Reads the state of the whole camera once, then follows the documents the
 *  camera pushes for a while.
 *
 *  Build:  make
 *  Run:    ./itscam_telemetry_example <host> <username> <password>
 *              [--seconds N] [--https] [--insecure]
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include "itscam_sdk.h"

using namespace itscam;

static void printHealth(const rest_types::SystemTelemetry& telemetry) {
    if (!telemetry.health) {
        std::cout << "health: unavailable" << std::endl;
        return;
    }
    const auto& health = *telemetry.health;
    std::cout << "health: " << health.status.value_or("?") << std::endl;
    if (health.issues) {
        for (const auto& issue : *health.issues) {
            std::cout << "  [" << issue.severity << "] " << issue.id << ": "
                      << issue.message << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <host> <username> <password>"
                     " [--seconds N] [--https] [--insecure]" << std::endl;
        return 1;
    }
    int seconds = 30;
    bool useHttps = false;
    bool insecure = false;
    for (int i = 4; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--seconds" && i + 1 < argc) seconds = std::atoi(argv[++i]);
        else if (arg == "--https") useHttps = true;
        else if (arg == "--insecure") insecure = true;
    }

    ItscamRestClient rest;
    rest.setBaseUrl(argv[1], useHttps ? 443 : 80, useHttps ? "https" : "http");
    if (insecure) rest.setVerifyServerCertificate(false);

    auto login = rest.login(argv[2], argv[3]);
    if (!login) {
        std::cerr << "login failed: " << login.error().message << std::endl;
        return 1;
    }

    // Polling: one read of the whole camera
    auto telemetry = rest.getSystemTelemetry();
    if (!telemetry) {
        std::cerr << "read failed: " << telemetry.error().message << std::endl;
        return 1;
    }
    printHealth(telemetry.value());

    // Stream: one document at once, then one every 5 s.  The callback runs
    // on the SDK's callback pool: do not block in it.
    auto started = rest.startTelemetryStream([](const TelemetryUpdate& update) {
        std::cout << "\nupdate #" << update.sequence << std::endl;
        auto typed = update.telemetry();
        if (typed) printHealth(typed.value());
    });
    if (!started) {
        std::cerr << "stream failed: " << started.error().message << std::endl;
        return 1;
    }
    os::sleepForMs(seconds * 1000);
    rest.stopTelemetryStream();

    auto latest = rest.latestTelemetry();
    if (latest) {
        std::cout << "\nlast document: #" << latest.value().sequence << std::endl;
    }
    return 0;
}
