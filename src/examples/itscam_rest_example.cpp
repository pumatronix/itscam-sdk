/*
 *  itscam_rest_example.cpp
 *
 *  ITSCAM Client SDK - REST API usage example.
 *
 *  Demonstrates: login, read image profiles, read volatile info,
 *  read/write equipment configuration (general, analytics, OCR,
 *  classifier, lanes, ITSCAM PRO).
 *
 *  Build:  make
 *  Run:    ./itscam_rest_example <host> [port] [username] [password]
 */

#include <iostream>
#include <string>
#include "itscam_sdk.h"

// ============================================================================
//  Helpers
// ============================================================================

static void log(const std::string& msg) {
    std::cout << msg << std::endl;
}

static void logErr(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

/// Pretty-print a JSON value (truncated if very large).
static std::string jsonPreview(const nlohmann::json& j, size_t maxLen = 300) {
    std::string s = j.dump(2);
    if (s.size() > maxLen) {
        s.resize(maxLen);
        s += "\n  ... (truncated)";
    }
    return s;
}

/// Render a typed config (or any nlohmann-serialisable value) the same way.
/// Routes through nlohmann::json's adl_serializer for the generated types
/// in itscam_rest_types.hpp.
template <typename T>
static std::string preview(const T& v, size_t maxLen = 300) {
    return jsonPreview(nlohmann::json(v), maxLen);
}

static void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName
              << " <host> [port] [username] [password] [--https] [--insecure]"
              << std::endl;
    std::cerr << std::endl;
    std::cerr << "  host        Hostname or IP of the ITSCAM webapp backend"
              << std::endl;
    std::cerr << "  port        HTTP port (default: 80, or 443 with --https)"
              << std::endl;
    std::cerr << "  username    Login username (default: admin)" << std::endl;
    std::cerr << "  password    Login password (default: 1234)" << std::endl;
    std::cerr << "  --https     Use HTTPS (statically-linked mbedTLS)"
              << std::endl;
    std::cerr << "  --insecure  Skip TLS server certificate verification"
              << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " 192.168.254.254" << std::endl;
    std::cerr << "  " << progName << " 192.168.254.254 80 admin secret"
              << std::endl;
    std::cerr << "  " << progName << " camera.example.com 443 admin secret --https"
              << std::endl;
}

// ============================================================================
//  Main
// ============================================================================

int main(int argc, char* argv[]) {
    using namespace itscam;

    // --- Argument parsing ----------------------------------------------------

    if (argc < 2 || std::string(argv[1]) == "--help" ||
        std::string(argv[1]) == "-h") {
        printUsage(argv[0]);
        return (argc < 2) ? 1 : 0;
    }

    const std::string host = argv[1];
    uint16_t          port = 80;
    std::string       username = "admin";
    std::string       password = "1234";
    bool              useHttps = false;
    bool              insecure = false;
    int posArg = 0;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--https")        useHttps = true;
        else if (a == "--insecure") insecure = true;
        else {
            switch (posArg++) {
                case 0: port     = static_cast<uint16_t>(std::stoi(a)); break;
                case 1: username = a; break;
                case 2: password = a; break;
            }
        }
    }
    if (useHttps && port == 80) port = 443;
    const std::string scheme = useHttps ? "https" : "http";

    log("ITSCAM REST Client SDK Example");
    log("Target: " + scheme + "://" + host + ":" + std::to_string(port));

    // --- SDK setup -----------------------------------------------------------

    ItscamRestClient rest;

    rest.setLogHandler([](LogLevel lvl, const std::string& msg) {
        if (lvl == LogLevel::Error)
            std::cerr << "[REST ERROR] " << msg << std::endl;
        else
            std::cout << "[REST] " << msg << std::endl;
    });

    rest.setBaseUrl(host, port, scheme);

    // ----- TLS configuration (HTTPS only) -----
    if (useHttps) {
        if (insecure) {
            rest.setVerifyServerCertificate(false);
        } else {
            // Production: install the camera's CA bundle and enable
            // verification.  The PEM file can come from your fleet
            // management system or be provisioned by an installer.
            // rest.setCaCertFile("/etc/itscam/ca-bundle.pem");
        }
    }

    // ========================================================================
    //  1. Authentication
    // ========================================================================

    log("");
    log("=== Authentication ===");

    {
        auto result = rest.login(username, password);
        if (!result) {
            logErr("Login failed: " + result.error().message);
            return 1;
        }
        log("Logged in as '" + username + "'");
        log("Response:\n" + jsonPreview(result.value()));
    }

    // ========================================================================
    //  2. Image profiles
    // ========================================================================

    log("");
    log("=== Image Profiles ===");

    {
        auto result = rest.getProfiles();
        if (result) {
            log("Loaded " + std::to_string(result.value().size())
                + " profile(s)");
            log("First profile preview:\n" + preview(result.value().front()));
        } else {
            logErr("getProfiles failed: " + result.error().message);
        }
    }

    // ========================================================================
    //  3. Volatile info (read-only)
    // ========================================================================

    log("");
    log("=== Volatile Info ===");

    {
        auto result = rest.getVolatileInfo();
        if (result) {
            log("Volatile:\n" + preview(result.value()));
        } else {
            logErr("getVolatileInfo failed: " + result.error().message);
        }
    }

    // ========================================================================
    //  4. Equipment general
    // ========================================================================

    log("");
    log("=== Equipment General ===");

    {
        auto result = rest.getGeneralConfig();
        if (result) {
            log("General:\n" + jsonPreview(result.value()));
        } else {
            logErr("getGeneralConfig failed: " + result.error().message);
        }
    }

    // ========================================================================
    //  5. Analytics
    // ========================================================================

    log("");
    log("=== Analytics Configuration ===");

    {
        auto result = rest.getAnalyticsConfig();
        if (result) {
            log("Analytics:\n" + preview(result.value()));
        } else {
            logErr("getAnalyticsConfig failed: " + result.error().message);
        }
    }

    // ========================================================================
    //  6. OCR
    // ========================================================================

    log("");
    log("=== OCR Configuration ===");

    {
        auto result = rest.getOcrConfig();
        if (result) {
            log("OCR:\n" + preview(result.value()));
        } else {
            logErr("getOcrConfig failed: " + result.error().message);
        }
    }

    // ========================================================================
    //  7. Classifier
    // ========================================================================

    log("");
    log("=== Classifier Configuration ===");

    {
        auto result = rest.getClassifierConfig();
        if (result) {
            log("Classifier:\n" + preview(result.value()));
        } else {
            logErr("getClassifierConfig failed: " + result.error().message);
        }
    }

    // ========================================================================
    //  8. Lanes
    // ========================================================================

    log("");
    log("=== Lanes Configuration ===");

    {
        auto result = rest.getLanesConfig();
        if (result) {
            log("Lanes:\n" + jsonPreview(result.value()));
        } else {
            logErr("getLanesConfig failed: " + result.error().message);
        }
    }

    // ========================================================================
    //  9. ITSCAM PRO
    // ========================================================================

    log("");
    log("=== ITSCAM PRO Configuration ===");

    {
        auto result = rest.getItscamproConfig();
        if (result) {
            log("ITSCAM PRO config:\n" + preview(result.value()));
        } else {
            logErr("getItscamproConfig failed: " + result.error().message);
        }

        auto status = rest.getItscamproStatus();
        if (status) {
            log("ITSCAM PRO status:\n" + jsonPreview(status.value()));
        } else {
            logErr("getItscamproStatus failed: " + status.error().message);
        }
    }

    // ========================================================================
    //  10. Generic HTTP escape hatch
    // ========================================================================

    log("");
    log("=== Generic HTTP GET (escape hatch) ===");

    {
        // Same as getGeneralConfig(), but using the generic method
        auto result = rest.httpGet("/api/equipment/general");
        if (result) {
            log("httpGet /api/equipment/general:\n"
                + jsonPreview(result.value()));
        } else {
            logErr("httpGet failed: " + result.error().message);
        }
    }

    log("");
    log("Done.");
    return 0;
}
