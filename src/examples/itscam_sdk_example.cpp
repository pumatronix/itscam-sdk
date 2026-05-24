/*
 *  itscam_sdk_example.cpp
 *
 *  ITSCAM Client SDK usage example.
 *
 *  Demonstrates: connect with auto-reconnection, authenticate, capture
 *  snapshots (single and multi-exposure), configure trigger to continuous
 *  mode, receive trigger images and exposure groups via callbacks,
 *  read/modify exposure settings, profile management, and connection
 *  state monitoring.
 *
 *  Build:  make
 *  Run:    ./itscam_sdk_example <camera_ip> [password]
 */

#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "itscam_sdk.h"

//=========================================================================
// Helpers
//=========================================================================

static void log(const std::string& msg) {
    std::cout << msg << std::endl;
}

static void logErr(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

static int saveJpeg(const std::string& prefix, const std::string& useCase,
                    uint64_t rid, int expIdx,
                    const std::vector<uint8_t>& data) {
    std::string path = prefix + "_" + useCase +
                       "_" + std::to_string(rid) +
                       "_" + std::to_string(expIdx) + ".jpg";
    std::ofstream out(path, std::ios::binary);
    if (!out) { logErr("Cannot open " + path); return 1; }
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    out.close();
    if (!out.good()) { logErr("Write error on " + path); return 1; }
    log("Saved: " + path);
    return 0;
}

static const char* connectionStateName(itscam::ConnectionState cs) {
    switch (cs) {
        case itscam::ConnectionState::Connected:    return "Connected";
        case itscam::ConnectionState::Disconnected:  return "Disconnected";
        case itscam::ConnectionState::Reconnecting:  return "Reconnecting";
        case itscam::ConnectionState::Reconnected:   return "Reconnected";
        default:                                     return "Unknown";
    }
}

static const char* triggerEventName(itscam::TriggerEvent ev) {
    switch (ev) {
        case itscam::TriggerEvent::Constant:    return "constant";
        case itscam::TriggerEvent::EdgeRising:  return "edge_rising";
        case itscam::TriggerEvent::EdgeFalling: return "edge_falling";
        case itscam::TriggerEvent::EdgeBoth:    return "edge_both";
        case itscam::TriggerEvent::LevelHigh:   return "level_high";
        case itscam::TriggerEvent::LevelLow:    return "level_low";
        case itscam::TriggerEvent::Motion:      return "motion";
        default:                                return "unchanged";
    }
}

static void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <camera_ip> [password]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "  camera_ip   IP address of the ITSCAM camera (required)" << std::endl;
    std::cerr << "  password    Authentication password (optional, default: none)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " 192.168.254.254" << std::endl;
    std::cerr << "  " << progName << " 192.168.254.254 1234" << std::endl;
}

//=========================================================================
// Main
//=========================================================================

int main(int argc, char* argv[]) {
    using namespace itscam;

    // --- Argument parsing ---------------------------------------------------

    if (argc < 2 || std::strcmp(argv[1], "--help") == 0 ||
        std::strcmp(argv[1], "-h") == 0) {
        printUsage(argv[0]);
        return (argc < 2) ? 1 : 0;
    }

    const std::string cameraIp = argv[1];
    const std::string password = (argc >= 3) ? argv[2] : "";

    log("ITSCAM Client SDK Example");
    log("Camera: " + cameraIp);

    // --- SDK setup ----------------------------------------------------------

    ItscamClient camera;

    camera.setLogHandler([](LogLevel lvl, const std::string& msg) {
        if (lvl == LogLevel::Error)
            std::cerr << "[SDK ERROR] " << msg << std::endl;
        else
            std::cout << "[SDK] " << msg << std::endl;
    });

    // --- Connection state callback ------------------------------------------

    camera.onConnectionStateChanged([](ConnectionState cs, const std::string& reason) {
        log("[ConnState] " + std::string(connectionStateName(cs)) +
            ": " + reason);
    });

    // --- Connect with auto-reconnection ------------------------------------

    AutoReconnectConfig reconnect;
    reconnect.enabled    = true;
    reconnect.intervalMs = 3000;   // retry every 3 seconds
    reconnect.maxRetries = 0;      // unlimited retries

    log("Connecting...");
    auto connResult = camera.connect(cameraIp, 60000, 5000, reconnect);
    if (!connResult) {
        logErr("Could not connect: " + connResult.error().message);
        return 1;
    }
    log("Connected");

    // --- Authenticate (if password provided) --------------------------------

    if (!password.empty()) {
        log("Authenticating...");
        auto authResult = camera.authenticate(password);
        if (!authResult) {
            logErr("Authentication failed: " + authResult.error().message);
            return 1;
        }
        log("Authenticated");
    }

    // --- Subscribe to capture events ----------------------------------------

    CaptureSubscriptionConfig captureEvents;
    captureEvents.embedSignature = true;
    camera.subscribeCaptures(captureEvents);

    // --- Register callback for trigger images -------------------------------

    std::atomic<int> triggerImageCount{0};
    camera.onTriggerImage([&triggerImageCount, &cameraIp](const CaptureResult& cr) {
        ++triggerImageCount;
        log("Received TRIGGER frame, RID=" +
            std::to_string(cr.info.requestId) +
            ", multiExp " + std::to_string(cr.info.multiExpIndex + 1) +
            "/" + std::to_string(cr.info.multiExpLength) +
            ", size=" + std::to_string(cr.jpeg.size()) + " bytes");
        saveJpeg(cameraIp, "trigger", cr.info.requestId,
                 cr.info.multiExpIndex, cr.jpeg);
    });

    // Register exposure group callback (fires once per complete group)
    std::atomic<int> triggerGroupCount{0};
    camera.onTriggerExposureGroup(
        [&triggerGroupCount, &cameraIp](const std::vector<CaptureResult>& group) {
        if (group.empty()) return;
        int n = ++triggerGroupCount;
        log("Received TRIGGER exposure group #" +
            std::to_string(n) + ", RID=" +
            std::to_string(group[0].info.requestId) +
            ", " + std::to_string(group.size()) + "/" +
            std::to_string(group[0].info.multiExpLength) + " frames");
        for (size_t i = 0; i < group.size(); ++i) {
            saveJpeg(cameraIp, "trigger_group", group[i].info.requestId,
                     group[i].info.multiExpIndex, group[i].jpeg);
        }
    });

    //=========================================================================
    // 1. Snapshot capture
    //=========================================================================

    log("");
    log("=== Snapshot Capture ===");

    {
        SnapshotRequest req;
        req.overlays["TextOverlay"] = "ITSCAM SDK Example";
        auto result = camera.captureSnapshot(req);
        if (result) {
            auto& frames = result.value();
            log("Snapshot OK, " + std::to_string(frames.size()) + " frame(s)");
            for (size_t i = 0; i < frames.size(); ++i) {
                log("  Frame " + std::to_string(i) + ": RID=" +
                    std::to_string(frames[i].info.requestId) +
                    ", " + std::to_string(frames[i].jpeg.size()) + " bytes" +
                    ", multiExp " + std::to_string(frames[i].info.multiExpIndex + 1) +
                    "/" + std::to_string(frames[i].info.multiExpLength));
                saveJpeg(cameraIp, "snapshot", frames[i].info.requestId,
                         frames[i].info.multiExpIndex, frames[i].jpeg);
            }
        } else {
            logErr("Snapshot failed: " + result.error().message);
        }
    }

    //=========================================================================
    // 2. Scenario-based snapshots
    //=========================================================================

    log("");
    log("=== Scenario-based Snapshots ===");

    {
        camera.setScenarioOverlay(1, "Scenario 1 - Speed: {User_Speed} km/h");
        camera.setScenarioOverlay(2, "Scenario 2 - Speed: {User_Speed} km/h");
        camera.setScenarioCrop(1, {0, 0, 799, 599});
        camera.setScenarioCrop(2, {400, 400, 1199, 999});

        SnapshotRequest req1;
        req1.overlays["Cenario"]    = "1";
        req1.overlays["User_Speed"] = "60";
        auto r1 = camera.captureSnapshot(req1);
        if (r1) {
            log("Scenario 1 captured (" +
                std::to_string(r1.value().size()) + " frame(s))");
            for (auto& f : r1.value())
                saveJpeg(cameraIp, "scenario1", f.info.requestId,
                         f.info.multiExpIndex, f.jpeg);
        } else {
            logErr("Scenario 1 failed: " + r1.error().message);
        }

        SnapshotRequest req2;
        req2.overlays["Cenario"]    = "2";
        req2.overlays["User_Speed"] = "120";
        auto r2 = camera.captureSnapshot(req2);
        if (r2) {
            log("Scenario 2 captured (" +
                std::to_string(r2.value().size()) + " frame(s))");
            for (auto& f : r2.value())
                saveJpeg(cameraIp, "scenario2", f.info.requestId,
                         f.info.multiExpIndex, f.jpeg);
        } else {
            logErr("Scenario 2 failed: " + r2.error().message);
        }
    }

    //=========================================================================
    // 3. Trigger test -- set to continuous mode and wait for images
    //=========================================================================

    log("");
    log("=== Trigger Test (continuous mode + multi-exposure) ===");

    {
        // Read current trigger config so we can restore it later
        auto oldTrig = camera.getTriggerConfig();
        if (oldTrig) {
            log("Current trigger config:");
            log("  enabled: " + std::to_string(oldTrig.value().enabled));
            log("  event:   " + std::string(triggerEventName(oldTrig.value().event)));
            log("  port:    " + std::to_string(oldTrig.value().port));
        }

        // Configure multi-exposure: 2 exposure steps with different
        // shutter speeds (percentage of current auto-exposure value).
        // The camera will produce 2 frames per trigger event.
        MultiExposureConfig multiExp;
        multiExp.enabled = true;

        MultiExpStep step1;
        step1.shutterPercent = 100;  // 100% of auto (normal)
        step1.gainPercent    = 100;

        MultiExpStep step2;
        step2.shutterPercent = 50;   // 50% of auto (darker)
        step2.gainPercent    = 100;

        multiExp.steps = {step1, step2};

        auto meResult = camera.setMultiExposureConfig(multiExp);
        if (meResult)
            log("Multi-exposure configured: 2 steps");
        else
            logErr("Failed to configure multi-exposure: " +
                   meResult.error().message);

        // Set trigger to continuous mode (fires at every frame period)
        TriggerConfig contTrig;
        contTrig.enabled = 1;
        contTrig.event   = TriggerEvent::Constant;
        contTrig.minimumIntervalMs = 2000; // limit rate for multi-exposure
        auto setResult = camera.setTriggerConfig(contTrig);
        if (setResult) {
            log("Trigger set to continuous mode");
        } else {
            logErr("Failed to set trigger: " + setResult.error().message);
        }

        log("Waiting 15s for trigger images (multi-exposure groups)...");
        itscam_os::sleepForMs(15000);

        log("Received " + std::to_string(triggerImageCount.load()) +
            " individual trigger frame(s), " +
            std::to_string(triggerGroupCount.load()) + " exposure group(s)");

        // Disable multi-exposure
        MultiExposureConfig disableMultiExp;
        disableMultiExp.enabled = false;
        disableMultiExp.steps = {MultiExpStep()};  // single default step
        camera.setMultiExposureConfig(disableMultiExp);
        log("Multi-exposure disabled");

        // Restore previous trigger config if it was read successfully
        if (oldTrig) {
            log("Restoring original trigger config...");
            camera.setTriggerConfig(oldTrig.value());
        }
    }

    //=========================================================================
    // 4. Exposure configuration
    //=========================================================================

    log("");
    log("=== Exposure Configuration ===");

    {
        auto exp = camera.getExposureConfig();
        if (exp) {
            log("Current exposure config:");
            log("  shutter.automatic:  " + std::to_string(exp.value().shutter.automatic));
            log("  shutter.fixedValue: " + std::to_string(exp.value().shutter.fixedValue));
            log("  shutter.maxValue:   " + std::to_string(exp.value().shutter.maxValue));
            log("  gain.automatic:     " + std::to_string(exp.value().gain.automatic));
            log("  gain.maxValue:      " + std::to_string(exp.value().gain.maxValue));
        } else {
            logErr("Failed to read exposure: " + exp.error().message);
        }
    }

    //=========================================================================
    // 5. Device info
    //=========================================================================

    log("");
    log("=== Device Info ===");

    {
        auto info = camera.getDeviceInfo();
        if (info) {
            log("  Model:      " + info.value().cameraModel);
            log("  Firmware:   " + info.value().firmwareVersion);
            log("  Sensor:     " + info.value().sensorType);
            log("  Lens:       " + info.value().lensModel);
            log("  Resolution: " + std::to_string(info.value().imageSize.width) +
                "x" + std::to_string(info.value().imageSize.height));
            log("  HDR:        " + std::string(info.value().hdrAvailable ? "yes" : "no"));
        } else {
            logErr("Failed to read device info: " + info.error().message);
        }
    }

    //=========================================================================
    // 6. Profile management
    //=========================================================================

    log("");
    log("=== Profile Management ===");

    {
        // List all profiles
        auto profiles = camera.listProfiles();
        if (profiles) {
            log("Available profiles:");
            for (auto& p : profiles.value()) {
                std::string line = "  id=" + std::to_string(p.id) +
                    "  name=\"" + p.name + "\"" +
                    (p.active ? "  [ACTIVE]" : "");
                log(line);
            }
        } else {
            logErr("Failed to list profiles: " + profiles.error().message);
        }

        // Read trigger config for the active profile (default)
        auto trigCurrent = camera.getTriggerConfig();
        if (trigCurrent) {
            log("Active profile trigger event: " +
                std::string(triggerEventName(trigCurrent.value().event)));
        }

        // Read trigger config for a specific profile by ID
        auto profileId = camera.getActiveProfileId();
        if (profileId) {
            log("Active profile ID: " + std::to_string(profileId.value()));

            // Read trigger config explicitly targeting profile 0
            auto trig0 = camera.getTriggerConfig(0);
            if (trig0) {
                log("Profile 0 trigger event: " +
                    std::string(triggerEventName(trig0.value().event)));
            }

            // Read exposure config for profile 0
            auto exp0 = camera.getExposureConfig(0);
            if (exp0) {
                log("Profile 0 shutter.automatic: " +
                    std::to_string(exp0.value().shutter.automatic));
            }
        }
    }

    log("");
    log("Done.");
    return 0;
}
