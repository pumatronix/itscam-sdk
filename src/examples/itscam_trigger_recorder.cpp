/*
 *  itscam_trigger_recorder.cpp
 *
 *  ITSCAM Client SDK - Trigger Event Recorder Example
 *
 *  This application connects to an ITSCAM camera and records trigger events
 *  to a specified folder. It runs until Ctrl+C is pressed and handles
 *  reconnections automatically.
 *
 *  Build:  make itscam_trigger_recorder_static
 *  Run:    ./itscam_trigger_recorder <camera_ip> <output_folder> [options]
 */

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>
#if defined(_WIN32)
#include <direct.h>
#endif
#include "itscam_sdk.h"
#include "itscam_jpeg_utils.h"

// ============================================================================
//  Global state
// ============================================================================

static std::atomic<bool> g_running{true};
static std::atomic<uint64_t> g_savedCount{0};

// ============================================================================
//  Signal handler
// ============================================================================

static void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "\nReceived signal " << signum << ", shutting down...\n";
        g_running = false;
    }
}

// ============================================================================
//  Logging helpers
// ============================================================================

static void log(const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::cout << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
              << "." << std::setfill('0') << std::setw(3) << ms.count()
              << " " << msg << std::endl;
}

static void logErr(const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::cerr << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
              << "." << std::setfill('0') << std::setw(3) << ms.count()
              << " [ERROR] " << msg << std::endl;
}

// ============================================================================
//  Filesystem helpers
// ============================================================================

static bool directoryExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

static bool createDirectoryOne(const std::string& path) {
#if defined(_WIN32)
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

/**
 * Recursively create directories for the given path.
 * Handles nested paths like "a/b/c" by creating each component.
 */
static bool createDirectoryRecursive(const std::string& path) {
    if (path.empty()) return true;
    if (directoryExists(path)) return true;

    // Find the parent directory
    size_t pos = path.rfind('/');
    if (pos != std::string::npos && pos > 0) {
        std::string parent = path.substr(0, pos);
        if (!createDirectoryRecursive(parent)) {
            return false;
        }
    }

    // Create this directory
    return createDirectoryOne(path);
}

// ============================================================================
//  Filename formatting
// ============================================================================

/**
 * Format a filename using the provided template.
 *
 * Supported placeholders:
 *   {ip}        - Camera IP address (dots replaced with underscores)
 *   {rid}       - Request ID
 *   {frame}     - Frame count
 *   {exp}       - Multi-exposure index (0-based)
 *   {explen}    - Multi-exposure length
 *   {year}      - Year (4 digits)
 *   {month}     - Month (2 digits)
 *   {day}       - Day (2 digits)
 *   {hour}      - Hour (2 digits)
 *   {min}       - Minute (2 digits)
 *   {sec}       - Second (2 digits)
 *   {msec}      - Millisecond (3 digits)
 *   {datetime}  - Full datetime: YYYYMMDD_HHMMSS_mmm
 *   {date}      - Date only: YYYYMMDD
 *   {time}      - Time only: HHMMSS_mmm
 *   {count}     - Global image counter
 *
 * Note: Datetime fields use camera timestamp when available (from FrameInfo),
 *       falling back to DatetimeMs from JPEG COM section if camera timestamp is zero.
 */

/**
 * Parse DatetimeMs string from JPEG COM section.
 * Format: "2026-02-18 12:44:52.123"
 * Returns true if parsing succeeded.
 */
static bool parseDatetimeMs(const std::string& dtStr,
                            int &year, int &month, int &day,
                            int &hour, int &min, int &sec, int &msec) {
    // Expected format: "YYYY-MM-DD HH:MM:SS.mmm"
    if (dtStr.length() < 23) return false;

    // Parse: "2026-02-18 12:44:52.123"
    //         0123456789...
    char dash1, dash2, space, colon1, colon2, dot;
    int n = std::sscanf(dtStr.c_str(), "%d%c%d%c%d%c%d%c%d%c%d%c%d",
                        &year, &dash1, &month, &dash2, &day, &space,
                        &hour, &colon1, &min, &colon2, &sec, &dot, &msec);

    if (n != 13) return false;
    if (dash1 != '-' || dash2 != '-' || space != ' ' ||
        colon1 != ':' || colon2 != ':' || dot != '.') return false;

    return (year > 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31);
}

static std::string formatFilename(const std::string& format,
                                   const itscam::CaptureResult& cr,
                                   uint64_t count,
                                   const std::string& cameraIp) {
    std::string result = format;

    auto replace = [&result](const std::string& token, const std::string& value) {
        size_t pos = 0;
        while ((pos = result.find(token, pos)) != std::string::npos) {
            result.replace(pos, token.length(), value);
            pos += value.length();
        }
    };

    auto pad = [](int value, int width) {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(width) << value;
        return oss.str();
    };

    // Use camera timestamp from FrameInfo, fall back to DatetimeMs from COM section
    const auto& info = cr.info;
    const auto& ts = info.timestamp;
    int year, month, day, hour, min, sec, msec;
    bool haveTimestamp = false;

    if (ts.year != 0 || ts.month != 0 || ts.day != 0) {
        // Use structured timestamp from FrameInfo
        year  = ts.year;
        month = ts.month;
        day   = ts.day;
        hour  = ts.hour;
        min   = ts.min;
        sec   = ts.sec;
        msec  = ts.msec;
        haveTimestamp = true;
    } else {
        // Try to extract DatetimeMs from JPEG COM section
        auto tags = itscam::parseJpegCommentTags(cr.jpeg);
        auto it = tags.find("DatetimeMs");
        if (it != tags.end() && parseDatetimeMs(it->second, year, month, day, hour, min, sec, msec)) {
            haveTimestamp = true;
        }
    }

    if (!haveTimestamp) {
        // Last resort: use zeros (should not happen with valid camera data)
        year = month = day = hour = min = sec = msec = 0;
    }

    // Build datetime components
    std::ostringstream datetime, date, time_only;
    datetime << pad(year, 4)
             << pad(month, 2)
             << pad(day, 2)
             << "_"
             << pad(hour, 2)
             << pad(min, 2)
             << pad(sec, 2)
             << "_"
             << pad(msec, 3);

    date << pad(year, 4)
         << pad(month, 2)
         << pad(day, 2);

    time_only << pad(hour, 2)
              << pad(min, 2)
              << pad(sec, 2)
              << "_"
              << pad(msec, 3);

    // Replace all placeholders
    replace("{rid}", std::to_string(info.requestId));
    replace("{frame}", std::to_string(info.frameCount));
    replace("{exp}", std::to_string(info.multiExpIndex));
    replace("{explen}", std::to_string(info.multiExpLength));
    replace("{year}", pad(year, 4));
    replace("{month}", pad(month, 2));
    replace("{day}", pad(day, 2));
    replace("{hour}", pad(hour, 2));
    replace("{min}", pad(min, 2));
    replace("{sec}", pad(sec, 2));
    replace("{msec}", pad(msec, 3));
    replace("{datetime}", datetime.str());
    replace("{date}", date.str());
    replace("{time}", time_only.str());
    replace("{count}", std::to_string(count));

    // Camera IP with dots replaced by underscores for filesystem safety
    std::string safeIp = cameraIp;
    for (char& c : safeIp) {
        if (c == '.') c = '_';
    }
    replace("{ip}", safeIp);

    return result;
}

// ============================================================================
//  Image saving
// ============================================================================

static bool saveImage(const std::string& folder,
                      const std::string& filename,
                      const std::vector<uint8_t>& data) {
    std::string path = folder;
    if (!path.empty() && path.back() != '/') {
        path += '/';
    }
    path += filename;

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        logErr("Cannot open file for writing: " + path);
        return false;
    }

    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    out.close();

    if (!out.good()) {
        logErr("Write error on: " + path);
        return false;
    }

    return true;
}

// ============================================================================
//  Connection state helper
// ============================================================================

static const char* connectionStateName(itscam::ConnectionState cs) {
    switch (cs) {
        case itscam::ConnectionState::Connected:    return "Connected";
        case itscam::ConnectionState::Disconnected: return "Disconnected";
        case itscam::ConnectionState::Reconnecting: return "Reconnecting";
        case itscam::ConnectionState::Reconnected:  return "Reconnected";
        default:                                    return "Unknown";
    }
}

// ============================================================================
//  Usage / help
// ============================================================================

static void printUsage(const char* progName) {
    std::cerr << "ITSCAM Trigger Recorder - Save trigger events to disk\n\n";
    std::cerr << "Usage: " << progName << " <camera_ip> <output_folder> [options]\n\n";
    std::cerr << "Required arguments:\n";
    std::cerr << "  camera_ip       IP address of the ITSCAM camera\n";
    std::cerr << "  output_folder   Directory to save trigger images\n\n";
    std::cerr << "Options:\n";
    std::cerr << "  -p, --password <pass>     Authentication password\n";
    std::cerr << "  -f, --format <fmt>        Filename format (default: {ip}_trigger_{datetime}_{exp}.jpg)\n";
    std::cerr << "  -q, --quality <1-100>     JPEG quality (default: camera setting)\n";
    std::cerr << "  -r, --reconnect-interval <ms>  Reconnect interval in ms (default: 3000)\n";
    std::cerr << "  -h, --help                Show this help message\n\n";
    std::cerr << "Filename format placeholders:\n";
    std::cerr << "  {ip}       Camera IP (dots replaced with underscores)\n";
    std::cerr << "  {rid}      Request ID\n";
    std::cerr << "  {frame}    Frame count\n";
    std::cerr << "  {exp}      Multi-exposure index (0-based)\n";
    std::cerr << "  {explen}   Multi-exposure length\n";
    std::cerr << "  {datetime} Full datetime: YYYYMMDD_HHMMSS_mmm\n";
    std::cerr << "  {date}     Date only: YYYYMMDD\n";
    std::cerr << "  {time}     Time only: HHMMSS_mmm\n";
    std::cerr << "  {year}, {month}, {day}, {hour}, {min}, {sec}, {msec}\n";
    std::cerr << "  {count}    Global image counter\n\n";
    std::cerr << "Examples:\n";
    std::cerr << "  " << progName << " 192.168.254.254 /tmp/triggers\n";
    std::cerr << "  " << progName << " 192.168.254.254 ./images -p 1234\n";
    std::cerr << "  " << progName << " 192.168.254.254 ./images -f \"cam1_{datetime}_{exp}.jpg\"\n";
    std::cerr << "  " << progName << " 192.168.254.254 ./images -f \"{date}/{rid}_{exp}.jpg\"\n";
}

// ============================================================================
//  Command-line argument parsing
// ============================================================================

struct Config {
    std::string cameraIp;
    std::string outputFolder;
    std::string password;
    std::string filenameFormat = "{ip}_trigger_{datetime}_{exp}.jpg";
    int jpegQuality = -1;  // -1 = use camera default
    uint32_t reconnectIntervalMs = 3000;
};

static bool parseArgs(int argc, char* argv[], Config& cfg) {
    if (argc < 3) {
        return false;
    }

    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 ||
            std::strcmp(argv[i], "--help") == 0) {
            return false;
        }
    }

    cfg.cameraIp = argv[1];
    cfg.outputFolder = argv[2];

    // Parse optional arguments
    for (int i = 3; i < argc; ++i) {
        if ((std::strcmp(argv[i], "-p") == 0 ||
             std::strcmp(argv[i], "--password") == 0) && i + 1 < argc) {
            cfg.password = argv[++i];
        } else if ((std::strcmp(argv[i], "-f") == 0 ||
                    std::strcmp(argv[i], "--format") == 0) && i + 1 < argc) {
            cfg.filenameFormat = argv[++i];
        } else if ((std::strcmp(argv[i], "-q") == 0 ||
                    std::strcmp(argv[i], "--quality") == 0) && i + 1 < argc) {
            cfg.jpegQuality = std::atoi(argv[++i]);
            if (cfg.jpegQuality < 1 || cfg.jpegQuality > 100) {
                std::cerr << "Error: JPEG quality must be between 1 and 100\n";
                return false;
            }
        } else if ((std::strcmp(argv[i], "-r") == 0 ||
                    std::strcmp(argv[i], "--reconnect-interval") == 0) &&
                   i + 1 < argc) {
            cfg.reconnectIntervalMs = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            return false;
        }
    }

    return true;
}

// ============================================================================
//  Main
// ============================================================================

int main(int argc, char* argv[]) {
    using namespace itscam;

    Config cfg;
    if (!parseArgs(argc, argv, cfg)) {
        printUsage(argv[0]);
        return (argc > 1 && (std::strcmp(argv[1], "-h") == 0 ||
                            std::strcmp(argv[1], "--help") == 0)) ? 0 : 1;
    }

    // --- Setup signal handlers ----------------------------------------------
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // --- Validate / create output directory ---------------------------------
    if (!directoryExists(cfg.outputFolder)) {
        log("Creating output directory: " + cfg.outputFolder);
        if (!createDirectoryRecursive(cfg.outputFolder)) {
            logErr("Failed to create output directory: " + cfg.outputFolder);
            return 1;
        }
    }

    log("ITSCAM Trigger Recorder");
    log("Camera: " + cfg.cameraIp);
    log("Output folder: " + cfg.outputFolder);
    log("Filename format: " + cfg.filenameFormat);
    log("Press Ctrl+C to stop recording");

    // --- SDK setup ----------------------------------------------------------
    ItscamClient camera;

    camera.setLogHandler([](LogLevel lvl, const std::string& msg) {
        if (lvl == LogLevel::Error)
            logErr("[SDK] " + msg);
    });

    // --- Connection state callback ------------------------------------------
    std::atomic<bool> connected{false};

    camera.onConnectionStateChanged([&connected](ConnectionState cs,
                                                  const std::string& reason) {
        log("[Connection] " + std::string(connectionStateName(cs)) +
            ": " + reason);
        connected = (cs == ConnectionState::Connected ||
                     cs == ConnectionState::Reconnected);
    });

    // --- Connect with auto-reconnection -------------------------------------
    AutoReconnectConfig reconnect;
    reconnect.enabled    = true;
    reconnect.intervalMs = cfg.reconnectIntervalMs;
    reconnect.maxRetries = 0;  // unlimited retries

    log("Connecting to camera...");
    auto connResult = camera.connect(cfg.cameraIp, 60000, 5000, reconnect);
    if (!connResult) {
        logErr("Initial connection failed: " + connResult.error().message);
        log("Will keep trying to reconnect...");
    } else {
        connected = true;
        log("Connected successfully");
    }

    // --- Authenticate if password provided ----------------------------------
    if (connected && !cfg.password.empty()) {
        log("Authenticating...");
        auto authResult = camera.authenticate(cfg.password);
        if (!authResult) {
            logErr("Authentication failed: " + authResult.error().message);
            return 1;
        }
        log("Authenticated");
    }

    // --- Subscribe to trigger events ----------------------------------------
    CaptureSubscriptionConfig captureEvents;
    captureEvents.includeSnapshot = false;
    captureEvents.embedSignature = true;
    captureEvents.triggerQuality = cfg.jpegQuality;
    if (cfg.jpegQuality > 0) {
        log("JPEG quality set to " + std::to_string(cfg.jpegQuality));
    }
    camera.subscribeCaptures(captureEvents);
    log("JPEG metadata embedding enabled (EXIF, comments, signature)");

    // --- Register trigger image callback ------------------------------------
    camera.onTriggerImage([&cfg](const CaptureResult& cr) {
        uint64_t count = ++g_savedCount;

        std::string filename = formatFilename(cfg.filenameFormat, cr, count, cfg.cameraIp);

        // Check if filename contains subdirectories
        std::string fullPath = cfg.outputFolder;
        if (!fullPath.empty() && fullPath.back() != '/') {
            fullPath += '/';
        }

        size_t lastSlash = filename.rfind('/');
        if (lastSlash != std::string::npos) {
            // Filename contains subdirectories - ensure they exist
            std::string subdir = fullPath + filename.substr(0, lastSlash);
            if (!directoryExists(subdir)) {
                createDirectoryRecursive(subdir);
            }
        }

        if (saveImage(cfg.outputFolder, filename, cr.jpeg)) {
            log("Saved [" + std::to_string(count) + "]: " + filename +
                " (" + std::to_string(cr.jpeg.size()) + " bytes, RID=" +
                std::to_string(cr.info.requestId) +
                ", exp " + std::to_string(cr.info.multiExpIndex + 1) +
                "/" + std::to_string(cr.info.multiExpLength) + ")");
        }
    });

    // --- Main loop: wait for Ctrl+C -----------------------------------------
    log("Recording started. Waiting for trigger events...");

    while (g_running) {
        itscam_os::sleepForMs(100);
    }

    // --- Cleanup ------------------------------------------------------------
    log("Disconnecting from camera...");
    camera.disconnect();

    log("Recording stopped. Total images saved: " +
        std::to_string(g_savedCount.load()));

    return 0;
}
