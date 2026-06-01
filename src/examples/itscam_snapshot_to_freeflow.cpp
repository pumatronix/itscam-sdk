/*
 *  itscam_snapshot_to_freeflow.cpp
 *
 *  ITSCAM Client SDK - Snapshot-to-Freeflow Swap Example
 *
 *  Demonstrates the two main ITSCAM operational modes and how to switch
 *  between them at runtime using the SDK:
 *
 *    Mode 1 - SNAPSHOT:  On-demand image capture via snapshot.cgi (CGI
 *             client).  The camera sits idle until the application
 *             requests an image.
 *
 *    Mode 2 - FREEFLOW:  Continuous trigger with majority voting and
 *             automatic plate dispatch via the REST API Client (RAC)
 *             service.  The camera continuously captures, runs on-device
 *             OCR with voting, and POSTs results (plate + timestamp +
 *             JPEG base64) to an external HTTP endpoint.
 *
 *  When switching to freeflow, the example configures:
 *    - Day profile (Diurno) with single exposure + constant trigger
 *    - Night profile (Noturno) with 2 multi-exposure steps at different
 *      flash power + constant trigger
 *    - Analytics voting (majority voting) for plate accuracy
 *    - RAC server with a simple JSON body template
 *
 *  Build: make
 *  Run:   ./itscam_snapshot_to_freeflow <host> <user> <password> [options]
 *
 *  Copyright (c) 2026 Pumatronix
 */

#include "itscam_sdk.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace itscam;

//=========================================================================
// Helpers
//=========================================================================

static void log(const std::string& msg) {
    std::cout << msg << std::endl;
}

static void logErr(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

static std::string jsonPreview(const nlohmann::json& j, size_t maxLen = 400) {
    std::string s = j.dump(2);
    if (s.size() > maxLen) {
        s.resize(maxLen);
        s += "\n  ... (truncated)";
    }
    return s;
}

template <typename T>
static std::string preview(const T& v, size_t maxLen = 400) {
    return jsonPreview(nlohmann::json(v), maxLen);
}

static void writeFile(const std::string& path,
                      const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    log("  wrote " + std::to_string(data.size()) + " bytes -> " + path);
}

static void printUsage(const char* prog) {
    std::cerr
        << "Usage: " << prog
        << " <host> <user> <password> [options]\n"
        << "\nOptions:\n"
        << "  --https              use HTTPS (default: HTTP)\n"
        << "  --strict-tls         require valid TLS certificate (default: insecure)\n"
        << "  --rac-host <host>    RAC destination host (default: 192.168.0.10)\n"
        << "  --rac-port <port>    RAC destination port (default: 8080)\n"
        << "  --rac-path <path>    RAC destination path (default: /api/captures)\n"
        << "  --day-profile <n>    day profile name (default: Diurno)\n"
        << "  --profile-name <n>   night profile name to configure (default: Noturno)\n"
        << "  --duration <sec>     freeflow run duration in seconds (default: 30)\n"
        << "\nThis example demonstrates switching between snapshot mode\n"
        << "(on-demand CGI captures) and freeflow mode (continuous trigger\n"
        << "with voting + RAC integration).\n";
}

//=========================================================================
// Configuration
//=========================================================================

struct Config {
    std::string host;
    std::string user;
    std::string password;
    bool        useHttps       = false;
    bool        insecure       = true;
    std::string racHost        = "192.168.0.10";    // <-- CHANGE THIS to your RAC destination host (e.g. a server on the LAN)
    int         racPort        = 8080;
    std::string racPath        = "/api/captures";
    std::string dayProfileName = "Diurno";
    std::string profileName    = "Noturno";
    int         durationSec    = 30;
};

static bool parseArgs(int argc, char* argv[], Config& cfg) {
    if (argc < 4) return false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") return false;
    }

    cfg.host     = argv[1];
    cfg.user     = argv[2];
    cfg.password = argv[3];

    for (int i = 4; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--https") {
            cfg.useHttps = true;
        } else if (a == "--strict-tls") {
            cfg.insecure = false;
        } else if (a == "--rac-host" && i + 1 < argc) {
            cfg.racHost = argv[++i];
        } else if (a == "--rac-port" && i + 1 < argc) {
            cfg.racPort = std::stoi(argv[++i]);
        } else if (a == "--rac-path" && i + 1 < argc) {
            cfg.racPath = argv[++i];
        } else if (a == "--day-profile" && i + 1 < argc) {
            cfg.dayProfileName = argv[++i];
        } else if (a == "--profile-name" && i + 1 < argc) {
            cfg.profileName = argv[++i];
        } else if (a == "--duration" && i + 1 < argc) {
            cfg.durationSec = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << a << "\n";
            return false;
        }
    }
    return true;
}

//=========================================================================
// Phase 1 - Snapshot mode (CGI)
//=========================================================================

static int runSnapshotMode(const Config& cfg) {
    log("");
    log("============================================================");
    log("  PHASE 1 - SNAPSHOT MODE (on-demand via snapshot.cgi)");
    log("============================================================");

    uint16_t port = cfg.useHttps ? 443 : 80;
    std::string scheme = cfg.useHttps ? "https" : "http";

    ItscamCgiClient cgi;
    cgi.setLogHandler([](LogLevel lvl, const std::string& msg) {
        if (lvl == LogLevel::Error)
            std::cerr << "[CGI ERR] " << msg << std::endl;
    });
    cgi.setBaseUrl(cfg.host, port, scheme);
    if (cfg.useHttps && cfg.insecure)
        cgi.setVerifyServerCertificate(false);

    // CGI auth is off by default; we don't call login() unless needed.
    log("Taking 3 on-demand snapshots via snapshot.cgi ...");

    for (int shot = 1; shot <= 3; ++shot) {
        log("\n--- Snapshot " + std::to_string(shot) + "/3 ---");

        SnapshotCgiRequest req;
        req.quality = 85;
        req.mosaic  = false;

        auto result = cgi.getSnapshot(req);
        if (!result) {
            logErr("snapshot.cgi failed: " + result.error().message);
            continue;
        }

        const auto& images = result.value();
        log("  received " + std::to_string(images.size()) + " image(s)");
        for (size_t i = 0; i < images.size(); ++i) {
            char path[128];
            std::snprintf(path, sizeof(path),
                          "snapshot_phase1_%d_exp%zu.jpg", shot, i);
            writeFile(path, images[i].data);
        }

        if (shot < 3) itscam::os::sleepForMs(1000);
    }

    log("\nSnapshot mode complete.");
    return 0;
}

//=========================================================================
// Phase 2 - Freeflow mode (REST config + binary capture)
//=========================================================================

/// Build the RAC configuration using the typed RestApiClientConfig struct.
static rest_types::RestApiClientConfig buildRacConfig(const Config& cfg) {

    // JSON body template -- the camera replaces the {{...}} placeholders.
    // See docs for the full list of available variables:
    //   {{plate}}     -> recognised plate string
    //   {{image}}     -> JPEG image encoded as base64
    //   {{utcYear}}, {{utcMonth}}, ... -> UTC timestamp components
    //   {{recognitionList}} -> full JSON array of all recognitions
    std::string bodyTemplate = R"({
    "plate":"{{plate}}",
    "timestamp":"{{utcYear}}-{{utcMonth}}-{{utcDay}}T{{utcHours}}:{{utcMinutes}}:{{utcSeconds}}.{{utcMilliseconds}}Z",
    "image":"{{image}}"
})";

    rest_types::Part part;
    part.data    = bodyTemplate;
    part.name    = "";
    part.type    = rest_types::Type::JSON;

    rest_types::Body body;
    body.parts   = {part};
    body.variant = rest_types::Variant::SINGLEPART;

    rest_types::Header contentType;
    contentType.name  = "Content-Type";
    contentType.value = "application/json";

    rest_types::Jpeg jpeg;
    jpeg.quality    = 85;
    jpeg.resolution = {0, 0};  // use camera default

    // Persistency: buffer captures on disk when the server is unreachable.
    // When the connection is restored the camera drains the backlog.
    rest_types::Persistency pers;
    pers.enabled        = true;
    pers.max_disk_usage = 2147483648;  // 2 GiB
    pers.max_file_age   = 604800;      // 7 days
    pers.newest_first   = false;

    rest_types::Tls tls;
    tls.insecure  = false;
    tls.mtls_key  = "";

    std::string racHostPort = cfg.racHost + ":" + std::to_string(cfg.racPort);

    rest_types::Url url;
    url.host   = racHostPort;
    url.path   = cfg.racPath;
    url.query  = {};
    url.scheme = rest_types::Scheme::HTTP;

    rest_types::RestApiClientConfig rac;
    rac.enabled                  = true;
    rac.body                     = body;
    rac.headers                  = {contentType};
    rac.jpeg                     = jpeg;
    rac.method                   = rest_types::Method::POST;
    rac.persistency              = pers;
    rac.retries                  = 3;
    rac.send_individual_requests = false;
    rac.send_without_ocr         = false;
    rac.timeout                  = 30000;  // ms
    rac.tls                      = tls;
    rac.url                      = url;

    return rac;
}

/// Configure the night profile with 2 multi-exposure steps at different
/// flash power levels and continuous trigger.
static rest_types::ProfileConfig buildNightProfile(int profileId) {

    // Multi-exposure step 1: 100 % shutter, flash at 80 % power
    rest_types::Power flash1Power;
    flash1Power.out     = 1;
    flash1Power.percent = 80;

    rest_types::Flash flash1;
    flash1.power = {{flash1Power}};

    rest_types::Shutter shutter1;
    shutter1.percentage_of_current = true;
    shutter1.value                 = 100.0;

    rest_types::SettingGain gain1;
    gain1.percentage_of_current = true;
    gain1.value                 = 100.0;

    rest_types::MultipleExposuresConfig step1;
    step1.flash   = flash1;
    step1.shutter = shutter1;
    step1.gain    = gain1;

    // Multi-exposure step 2: 100 % shutter, flash at 40 % power
    rest_types::Power flash2Power;
    flash2Power.out     = 1;
    flash2Power.percent = 40;

    rest_types::Flash flash2;
    flash2.power = {{flash2Power}};

    rest_types::Shutter shutter2;
    shutter2.percentage_of_current = true;
    shutter2.value                 = 100.0;

    rest_types::SettingGain gain2;
    gain2.percentage_of_current = true;
    gain2.value                 = 100.0;

    rest_types::MultipleExposuresConfig step2;
    step2.flash   = flash2;
    step2.shutter = shutter2;
    step2.gain    = gain2;

    rest_types::MultipleExposures multiExp;
    multiExp.enabled  = true;
    multiExp.settings = {{step1, step2}};

    // Continuous trigger
    rest_types::Trigger trigger;
    trigger.enabled          = true;
    trigger.event            = "constant";
    trigger.minimum_interval = 150;    // ms between triggers

    rest_types::ProfileConfig profile;
    profile.id                 = profileId;
    profile.multiple_exposures = multiExp;
    profile.trigger            = trigger;

    return profile;
}

/// Configure the day profile with single exposure and continuous trigger.
static rest_types::ProfileConfig buildDayProfile(int profileId) {
    // Disable multi-exposure for single-shot capture
    rest_types::MultipleExposures multiExp;
    multiExp.enabled = false;

    // Continuous trigger at 150 ms interval
    rest_types::Trigger trigger;
    trigger.enabled          = true;
    trigger.event            = "constant";
    trigger.minimum_interval = 150;

    rest_types::ProfileConfig profile;
    profile.id                 = profileId;
    profile.multiple_exposures = multiExp;
    profile.trigger            = trigger;

    return profile;
}

/// Build analytics config with majority voting enabled.
static rest_types::AnalyticsConfig buildVotingConfig() {
    rest_types::Voting voting;
    voting.enabled                          = true;
    voting.keep_best_only                   = true;
    voting.max_diff_chars                   = 2;
    voting.same_plate_debounce              = 30;  // seconds
    voting.use_classifier                   = true;
    voting.forward_without_plate_if_tracker = false;

    rest_types::AnalyticsConfig analytics;
    analytics.voting = voting;
    return analytics;
}

static int runFreeflowMode(const Config& cfg) {
    log("");
    log("============================================================");
    log("  PHASE 2 - FREEFLOW MODE (continuous trigger + voting + RAC)");
    log("============================================================");

    uint16_t port = cfg.useHttps ? 443 : 80;
    std::string scheme = cfg.useHttps ? "https" : "http";

    // ---- REST client: configure equipment ---------------------------------

    ItscamRestClient rest;
    rest.setLogHandler([](LogLevel lvl, const std::string& msg) {
        if (lvl == LogLevel::Error)
            std::cerr << "[REST ERR] " << msg << std::endl;
    });
    rest.setBaseUrl(cfg.host, port, scheme);
    if (cfg.useHttps && cfg.insecure)
        rest.setVerifyServerCertificate(false);

    log("\nAuthenticating with REST API...");
    {
        auto loginRes = rest.login(cfg.user, cfg.password);
        if (!loginRes) {
            logErr("REST login failed: " + loginRes.error().message);
            return 1;
        }
        log("Logged in as '" + cfg.user + "'");
    }

    // -- Find both profiles by name --

    log("\nLooking up night profile '" + cfg.profileName + "'...");
    auto profileResult = rest.getProfileByName(cfg.profileName);
    if (!profileResult) {
        logErr("Night profile '" + cfg.profileName
               + "' not found on camera: "
               + profileResult.error().message);
        log("Available profiles:");
        auto all = rest.getProfiles();
        if (all) {
            for (const auto& p : all.value())
                log("  id=" + std::to_string(p.id) + "  name=\""
                    + (p.name ? *p.name : "(unnamed)") + "\"");
        }
        return 1;
    }
    int profileId = static_cast<int>(profileResult.value().id);
    log("  found night profile id=" + std::to_string(profileId));

    log("\nLooking up day profile '" + cfg.dayProfileName + "'...");
    auto dayProfileResult = rest.getProfileByName(cfg.dayProfileName);
    if (!dayProfileResult) {
        logErr("Day profile '" + cfg.dayProfileName
               + "' not found on camera: "
               + dayProfileResult.error().message);
        log("Available profiles:");
        auto all = rest.getProfiles();
        if (all) {
            for (const auto& p : all.value())
                log("  id=" + std::to_string(p.id) + "  name=\""
                    + (p.name ? *p.name : "(unnamed)") + "\"");
        }
        return 1;
    }
    int dayProfileId = static_cast<int>(dayProfileResult.value().id);
    log("  found day profile id=" + std::to_string(dayProfileId));

    // -- Save original configs so we can restore them later --
    // We only save the fields we are going to modify (trigger and
    // multiple_exposures).  Restoring the full profile document would
    // include read-only / computed fields that the PUT endpoint rejects.

    log("\nSaving original configuration...");

    auto origNightTrigger  = profileResult.value().trigger;
    auto origNightMultiExp = profileResult.value().multiple_exposures;
    log("  saved night profile " + std::to_string(profileId)
        + " (trigger + multipleExposures)");

    auto origDayTrigger  = dayProfileResult.value().trigger;
    auto origDayMultiExp = dayProfileResult.value().multiple_exposures;
    log("  saved day profile " + std::to_string(dayProfileId)
        + " (trigger + multipleExposures)");

    auto origAnalytics = rest.getAnalyticsConfig();
    if (!origAnalytics) {
        logErr("Failed to read analytics config: "
               + origAnalytics.error().message);
        return 1;
    }
    log("  saved analytics config");

    auto origRac = rest.getRestApiClientConfig(0);
    bool hadRac = static_cast<bool>(origRac);
    if (hadRac) {
        log("  saved RAC server 0 config");
    } else {
        log("  RAC server 0 not yet configured (will create)");
    }

    // -- Configure both profiles with continuous trigger --

    log("\nConfiguring night profile (2 exposures, different flash power)...");
    {
        auto nightProfile = buildNightProfile(profileId);
        auto result = rest.updateProfileById(profileId, nightProfile);
        if (!result) {
            logErr("Night profile update failed: " + result.error().message);
            return 1;
        }
        log("  night profile " + std::to_string(profileId)
            + " updated:\n" + preview(result.value()));
    }

    log("\nConfiguring day profile (single exposure, constant trigger)...");
    {
        auto dayProfile = buildDayProfile(dayProfileId);
        auto result = rest.updateProfileById(dayProfileId, dayProfile);
        if (!result) {
            logErr("Day profile update failed: " + result.error().message);
            return 1;
        }
        log("  day profile " + std::to_string(dayProfileId)
            + " updated:\n" + preview(result.value()));
    }

    // -- Configure analytics with majority voting --

    log("\nConfiguring analytics (majority voting)...");
    {
        auto votingCfg = buildVotingConfig();
        auto result = rest.setAnalyticsConfig(votingCfg);
        if (!result) {
            logErr("Analytics config failed: " + result.error().message);
            return 1;
        }
        log("  voting enabled:\n" + preview(result.value()));
    }

    // -- Configure RAC server --

    log("\nConfiguring RAC server 0 (HTTP webhook)...");
    {
        auto racCfg = buildRacConfig(cfg);
        auto result = rest.setRestApiClientConfig(0, racCfg);
        if (!result) {
            logErr("RAC config failed: " + result.error().message);
            return 1;
        }
        log("  RAC server 0 enabled:");
        log("    URL: http://" + cfg.racHost + ":"
            + std::to_string(cfg.racPort) + cfg.racPath);
        log("    body: {plate, timestamp, image(base64)}");
    }

    // -- Connect binary client and watch captures for the duration --

    log("\nConnecting binary client to receive trigger events...");

    ItscamClient camera;
    camera.setLogHandler([](LogLevel lvl, const std::string& msg) {
        if (lvl == LogLevel::Error)
            std::cerr << "[SDK ERR] " << msg << std::endl;
    });

    AutoReconnectConfig reconnect;
    reconnect.enabled    = true;
    reconnect.intervalMs = 3000;
    reconnect.maxRetries = 0;

    auto connRes = camera.connect(cfg.host, 60000, 5000, reconnect);
    if (!connRes) {
        logErr("Binary connect failed: " + connRes.error().message);
        return 1;
    }

    if (!cfg.password.empty()) {
        auto authRes = camera.authenticate(cfg.password);
        if (!authRes) {
            logErr("Binary auth failed: " + authRes.error().message);
            return 1;
        }
    }

    CaptureSubscriptionConfig sub;
    sub.embedSignature = true;
    camera.subscribeCaptures(sub);

    std::atomic<int> frameCount{0};
    std::atomic<int> groupCount{0};

    camera.onTriggerImage([&frameCount](const CaptureResult& cr) {
        int n = ++frameCount;
        log("  [trigger] frame #" + std::to_string(n)
            + ", RID=" + std::to_string(cr.info.requestId)
            + ", exp " + std::to_string(cr.info.multiExpIndex + 1)
            + "/" + std::to_string(cr.info.multiExpLength)
            + ", " + std::to_string(cr.jpeg.size()) + " bytes");
    });

    camera.onTriggerExposureGroup(
        [&groupCount](const std::vector<CaptureResult>& group) {
        if (group.empty()) return;
        int n = ++groupCount;
        log("  [group] exposure group #" + std::to_string(n)
            + ", RID=" + std::to_string(group[0].info.requestId)
            + ", " + std::to_string(group.size()) + " frame(s)");
    });

    log("\nFreeflow running for " + std::to_string(cfg.durationSec)
        + " seconds...");
    log("  (Camera is sending captures to RAC at http://"
        + cfg.racHost + ":" + std::to_string(cfg.racPort) + cfg.racPath
        + ")");

    itscam::os::sleepForMs(cfg.durationSec * 1000);

    log("\nFreeflow phase complete.");
    log("  total trigger frames:   " + std::to_string(frameCount.load()));
    log("  total exposure groups:  " + std::to_string(groupCount.load()));

    camera.disconnect();

    // -- Restore original configuration --

    log("\n--- Restoring original configuration ---");

    {
        rest_types::ProfileConfig restore;
        restore.id                 = profileId;
        restore.trigger            = origNightTrigger;
        restore.multiple_exposures = origNightMultiExp;
        auto r = rest.updateProfileById(profileId, restore);
        if (r) log("  night profile " + std::to_string(profileId) + " restored");
        else   logErr("  night profile restore failed: " + r.error().message);
    }

    {
        rest_types::ProfileConfig restore;
        restore.id                 = dayProfileId;
        restore.trigger            = origDayTrigger;
        restore.multiple_exposures = origDayMultiExp;
        auto r = rest.updateProfileById(dayProfileId, restore);
        if (r) log("  day profile " + std::to_string(dayProfileId) + " restored");
        else   logErr("  day profile restore failed: " + r.error().message);
    }

    {
        auto r = rest.setAnalyticsConfig(origAnalytics.value());
        if (r) log("  analytics config restored");
        else   logErr("  analytics restore failed: " + r.error().message);
    }

    if (hadRac) {
        auto r = rest.setRestApiClientConfig(0, origRac.value());
        if (r) log("  RAC server 0 restored");
        else   logErr("  RAC restore failed: " + r.error().message);
    }

    return 0;
}

//=========================================================================
// Main
//=========================================================================

int main(int argc, char* argv[]) {
    Config cfg;
    if (!parseArgs(argc, argv, cfg)) {
        printUsage(argv[0]);
        return 1;
    }

    log("ITSCAM Snapshot-to-Freeflow Swap Example");
    log("Target: " + cfg.host);
    log("");

    // Phase 1: snapshot mode (CGI)
    int rc = runSnapshotMode(cfg);
    if (rc != 0) return rc;

    // Phase 2: freeflow mode (REST + binary)
    rc = runFreeflowMode(cfg);

    log("\nAll done.");
    return rc;
}
