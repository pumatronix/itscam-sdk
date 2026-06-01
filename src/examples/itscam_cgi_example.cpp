/*
 *  itscam_cgi_example.cpp
 *
 *  ITSCAM Client SDK - CGI API usage example.
 *
 *  Demonstrates the three groups of CGI endpoints the SDK supports:
 *    1. snapshot.cgi  -- one-shot capture with multipart multi-exposure
 *                        support
 *    2. lastframe.cgi -- most recent preview frame
 *    3. mjpegvideo.cgi -- streaming MJPEG captured for 5 seconds
 *
 *  Build: make
 *  Run:   ./itscam_cgi_example <host> [port] [options]
 *
 *  CGI endpoints are unauthenticated by default on the camera
 *  (configCgi.blockAPI = false), so credentials are optional and only
 *  passed via `--user <name> --password <pwd>` when the camera has CGI
 *  auth enabled.  Add `--https` to use TLS (mbedTLS).
 */

#include "itscam_sdk.h"

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

using itscam::CgiImage;
using itscam::ItscamCgiClient;
using itscam::LogLevel;
using itscam::SnapshotCgiRequest;

namespace {

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    std::cout << "  wrote " << data.size() << " bytes -> " << path << "\n";
}

void printUsage(const char* prog) {
    std::cerr
        << "Usage: " << prog
        << " <host> [port] [--https] [--insecure]"
           " [--user <name> --password <pwd>]\n"
        << "\n  --https              use HTTPS (default scheme is http)"
        << "\n  --insecure           skip TLS server certificate verification"
        << "\n  --user <name>        opt-in CGI authentication user"
        << "\n  --password <pwd>     opt-in CGI authentication password"
        << "\n\nCGI auth is normally disabled (configCgi.blockAPI=false);"
           " omit --user/--password\nunless your camera has it turned on.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) { printUsage(argv[0]); return 1; }

    std::string host = argv[1];
    uint16_t    port = 80;
    std::string user;
    std::string pass;
    bool        useHttps = false;
    bool        insecure = false;

    int posArg = 0;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--https") {
            useHttps = true;
        } else if (a == "--insecure") {
            insecure = true;
        } else if (a == "--user" && i + 1 < argc) {
            user = argv[++i];
        } else if (a == "--password" && i + 1 < argc) {
            pass = argv[++i];
        } else if (posArg == 0) {
            port = static_cast<uint16_t>(std::stoi(a));
            ++posArg;
        } else {
            std::cerr << "Unexpected argument: " << a << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    if (useHttps && port == 80) port = 443;
    std::string scheme = useHttps ? "https" : "http";

    ItscamCgiClient cgi;
    cgi.setLogHandler([](LogLevel lvl, const std::string& msg) {
        std::cout << "[" << (lvl == LogLevel::Error ? "ERR " : "INFO")
                  << "] " << msg << std::endl;
    });
    cgi.setBaseUrl(host, port, scheme);
    if (useHttps && insecure) cgi.setVerifyServerCertificate(false);

    if (!user.empty() && !pass.empty()) {
        std::cout << "Logging in as '" << user << "' at " << scheme
                  << "://" << host << ":" << port << " ...\n";
        auto loginRes = cgi.login(user, pass);
        if (!loginRes) {
            std::cerr << "Login failed: " << loginRes.error().message
                      << std::endl;
            return 2;
        }
    } else {
        std::cout << "No credentials provided; talking to "
                  << scheme << "://" << host << ":" << port
                  << " without CGI authentication.\n";
    }

    // ----- /api/lastframe.cgi -----
    std::cout << "\nFetching lastframe.cgi ...\n";
    auto lastRes = cgi.getLastFrame();
    if (lastRes) {
        std::cout << "  mime=" << lastRes.value().mimeType
                  << ", bytes=" << lastRes.value().data.size() << "\n";
        writeFile("lastframe.jpg", lastRes.value().data);
    } else {
        std::cerr << "  failed: " << lastRes.error().message << "\n";
    }

    // ----- /api/snapshot.cgi -----
    std::cout << "\nTriggering snapshot.cgi (Q=80, mosaic off) ...\n";
    SnapshotCgiRequest snapReq;
    snapReq.quality = 80;
    snapReq.mosaic  = false;
    auto snapRes = cgi.getSnapshot(snapReq);
    if (snapRes) {
        const auto& images = snapRes.value();
        std::cout << "  received " << images.size() << " image(s)\n";
        for (size_t i = 0; i < images.size(); ++i) {
            char path[64];
            std::snprintf(path, sizeof(path), "snapshot-%zu.jpg", i);
            std::cout << "    " << path << " mime=" << images[i].mimeType
                      << "\n";
            writeFile(path, images[i].data);
        }
    } else {
        std::cerr << "  failed: " << snapRes.error().message << "\n";
    }

    // ----- /api/mjpegvideo.cgi -----
    std::cout << "\nStreaming mjpegvideo.cgi for 5 seconds...\n";
    std::atomic<int> frameCount{0};
    cgi.startMjpegStream([&](const itscam::CgiStreamFrame& f) {
        int n = ++frameCount;
        if (n == 1) writeFile("mjpeg-first.jpg", f.data);
        if ((n % 5) == 0) {
            std::cout << "  received frame " << n << " ("
                      << f.data.size() << " bytes)\n";
        }
    });
    itscam::os::sleepForMs(5000);
    cgi.stopMjpegStream();
    std::cout << "  stopped after " << frameCount.load() << " frame(s)\n";

    std::cout << "\nAll done.\n";
    return 0;
}
