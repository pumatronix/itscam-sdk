/*
 *  itscam_types.h
 *
 *  ITSCAM Client SDK - Type definitions
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Self-contained header (C++14). Uses OS abstraction layer for threading.
 */
#pragma once

#include "itscam_os.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace itscam {

// =============================================================================
//  Error
// =============================================================================

struct Error {
    enum Code {
        ConnectionFailed,
        Timeout,
        NotAuthenticated,
        InvalidParameter,
        ServerError,
        Disconnected,
        Unknown
    };

    Code code = Unknown;
    std::string message;

    Error() = default;
    Error(Code c, const std::string& msg) : code(c), message(msg) {}
};

// =============================================================================
//  Result<T>
// =============================================================================

template<typename T>
class Result {
public:
    // Default constructor (unset state for SharedState initialization)
    Result() : mHasValue(false) {}

    // Success constructors
    Result(const T& val) : mHasValue(true), mValue(val) {}
    Result(T&& val)      : mHasValue(true), mValue(std::move(val)) {}

    // Failure constructors
    Result(const Error& err) : mHasValue(false), mError(err) {}
    Result(Error&& err)      : mHasValue(false), mError(std::move(err)) {}

    explicit operator bool() const { return mHasValue; }

    const T& value() const & { return mValue; }
    T&       value() &       { return mValue; }
    T&&      value() &&      { return std::move(mValue); }

    const Error& error() const { return mError; }

private:
    bool  mHasValue = false;
    T     mValue{};
    Error mError;
};

// Specialisation for void results (success / fail with no payload)
template<>
class Result<void> {
public:
    // Success
    Result() : mHasValue(true) {}
    static Result success() { return Result(); }

    // Failure
    Result(const Error& err) : mHasValue(false), mError(err) {}
    Result(Error&& err)      : mHasValue(false), mError(std::move(err)) {}

    explicit operator bool() const { return mHasValue; }

    const Error& error() const { return mError; }

private:
    bool  mHasValue = false;
    Error mError;
};

// =============================================================================
//  Future<T> - Cross-platform async result using OS abstraction layer
// =============================================================================

template<typename T>
class Promise;

template<typename T>
class Future {
public:
    Future() : m_state(nullptr) {}

    // Move-only
    Future(Future&&) noexcept = default;
    Future& operator=(Future&&) noexcept = default;
    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    /// Block until the result is available.
    Result<T> get() {
        if (!m_state) return Result<T>(Error{Error::Unknown, "invalid future"});
        itscam_os::UniqueLock<itscam_os::Mutex> lk(m_state->mtx);
        while (!m_state->ready) {
            m_state->cv.wait(lk);
        }
        return std::move(m_state->value);
    }

    /// Block up to @p waitMs milliseconds. Returns Error::Timeout on expiry.
    Result<T> get(uint32_t waitMs) {
        if (!m_state) return Result<T>(Error{Error::Unknown, "invalid future"});
        itscam_os::UniqueLock<itscam_os::Mutex> lk(m_state->mtx);
        if (!m_state->ready) {
            if (m_state->cv.waitFor(lk, waitMs) == itscam_os::ConditionVariable::CvStatus::timeout) {
                if (!m_state->ready) {
                    return Result<T>(Error{Error::Timeout, "future timed out"});
                }
            }
        }
        return std::move(m_state->value);
    }

    /// Non-blocking readiness check.
    bool isReady() const {
        if (!m_state) return false;
        itscam_os::LockGuard<itscam_os::Mutex> lk(m_state->mtx);
        return m_state->ready;
    }

    bool valid() const { return m_state != nullptr; }

private:
    friend class Promise<T>;

    struct SharedState {
        mutable itscam_os::Mutex mtx;
        itscam_os::ConditionVariable cv;
        Result<T> value;
        bool ready = false;
    };

    std::shared_ptr<SharedState> m_state;

    explicit Future(std::shared_ptr<SharedState> state) : m_state(std::move(state)) {}
};

template<typename T>
class Promise {
public:
    Promise() : m_state(std::make_shared<typename Future<T>::SharedState>()) {}

    // Move-only
    Promise(Promise&&) noexcept = default;
    Promise& operator=(Promise&&) noexcept = default;
    Promise(const Promise&) = delete;
    Promise& operator=(const Promise&) = delete;

    Future<T> get_future() {
        return Future<T>(m_state);
    }

    void set_value(Result<T> value) {
        if (!m_state) return;
        {
            itscam_os::LockGuard<itscam_os::Mutex> lk(m_state->mtx);
            m_state->value = std::move(value);
            m_state->ready = true;
        }
        m_state->cv.notifyAll();
    }

private:
    std::shared_ptr<typename Future<T>::SharedState> m_state;
};

// =============================================================================
//  waitAll  --  block until every Future resolves; true iff all succeed.
// =============================================================================

namespace detail {
inline bool waitAllImpl() { return true; }

template<typename T, typename... Rest>
bool waitAllImpl(Future<T>& first, Future<Rest>&... rest) {
    auto r = first.get();
    bool tailOk = waitAllImpl(rest...);
    return static_cast<bool>(r) && tailOk;
}
} // namespace detail

template<typename T, typename... Rest>
bool waitAll(Future<T>& first, Future<Rest>&... rest) {
    return detail::waitAllImpl(first, rest...);
}

// =============================================================================
//  Geometry helpers
// =============================================================================

struct Size  { int width = 0, height = 0; };
struct Point { int x = 0, y = 0; };

// =============================================================================
//  Timestamp / timing
// =============================================================================

struct Timestamp {
    int year = 0, month = 0, day = 0;
    int hour = 0, min = 0, sec = 0, msec = 0;
};

struct MonotonicTiming {
    uint64_t now = 0;
    uint64_t dmaTransfer = 0;
    uint64_t exposureSetup = 0;
    uint64_t exposureEnd = 0;
};

// =============================================================================
//  Image statistics
// =============================================================================

struct ImageStats {
    float level = 0;
    float meanR = 0, meanG = 0, meanB = 0;
    float stdDev = 0;
};

// =============================================================================
//  GPIO
// =============================================================================

struct GpioState {
    bool input[4]  = {};
    bool output[4] = {};
};

struct GpioEvent {
    uint64_t frameCount = 0;
    uint32_t risingEdge = 0;
    uint32_t fallingEdge = 0;
    uint32_t state = 0;
};

// =============================================================================
//  Recognition / detection results (sent with trigger/snapshot events)
// =============================================================================

struct PlateRecognition {
    std::string plate;
    std::vector<float> charProbabilities;
    int x = 0, y = 0, width = 0, height = 0;
    std::string color;
    bool isMotorcycle = false;
    int countryCode = 0;
};

struct ObjectDetection {
    int type = 0;
    float probability = 0;
    int x = 0, y = 0, width = 0, height = 0;
    std::string brand, model, color;
    float brandProb = 0, modelProb = 0, colorProb = 0;
    int trackerSpeed = 0;
    std::string lane;
};

// =============================================================================
//  FrameInfo  --  metadata that accompanies every event frame
// =============================================================================

struct FrameInfo {
    uint64_t requestId = 0;
    uint64_t frameCount = 0;
    Size originalSize;
    Size size;
    Point cropOffset;
    int shutter = 0;
    float gain = 0;
    int multiExpLength = 0;
    int multiExpIndex = 0;
    Timestamp timestamp;
    MonotonicTiming timing;
    ImageStats stats;
    GpioState gpio;
    std::vector<PlateRecognition> plates;
    std::vector<ObjectDetection> objects;
};

// =============================================================================
//  CaptureResult  --  frame + image data (returned by captureSnapshot, etc.)
// =============================================================================

struct CaptureResult {
    FrameInfo info;
    std::vector<uint8_t> jpeg;
    int quality = 0;
    std::map<std::string, std::string> tags;   // EXIF / comments / sign fields
};

// =============================================================================
//  Serial port
// =============================================================================

enum class SerialPort { Serial1, Serial2 };

enum class Parity { None = 0, Odd = 1, Even = 2 };

struct SerialConfig {
    int baudRate = -1;    // 300–250000, -1 = unchanged
    int dataBits = -1;    // 5–8, -1 = unchanged
    int parity   = -1;    // 0=None, 1=Odd, 2=Even, -1 = unchanged
    int stopBits = -1;    // 1 or 2, -1 = unchanged
};

struct SerialData {
    SerialPort port = SerialPort::Serial1;
    std::vector<uint8_t> data;
    std::string datetimeUtc;
    uint64_t uptimeMs = 0;
};

// =============================================================================
//  Event subscription
// =============================================================================

struct EventSubscription {
    bool pipeline         = false;
    bool triggerMetadata  = false;
    bool snapshotMetadata = false;
    bool previewMetadata  = false;
    bool gpio         = false;
    bool triggerImage  = false;
    bool snapshotImage = false;
    bool previewImage  = false;
    bool serial1      = false;
    bool serial2      = false;

    static EventSubscription captureResults(bool includeMetadata = true,
                                            bool includeTrigger = true,
                                            bool includeSnapshot = true) {
        EventSubscription ev;
        ev.triggerImage      = includeTrigger;
        ev.snapshotImage     = includeSnapshot;
        ev.triggerMetadata   = includeMetadata && includeTrigger;
        ev.snapshotMetadata  = includeMetadata && includeSnapshot;
        return ev;
    }
};

// =============================================================================
//  JPEG / IMGPKG configuration
// =============================================================================

struct JpegConfig {
    int triggerQuality      = -1;   // 5–100, -1 = unchanged
    int snapshotQuality     = -1;
    int previewQuality      = -1;
    int previewMinIntervalMs = -1;  // 0–10000, -1 = unchanged

    struct ImgpkgOptions {
        int embedExif       = -1;   // 0 = off, 1 = on, -1 = unchanged
        int embedComments   = -1;
        int embedSignature  = -1;
    } imgpkg;

    static JpegConfig imgpkgDefaults(bool embedSignature = false) {
        JpegConfig cfg;
        cfg.imgpkg.embedExif      = 1;
        cfg.imgpkg.embedComments  = 1;
        cfg.imgpkg.embedSignature = embedSignature ? 1 : 0;
        return cfg;
    }

    bool usesImgpkg() const {
        return imgpkg.embedExif > 0 || imgpkg.embedComments > 0 ||
               imgpkg.embedSignature > 0;
    }
};

struct CaptureSubscriptionConfig {
    bool includeTrigger  = true;
    bool includeSnapshot = true;
    bool includeMetadata = true;
    bool embedComments   = true;
    bool embedExif       = true;
    bool embedSignature  = false;
    int triggerQuality   = -1;
    int snapshotQuality  = -1;

    EventSubscription toEventSubscription() const {
        return EventSubscription::captureResults(includeMetadata,
                                                 includeTrigger,
                                                 includeSnapshot);
    }

    JpegConfig toJpegConfig() const {
        JpegConfig cfg = JpegConfig::imgpkgDefaults(embedSignature);
        cfg.triggerQuality = triggerQuality;
        cfg.snapshotQuality = snapshotQuality;
        cfg.imgpkg.embedExif = embedExif ? 1 : 0;
        cfg.imgpkg.embedComments = embedComments ? 1 : 0;
        return cfg;
    }
};

// =============================================================================
//  Snapshot request  (used by captureSnapshot / captureSnapshotAsync)
// =============================================================================

struct ExposureParams {
    enum Mode { Unchanged = 0, Absolute, PercentOfCurrent };
    Mode  mode  = Unchanged;
    float value = 0;
};

struct FlashParams {
    int channel      = 0;   // 1–8
    int powerPercent = 0;   // 0–100
};

struct ExposureStep {
    ExposureParams shutter;
    ExposureParams gain;
    std::vector<FlashParams> flash;
};

struct SnapshotRequest {
    std::vector<ExposureStep> multiExposure;         // empty = camera defaults
    std::map<std::string, std::string> overlays;     // string map
};

// =============================================================================
//  Multi-exposure profile configuration
// =============================================================================

/// Flash output channel configuration for a multi-exposure step.
struct MultiExpFlashOutput {
    int gpioOutput   = 1;   // 1-based GPIO output index (1–8)
    int powerPercent = 0;   // 0–100
};

/// A single exposure step in a multi-exposure profile configuration.
/// Each field uses percentage-of-current-auto by default.
struct MultiExpStep {
    float shutterPercent = 100;   // percentage of current auto shutter
    float gainPercent    = 100;   // percentage of current auto gain
    std::vector<MultiExpFlashOutput> flash;  // optional flash outputs
};

/// Multi-exposure configuration for a profile.
struct MultiExposureConfig {
    bool enabled = false;
    std::vector<MultiExpStep> steps;  // 1–8 exposure steps
};

// =============================================================================
//  Profile management
// =============================================================================

/// Sentinel value meaning "operate on the currently active profile".
static constexpr uint32_t CURRENT_PROFILE = 0xFFFFFFFF;

struct ProfileInfo {
    uint32_t    id     = 0;
    std::string name;
    std::string description;
    bool        active = false;
};

// =============================================================================
//  Trigger event type
// =============================================================================

enum class TriggerEvent {
    Unchanged,      // do not change current setting
    Constant,       // continuous (fires every frame)
    EdgeRising,     // rising edge on IO port
    EdgeFalling,    // falling edge on IO port
    EdgeBoth,       // both edges on IO port
    LevelHigh,      // high level on IO port
    LevelLow,       // low level on IO port
    Motion          // motion detection
};

// =============================================================================
//  Trigger configuration  (per-profile)
// =============================================================================

struct TriggerConfig {
    int          enabled           = -1;                  // 0 off, 1 on, -1 unchanged
    TriggerEvent event             = TriggerEvent::Unchanged;
    int          port              = -1;                  // IO port (1–4)
    int          minimumIntervalMs = -1;                  // min ms between triggers
    int          motionThreshold   = -1;                  // motion detection threshold
};

// =============================================================================
//  Exposure configuration  (per-profile)
// =============================================================================

struct ExposureConfig {
    int targetLevel = -1;         // AE target level, -1 unchanged

    struct ShutterSettings {
        int automatic  = -1;      // 0 manual, 1 auto, -1 unchanged
        int fixedValue = -1;      // manual shutter (us)
        int minValue   = -1;      // auto lower limit
        int maxValue   = -1;      // auto upper limit
    } shutter;

    struct GainSettings {
        int automatic  = -1;      // 0 manual, 1 auto, -1 unchanged
        int fixedValue = -1;      // fixed gain (x100, e.g. 100 = 1.0x)
        int minValue   = -1;
        int maxValue   = -1;
    } gain;

    struct IrisSettings {
        int automatic  = -1;      // 0 manual, 1 auto, -1 unchanged
        int fixedValue = -1;
    } iris;
};

// =============================================================================
//  Scenario crop
// =============================================================================

struct ScenarioCrop {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
};

// =============================================================================
//  Device info (read-only, from equip.miscRO)
// =============================================================================

struct DeviceInfo {
    std::string cameraModel;
    std::string firmwareVersion;
    Size imageSize;
    std::string sensorType;
    std::string lensModel;
    bool hdrAvailable = false;
    int framePeriodUs = 0;
};

// =============================================================================
//  Connection state  (used by onConnectionStateChanged callback)
// =============================================================================

enum class ConnectionState {
    Connected,
    Disconnected,
    Reconnecting,
    Reconnected
};

// =============================================================================
//  Auto-reconnect configuration
// =============================================================================

struct AutoReconnectConfig {
    bool     enabled    = false;
    uint32_t intervalMs = 3000;   // delay between retry attempts
    uint32_t maxRetries = 0;      // 0 = unlimited
};

// =============================================================================
//  Log level
// =============================================================================

enum class LogLevel { Info, Error };

} // namespace itscam
