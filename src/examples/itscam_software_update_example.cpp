/*
 *  itscam_software_update_example.cpp
 *
 *  ITSCAM Client SDK - REST software update example.
 *
 *  Demonstrates: login, authenticated SWU archive upload through the webapp
 *  backend, websocket status follow-up, and optional restart request.
 *
 *  Build:  make
 *  Run:    ./itscam_software_update_example <host> <archive.swu>
 */

#include <iostream>
#include <string>
#include "itscam_sdk.h"

static void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName
              << " <host> <archive.swu> [port] [username] [password]"
              << " [--https] [--insecure] [--restart]"
              << std::endl;
}

int main(int argc, char* argv[]) {
    using namespace itscam;

    if (argc < 3 || std::string(argv[1]) == "--help" ||
        std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return (argc < 3) ? 1 : 0;
    }

    const std::string host = argv[1];
    const std::string swuPath = argv[2];
    uint16_t port = 80;
    std::string username = "admin";
    std::string password = "1234";
    bool useHttps = false;
    bool insecure = false;
    bool restart = false;

    int posArg = 0;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--https") {
            useHttps = true;
        } else if (arg == "--insecure") {
            insecure = true;
        } else if (arg == "--restart") {
            restart = true;
        } else {
            switch (posArg++) {
                case 0: port = static_cast<uint16_t>(std::stoi(arg)); break;
                case 1: username = arg; break;
                case 2: password = arg; break;
                default: break;
            }
        }
    }

    if (useHttps && port == 80) port = 443;
    const std::string scheme = useHttps ? "https" : "http";

    ItscamRestClient rest;
    rest.setBaseUrl(host, port, scheme);
    if (useHttps && insecure) {
        rest.setVerifyServerCertificate(false);
    }

    auto login = rest.login(username, password);
    if (!login) {
        std::cerr << "Login failed: " << login.error().message << std::endl;
        return 1;
    }

    ItscamRestClient::SoftwareUpdateOptions options;
    options.swuPath = swuPath;
    options.requestRestart = restart;

    int lastPercent = -1;
    auto operation = rest.startSoftwareUpdate(
        options,
        [&lastPercent](const ItscamRestClient::SoftwareUpdateStatus& status) {
            if (status.uploadTotal > 0) {
                int percent = static_cast<int>(
                    (status.uploadCurrent * 100) / status.uploadTotal);
                if (percent != lastPercent) {
                    lastPercent = percent;
                    std::cout << "Upload: " << percent << "%\r" << std::flush;
                }
            }
            if (!status.installStatus.empty()) {
                std::cout << "\nSWUpdate: " << status.installStatus
                          << std::endl;
            }
            if (!status.stepName.empty()) {
                std::cout << "Step: " << status.stepName;
                if (status.stepPercent >= 0) {
                    std::cout << " " << status.stepPercent << "%";
                }
                std::cout << std::endl;
            }
            if (!status.message.empty()) {
                std::cout << "Message: " << status.message << std::endl;
            }
        });

    if (!operation) {
        std::cerr << "Update start failed: " << operation.error().message
                  << std::endl;
        return 1;
    }

    auto finalStatus = operation.value().wait();
    std::cout << std::endl;
    if (!finalStatus) {
        std::cerr << "Update wait failed: " << finalStatus.error().message
                  << std::endl;
        return 1;
    }

    std::cout << "Final status: " << finalStatus.value().toJson().dump()
              << std::endl;
    if (finalStatus.value().phase !=
        ItscamRestClient::SoftwareUpdatePhase::Succeeded) {
        std::cerr << "Software update did not complete successfully."
                  << std::endl;
        return 1;
    }

    return 0;
}