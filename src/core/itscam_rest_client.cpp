/*
 *  itscam_rest_client.cpp
 *
 *  ITSCAM Client SDK - REST API client implementation
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  HTTP / HTTPS client for the ITSCAM webapp backend REST API.  Delegates
 *  the wire-level work to detail::HttpTransport, which centralises
 *  cpp-httplib usage, auth handling and mbedTLS configuration.
 */

#include "itscam_rest_client.h"
#include "impl/itscam_callback_queue.h"
#include "impl/itscam_http_transport.h"
#include "itscam_os.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace itscam {

using json      = nlohmann::json;
using Transport = detail::HttpTransport;
using Thread    = os::Thread;
using Mutex     = os::Mutex;
template<typename M> using LockGuard  = os::LockGuard<M>;
template<typename M> using UniqueLock = os::UniqueLock<M>;
namespace rt    = rest_types;

namespace {

/// Convert a `Result<json>` into a `Result<T>` by piping the JSON payload
/// through nlohmann's adl-based `from_json` machinery.  Schema mismatches
/// surface as `Error::InvalidParameter` with the original exception message
/// attached, so wrappers can distinguish them from transport / server
/// failures.
template <typename T>
Result<T> mapTyped(Result<json>&& raw) {
    if (!raw) return raw.error();
    try {
        return raw.value().get<T>();
    } catch (const std::exception& e) {
        return Error{Error::InvalidParameter,
                     std::string("schema mismatch: ") + e.what()};
    }
}

//=========================================================================
// Telemetry stream
//=========================================================================

// Pushed by the camera every 5 s; the stream path asks for exactly the
// GET /system/telemetry document
const char kTelemetryStreamPath[] = "/statusws?format=telemetry";

// Waits before reconnecting: doubles from the first up to the last
const uint32_t kTelemetryReconnectMinMs = 1000;
const uint32_t kTelemetryReconnectMaxMs = 30000;

int64_t unixTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/// The user's log handler, called through a callback queue by threads that
/// may outlive the client
struct LogSlot {
    Mutex mtx;
    std::function<void(LogLevel, const std::string&)> handler;

    std::function<void(LogLevel, const std::string&)> get() {
        LockGuard<Mutex> lk(mtx);
        return handler;
    }
};

/// The last telemetry document received, kept after the stream stops
struct LatestTelemetry {
    Mutex           mtx;
    bool            has = false;
    TelemetryUpdate update;
};

/// One telemetry stream and its thread, which connects, reads and
/// reconnects.  As the MJPEG stream of ItscamCgiClient, the thread only
/// queues callbacks on the client's callback queue, so it never runs user
/// code and any thread may join it.  The thread and each queued callback
/// hold a reference to the worker.
class TelemetryStreamWorker
    : public std::enable_shared_from_this<TelemetryStreamWorker> {
public:
    TelemetryStreamWorker(std::shared_ptr<Transport> transport,
                          std::shared_ptr<detail::CallbackQueue> callbacks,
                          std::shared_ptr<LogSlot> logSlot,
                          std::shared_ptr<LatestTelemetry> latest,
                          std::string path, uint32_t timeoutMs,
                          TelemetryCallback userCallback)
        : mTransport(std::move(transport))
        , mCallbacks(std::move(callbacks))
        , mLogSlot(std::move(logSlot))
        , mLatest(std::move(latest))
        , mPath(std::move(path))
        , mTimeoutMs(timeoutMs)
        , mUserCallback(std::move(userCallback))
    {}

    Result<void> start() {
        mRunning = true;
        auto self = shared_from_this();
        mThread = Thread([self]() { self->run(); });
        if (!mThread.joinable()) {
            mRunning = false;
            return Error{Error::Unknown,
                         "failed to start the telemetry stream thread"};
        }
        return Result<void>::success();
    }

    /// Ask the stream to end.  No document is delivered afterwards, not
    /// even one already queued.  Never waits: join() ends the connection.
    void requestStop() {
        {
            LockGuard<Mutex> lk(mWaitMtx);
            mShouldStop = true;
        }
        mWaitCv.notifyAll();
    }

    /// End the connection and wait for the stream thread to exit.  Several
    /// threads may wait at once.
    void join() {
        mCancel.cancel();
        LockGuard<Mutex> lk(mJoinMtx);
        if (mThread.joinable()) mThread.join();
    }

    bool isRunning() const { return mRunning.load() && !mShouldStop.load(); }
    bool isConnected() const { return mConnected.load(); }

private:
    void run() {
        uint32_t reconnectMs = kTelemetryReconnectMinMs;
        uint64_t sequence = 0;
        bool warnedFormat = false;

        while (!mShouldStop.load()) {
            bool received = false;
            auto res = mTransport->webSocket(
                mPath, mTimeoutMs,
                [this]() {
                    mConnected = true;
                    log(LogLevel::Info, "Telemetry stream connected");
                },
                [&](const std::string& message) -> bool {
                    if (mShouldStop.load()) return false;
                    json document = json::parse(message, nullptr, false);
                    if (!document.is_object() || !document.contains("version")) {
                        // Firmware without the system telemetry pushes only
                        // the web interface's status
                        if (!warnedFormat) {
                            warnedFormat = true;
                            log(LogLevel::Error,
                                "Telemetry stream: the camera sends no telemetry "
                                "(firmware without GET /api/system/telemetry?)");
                        }
                        return true;
                    }
                    received = true;
                    TelemetryUpdate update;
                    update.sequence     = ++sequence;
                    update.receivedAtMs = unixTimeMs();
                    update.document     = std::move(document);
                    deliver(std::move(update), message.size());
                    return !mShouldStop.load();
                },
                mCancel);
            mConnected = false;
            if (mShouldStop.load()) break;

            if (received) reconnectMs = kTelemetryReconnectMinMs;
            log(LogLevel::Error,
                "Telemetry stream: " +
                    (res ? std::string("closed") : res.error().message) +
                    "; reconnecting in " + std::to_string(reconnectMs) + " ms");
            {
                UniqueLock<Mutex> lk(mWaitMtx);
                mWaitCv.waitFor(lk, reconnectMs,
                                [this]() { return mShouldStop.load(); });
            }
            reconnectMs = std::min(reconnectMs * 2, kTelemetryReconnectMaxMs);
        }
        mConnected = false;
        mRunning = false;
    }

    // Kept as the latest, then queued for the callback; a callback that
    // falls behind gets the latest documents
    void deliver(TelemetryUpdate update, size_t bytes) {
        {
            LockGuard<Mutex> lk(mLatest->mtx);
            mLatest->update = update;
            mLatest->has = true;
        }
        if (!mUserCallback || mShouldStop.load()) return;
        mCallbacks->post(
            [self = shared_from_this(), update = std::move(update)]() {
                if (!self->mShouldStop.load() && self->mUserCallback) {
                    self->mUserCallback(update);
                }
            },
            bytes, detail::CallbackQueue::Kind::Frame);
    }

    // Logs run on the callback queue too, never on the stream thread
    void log(LogLevel lvl, const std::string& msg) {
        if (!mLogSlot->get()) return;
        std::shared_ptr<LogSlot> slot = mLogSlot;
        mCallbacks->post([slot, lvl, msg]() {
            auto handler = slot->get();
            if (handler) handler(lvl, msg);
        }, msg.size(), detail::CallbackQueue::Kind::Event);
    }

    std::shared_ptr<Transport>             mTransport;
    std::shared_ptr<detail::CallbackQueue> mCallbacks;
    std::shared_ptr<LogSlot>               mLogSlot;
    std::shared_ptr<LatestTelemetry>       mLatest;
    std::string                            mPath;
    uint32_t                               mTimeoutMs;
    TelemetryCallback                      mUserCallback;
    Transport::StreamCancel                mCancel;
    Thread                                 mThread;
    Mutex                                  mJoinMtx;   // a second join() waits for the first
    Mutex                                  mWaitMtx;   // reconnection wait
    os::ConditionVariable                  mWaitCv;
    std::atomic<bool>                      mRunning{false};
    std::atomic<bool>                      mConnected{false};
    std::atomic<bool>                      mShouldStop{false};
};

}  // namespace

//=========================================================================
// Impl
//=========================================================================

struct ItscamRestClient::Impl {

    // Shared with the telemetry stream thread, which may outlive a call
    std::shared_ptr<Transport> transport = std::make_shared<Transport>();
    std::string apiPrefix = "/api";

    // Telemetry and stream log callbacks run on the shared callback pool
    // (see CallbackQueue), never on the stream thread.  Request logs still
    // run on the calling thread.
    std::shared_ptr<detail::CallbackQueue> callbacks =
        detail::CallbackQueue::create();
    std::shared_ptr<LogSlot>         logSlot = std::make_shared<LogSlot>();
    std::shared_ptr<LatestTelemetry> latest  = std::make_shared<LatestTelemetry>();

    // A telemetry callback may start or stop the stream while another
    // thread uses the client.
    mutable Mutex                          streamMtx;
    std::shared_ptr<TelemetryStreamWorker> telemetryWorker;

    Impl() {
        std::shared_ptr<LogSlot> slot = logSlot;
        callbacks->setDropReporter([slot](uint64_t dropped) {
            auto handler = slot->get();
            if (handler) {
                handler(LogLevel::Error,
                        "Callbacks fell behind: " + std::to_string(dropped) +
                        " events dropped");
            }
        });
    }

    ~Impl() {
        stopTelemetryStream();
        // Destroyed from a callback, that one runs on: the queue only drops
        // the others
        callbacks->close();
    }

    // Stops the stream.  Once this returns no telemetry callback runs,
    // unless it is called from one, which then runs on.
    void stopTelemetryStream() {
        std::shared_ptr<TelemetryStreamWorker> worker;
        {
            LockGuard<Mutex> lk(streamMtx);
            worker = std::move(telemetryWorker);
            // Under streamMtx: a concurrent stop that finds no worker must
            // find this one's documents already cancelled
            if (worker) worker->requestStop();
        }
        // Without streamMtx: ending the connection may wait for a connection
        // attempt, and the next stream may start meanwhile
        if (worker) worker->join();
        callbacks->waitForCallback();
    }

    /// Parse the body of an HttpResponse as JSON, falling back to a string
    /// JSON value when the body is not valid JSON.  Empty bodies become
    /// nullptr.
    static json parseBody(const std::vector<uint8_t>& body) {
        if (body.empty()) return json();
        try {
            return json::parse(body.begin(), body.end());
        } catch (const json::parse_error&) {
            return json(std::string(body.begin(), body.end()));
        }
    }

    /// Map an HttpResponse onto a Result<json>.  Successful 2xx responses
    /// are returned as parsed JSON; everything else becomes an Error.
    Result<json> mapResponse(const Result<detail::HttpResponse>& res,
                             const std::string& method,
                             const std::string& path) {
        if (!res) {
            return res.error();
        }

        const auto& rsp  = res.value();
        json        body = parseBody(rsp.body);

        if (rsp.status >= 200 && rsp.status < 300) {
            return body;
        }

        std::string message;
        if (body.is_object() && body.contains("message")) {
            try {
                message = body["message"].get<std::string>();
            } catch (...) {
                message.clear();
            }
        }
        return detail::mapHttpStatusToError(rsp.status, message,
                                            method, path);
    }

    // --- Core HTTP verbs -----------------------------------------------------

    Result<json> doGet(const std::string& path, uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method = "GET";
        req.path   = path;
        return mapResponse(transport->request(req, timeoutMs), "GET", path);
    }

    Result<json> doPut(const std::string& path, const json& body,
                       uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method      = "PUT";
        req.path        = path;
        req.body        = body.dump();
        req.contentType = "application/json";
        return mapResponse(transport->request(req, timeoutMs), "PUT", path);
    }

    Result<json> doPost(const std::string& path, const json& body,
                        uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method      = "POST";
        req.path        = path;
        req.body        = body.dump();
        req.contentType = "application/json";
        return mapResponse(transport->request(req, timeoutMs), "POST", path);
    }

    Result<json> doDelete(const std::string& path, uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method = "DELETE";
        req.path   = path;
        return mapResponse(transport->request(req, timeoutMs), "DELETE", path);
    }
};

//=========================================================================
// Constructor / destructor / move
//=========================================================================

ItscamRestClient::ItscamRestClient() : mImpl(new Impl()) {}
ItscamRestClient::~ItscamRestClient() = default;

ItscamRestClient::ItscamRestClient(ItscamRestClient&&) noexcept = default;
ItscamRestClient& ItscamRestClient::operator=(ItscamRestClient&&) noexcept
    = default;

//=========================================================================
// Connection
//=========================================================================

void ItscamRestClient::setBaseUrl(const std::string& host, uint16_t port,
                                  const std::string& scheme) {
    mImpl->transport->setBaseUrl(host, port, scheme);
}

//=========================================================================
// TLS
//=========================================================================

void ItscamRestClient::setCaCertFile(const std::string& pemPath) {
    mImpl->transport->setCaCertFile(pemPath);
}

void ItscamRestClient::setCaCertData(const std::string& pem) {
    mImpl->transport->setCaCertData(pem);
}

void ItscamRestClient::setVerifyServerCertificate(bool verify) {
    mImpl->transport->setVerifyServerCertificate(verify);
}

void ItscamRestClient::setClientCertificate(const std::string& certPem,
                                            const std::string& keyPem) {
    mImpl->transport->setClientCertificate(certPem, keyPem);
}

//=========================================================================
// Authentication
//=========================================================================

Result<nlohmann::json> ItscamRestClient::login(const std::string& username,
                                               const std::string& password,
                                               uint32_t timeoutMs) {
    json reqBody = {
        {"params", {
            {"username", username},
            {"password", password}
        }}
    };

    auto result = mImpl->doPost(mImpl->apiPrefix + "/auth", reqBody,
                                timeoutMs);
    if (result) {
        const auto& body = result.value();
        if (body.is_object() && body.contains("token")) {
            mImpl->transport->setBearerToken(
                body["token"].get<std::string>());
            mImpl->transport->log(LogLevel::Info,
                                 "Authenticated as '" + username + "'");
        }
    }
    return result;
}

void ItscamRestClient::setAuthToken(const std::string& token) {
    mImpl->transport->setBearerToken(token);
}

void ItscamRestClient::clearAuthToken() {
    mImpl->transport->clearBearerToken();
}

//=========================================================================
// Image profiles
//=========================================================================

Result<std::vector<rt::ProfileConfig>> ItscamRestClient::getProfiles(
    uint32_t timeoutMs) {
    return mapTyped<std::vector<rt::ProfileConfig>>(
        mImpl->doGet(mImpl->apiPrefix + "/image/profiles", timeoutMs));
}

Result<std::vector<rt::ProfileConfig>> ItscamRestClient::getProfile(
    int id, uint32_t timeoutMs) {
    std::string path =
        mImpl->apiPrefix + "/image/profiles?id=" + std::to_string(id);
    return mapTyped<std::vector<rt::ProfileConfig>>(
        mImpl->doGet(path, timeoutMs));
}

Result<rt::ProfileConfig> ItscamRestClient::createProfile(
    const rt::ProfileConfig& profile, uint32_t timeoutMs) {
    return mapTyped<rt::ProfileConfig>(
        mImpl->doPost(mImpl->apiPrefix + "/image/profiles",
                      json(profile), timeoutMs));
}

Result<rt::ProfileConfig> ItscamRestClient::getProfileByName(
    const std::string& name, uint32_t timeoutMs) {
    auto all = getProfiles(timeoutMs);
    if (!all) return all.error();
    for (auto& p : all.value()) {
        if (p.name && *p.name == name)
            return std::move(p);
    }
    return Error{Error::InvalidParameter,
                 "profile not found: \"" + name + "\""};
}

Result<rt::ProfileConfig> ItscamRestClient::updateProfileById(
    int id, const rt::ProfileConfig& profile, uint32_t timeoutMs) {
    std::string path =
        mImpl->apiPrefix + "/image/profiles/" + std::to_string(id);
    return mapTyped<rt::ProfileConfig>(
        mImpl->doPut(path, rt::to_partial_json(profile), timeoutMs));
}

Result<rt::ProfileConfig> ItscamRestClient::updateProfileByName(
    const std::string& name, const rt::ProfileConfig& profile,
    uint32_t timeoutMs) {
    auto found = getProfileByName(name, timeoutMs);
    if (!found) return found.error();
    return updateProfileById(static_cast<int>(found.value().id), profile,
                             timeoutMs);
}

Result<std::vector<rt::ProfileConfig>> ItscamRestClient::updateProfiles(
    const std::vector<rt::ProfileConfig>& profiles, uint32_t timeoutMs) {
    json arr = json::array();
    for (auto const& p : profiles) arr.push_back(rt::to_partial_json(p));
    return mapTyped<std::vector<rt::ProfileConfig>>(
        mImpl->doPut(mImpl->apiPrefix + "/image/profiles",
                     arr, timeoutMs));
}

Result<nlohmann::json> ItscamRestClient::deleteProfile(int id,
                                                       uint32_t timeoutMs) {
    std::string path =
        mImpl->apiPrefix + "/image/profiles?id=" + std::to_string(id);
    return mImpl->doDelete(path, timeoutMs);
}

//=========================================================================
// Equipment volatile info
//=========================================================================

Result<rt::MiscVolatile> ItscamRestClient::getVolatileInfo(
    uint32_t timeoutMs) {
    return mapTyped<rt::MiscVolatile>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/misc/readonly/volatile", timeoutMs));
}

//=========================================================================
// Equipment general (untyped until Phase 2)
//=========================================================================

Result<nlohmann::json> ItscamRestClient::getGeneralConfig(
    uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/equipment/general", timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::setGeneralConfig(
    const nlohmann::json& config, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/equipment/general", config,
                        timeoutMs);
}

//=========================================================================
// Analytics
//=========================================================================

Result<rt::AnalyticsConfig> ItscamRestClient::getAnalyticsConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::AnalyticsConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/analytics", timeoutMs));
}

Result<rt::AnalyticsConfig> ItscamRestClient::setAnalyticsConfig(
    const rt::AnalyticsConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::AnalyticsConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/analytics",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// OCR
//=========================================================================

Result<rt::OcrConfig> ItscamRestClient::getOcrConfig(uint32_t timeoutMs) {
    return mapTyped<rt::OcrConfig>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/ocr", timeoutMs));
}

Result<rt::OcrConfig> ItscamRestClient::setOcrConfig(
    const rt::OcrConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::OcrConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/ocr",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Classifier
//=========================================================================

Result<rt::ClassifierConfig> ItscamRestClient::getClassifierConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ClassifierConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/classifier", timeoutMs));
}

Result<rt::ClassifierConfig> ItscamRestClient::setClassifierConfig(
    const rt::ClassifierConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::ClassifierConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/classifier",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Lanes
//=========================================================================

Result<rt::LanesConfig> ItscamRestClient::getLanesConfig(uint32_t timeoutMs) {
    return mapTyped<rt::LanesConfig>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/lanes", timeoutMs));
}

Result<rt::LanesConfig> ItscamRestClient::setLanesConfig(
    const rt::LanesConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::LanesConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/lanes",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// ITSCAM PRO
//=========================================================================

Result<rt::ItscamproConfig> ItscamRestClient::getItscamproConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ItscamproConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/itscampro", timeoutMs));
}

Result<rt::ItscamproConfig> ItscamRestClient::setItscamproConfig(
    const rt::ItscamproConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::ItscamproConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/itscampro",
        rt::to_partial_json(config), timeoutMs));
}

Result<rt::ItscamproStatus> ItscamRestClient::getItscamproStatus(
    uint32_t timeoutMs) {
    return mapTyped<rt::ItscamproStatus>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/itscampro/status", timeoutMs));
}

//=========================================================================
// AutoFocus
//=========================================================================

Result<rt::AutoFocus> ItscamRestClient::getAutoFocus(uint32_t timeoutMs) {
    return mapTyped<rt::AutoFocus>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/autofocus", timeoutMs));
}

Result<rt::AutoFocus> ItscamRestClient::setAutoFocus(
    const rt::AutoFocus& config, uint32_t timeoutMs) {
    return mapTyped<rt::AutoFocus>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/autofocus",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Stream config
//=========================================================================

Result<rt::StreamConfig> ItscamRestClient::getStreamConfig(uint32_t timeoutMs) {
    return mapTyped<rt::StreamConfig>(
        mImpl->doGet(mImpl->apiPrefix + "/video/streams", timeoutMs));
}

Result<rt::StreamConfig> ItscamRestClient::setStreamConfig(
    const rt::StreamConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::StreamConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/video/streams",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Misc
//=========================================================================

Result<rt::Misc> ItscamRestClient::getMisc(uint32_t timeoutMs) {
    return mapTyped<rt::Misc>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/misc", timeoutMs));
}

Result<rt::Misc> ItscamRestClient::setMisc(const rt::Misc& config,
                                           uint32_t timeoutMs) {
    return mapTyped<rt::Misc>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/misc",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Image sign
//=========================================================================

Result<rt::ImageSignConfig> ItscamRestClient::getImageSignConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ImageSignConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/imageSign", timeoutMs));
}

//=========================================================================
// FTP
//=========================================================================

Result<rt::FtpConfig> ItscamRestClient::getFtpConfig(uint32_t timeoutMs) {
    return mapTyped<rt::FtpConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/ftp", timeoutMs));
}

Result<rt::FtpConfig> ItscamRestClient::setFtpConfig(
    const rt::FtpConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::FtpConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/ftp",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Lince
//=========================================================================

Result<rt::LinceConfig> ItscamRestClient::getLinceConfig(uint32_t timeoutMs) {
    return mapTyped<rt::LinceConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/lince", timeoutMs));
}

Result<rt::LinceConfig> ItscamRestClient::setLinceConfig(
    const rt::LinceConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::LinceConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/lince",
        rt::to_partial_json(config), timeoutMs));
}

Result<rt::LinceStatus> ItscamRestClient::getLinceStatus(uint32_t timeoutMs) {
    return mapTyped<rt::LinceStatus>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/lince/status", timeoutMs));
}

//=========================================================================
// Vehicle indicator
//=========================================================================

Result<rt::VehicleIndicatorConfig>
ItscamRestClient::getVehicleIndicatorConfig(uint32_t timeoutMs) {
    return mapTyped<rt::VehicleIndicatorConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/vehicleIndicator", timeoutMs));
}

Result<rt::VehicleIndicatorConfig>
ItscamRestClient::setVehicleIndicatorConfig(
    const rt::VehicleIndicatorConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::VehicleIndicatorConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/vehicleIndicator",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Protocols
//=========================================================================

Result<rt::ProtocolsConfig> ItscamRestClient::getProtocolsConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ProtocolsConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/protocols", timeoutMs));
}

Result<rt::ProtocolsConfig> ItscamRestClient::setProtocolsConfig(
    const rt::ProtocolsConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::ProtocolsConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/protocols",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// Profile transitioner
//=========================================================================

Result<rt::ProfileTransitioner> ItscamRestClient::getProfileTransitioner(
    uint32_t timeoutMs) {
    return mapTyped<rt::ProfileTransitioner>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/transitioner", timeoutMs));
}

Result<rt::ProfileTransitioner> ItscamRestClient::setProfileTransitioner(
    const rt::ProfileTransitioner& config, uint32_t timeoutMs) {
    return mapTyped<rt::ProfileTransitioner>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/transitioner",
        rt::to_partial_json(config), timeoutMs));
}

//=========================================================================
// I/O ports
//=========================================================================

Result<std::vector<rt::IoConfig>> ItscamRestClient::getIoPorts(
    uint32_t timeoutMs) {
    return mapTyped<std::vector<rt::IoConfig>>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/ioPorts", timeoutMs));
}

Result<std::vector<rt::IoConfig>> ItscamRestClient::setIoPorts(
    const std::vector<rt::IoConfig>& ports, uint32_t timeoutMs) {
    json arr = json::array();
    for (auto const& p : ports) arr.push_back(rt::to_partial_json(p));
    return mapTyped<std::vector<rt::IoConfig>>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/ioPorts", arr, timeoutMs));
}

Result<rt::IoConfig> ItscamRestClient::getIoPort(int id,
                                                 uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/equipment/ioPorts/"
                       + std::to_string(id);
    return mapTyped<rt::IoConfig>(mImpl->doGet(path, timeoutMs));
}

Result<rt::IoConfig> ItscamRestClient::setIoPort(int id,
                                                 const rt::IoConfig& port,
                                                 uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/equipment/ioPorts/"
                       + std::to_string(id);
    return mapTyped<rt::IoConfig>(
        mImpl->doPut(path, rt::to_partial_json(port), timeoutMs));
}

Result<std::vector<rt::IoBasic>> ItscamRestClient::getIoBasic(
    uint32_t timeoutMs) {
    return mapTyped<std::vector<rt::IoBasic>>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/ioBasic", timeoutMs));
}

Result<std::vector<rt::IoBasic>> ItscamRestClient::setIoBasic(
    const std::vector<rt::IoBasic>& ports, uint32_t timeoutMs) {
    json arr = json::array();
    for (auto const& p : ports) arr.push_back(rt::to_partial_json(p));
    return mapTyped<std::vector<rt::IoBasic>>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/ioBasic", arr, timeoutMs));
}

//=========================================================================
// REST API client (webhook) servers
//=========================================================================

Result<rt::RestApiClientConfig> ItscamRestClient::getRestApiClientConfig(
    int id, uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/equipment/servers/restapiclient/"
                       + std::to_string(id) + "/config";
    return mapTyped<rt::RestApiClientConfig>(mImpl->doGet(path, timeoutMs));
}

Result<rt::RestApiClientConfig> ItscamRestClient::setRestApiClientConfig(
    int id, const rt::RestApiClientConfig& config, uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/equipment/servers/restapiclient/"
                       + std::to_string(id) + "/config";
    return mapTyped<rt::RestApiClientConfig>(
        mImpl->doPut(path, rt::to_partial_json(config), timeoutMs));
}

Result<rt::RestApiClientStatus> ItscamRestClient::getRestApiClientStatus(
    int id, uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/equipment/servers/restapiclient/"
                       + std::to_string(id) + "/status";
    return mapTyped<rt::RestApiClientStatus>(mImpl->doGet(path, timeoutMs));
}

//=========================================================================
// Licenses
//=========================================================================

Result<rt::Licenses> ItscamRestClient::getLicenses(uint32_t timeoutMs) {
    return mapTyped<rt::Licenses>(
        mImpl->doGet(mImpl->apiPrefix + "/system/licenses", timeoutMs));
}

//=========================================================================
// System telemetry
//=========================================================================

Result<rt::SystemTelemetry> TelemetryUpdate::telemetry() const {
    return mapTyped<rt::SystemTelemetry>(Result<json>(document));
}

Result<rt::SystemTelemetry> ItscamRestClient::getSystemTelemetry(
    uint32_t timeoutMs) {
    return mapTyped<rt::SystemTelemetry>(
        mImpl->doGet(mImpl->apiPrefix + "/system/telemetry", timeoutMs));
}

Result<void> ItscamRestClient::startTelemetryStream(TelemetryCallback callback,
                                                    uint32_t timeoutMs) {
    // Never waits for a telemetry callback: the caller may hold a lock that
    // they need.
    std::shared_ptr<TelemetryStreamWorker> ended;
    Result<void> started;
    {
        LockGuard<Mutex> lk(mImpl->streamMtx);
        if (mImpl->telemetryWorker) {
            if (mImpl->telemetryWorker->isRunning()) {
                return Error{Error::InvalidParameter,
                             "telemetry stream already running"};
            }
            ended = std::move(mImpl->telemetryWorker);
            ended->requestStop();
        }
        auto worker = std::make_shared<TelemetryStreamWorker>(
            mImpl->transport, mImpl->callbacks, mImpl->logSlot, mImpl->latest,
            mImpl->apiPrefix + kTelemetryStreamPath, timeoutMs,
            std::move(callback));
        started = worker->start();
        if (started) mImpl->telemetryWorker = std::move(worker);
    }
    if (ended) ended->join();
    return started;
}

void ItscamRestClient::stopTelemetryStream() {
    mImpl->stopTelemetryStream();
}

bool ItscamRestClient::isTelemetryStreamRunning() const {
    LockGuard<Mutex> lk(mImpl->streamMtx);
    return mImpl->telemetryWorker && mImpl->telemetryWorker->isRunning();
}

bool ItscamRestClient::isTelemetryStreamConnected() const {
    LockGuard<Mutex> lk(mImpl->streamMtx);
    return mImpl->telemetryWorker && mImpl->telemetryWorker->isConnected();
}

Result<TelemetryUpdate> ItscamRestClient::latestTelemetry() const {
    LockGuard<Mutex> lk(mImpl->latest->mtx);
    if (!mImpl->latest->has) {
        return Error{Error::Disconnected, "no telemetry received yet"};
    }
    return mImpl->latest->update;
}

//=========================================================================
// Generic HTTP methods
//=========================================================================

Result<nlohmann::json> ItscamRestClient::httpGet(const std::string& path,
                                                 uint32_t timeoutMs) {
    return mImpl->doGet(path, timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::httpPut(const std::string& path,
                                                 const nlohmann::json& body,
                                                 uint32_t timeoutMs) {
    return mImpl->doPut(path, body, timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::patchJson(const std::string& path,
                                                    const nlohmann::json& patch,
                                                    uint32_t timeoutMs) {
    return mImpl->doPut(path, patch, timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::httpPost(const std::string& path,
                                                  const nlohmann::json& body,
                                                  uint32_t timeoutMs) {
    return mImpl->doPost(path, body, timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::httpDelete(const std::string& path,
                                                    uint32_t timeoutMs) {
    return mImpl->doDelete(path, timeoutMs);
}

//=========================================================================
// Settings
//=========================================================================

void ItscamRestClient::setApiPrefix(const std::string& prefix) {
    mImpl->apiPrefix = prefix;
}

std::string ItscamRestClient::apiPrefix() const {
    return mImpl->apiPrefix;
}

void ItscamRestClient::setLogHandler(
    std::function<void(LogLevel, const std::string&)> cb) {
    {
        LockGuard<Mutex> lk(mImpl->logSlot->mtx);
        mImpl->logSlot->handler = cb;
    }
    mImpl->transport->setLogHandler(std::move(cb));
}

} // namespace itscam
