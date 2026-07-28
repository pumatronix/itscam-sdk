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
#include "impl/itscam_http_transport.h"
#include "impl/itscam_websocket_transport.h"

#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace itscam {

using json      = nlohmann::json;
using Transport = detail::HttpTransport;
namespace rt    = rest_types;

namespace {

const char* phaseToString(ItscamRestClient::SoftwareUpdatePhase phase) {
    using Phase = ItscamRestClient::SoftwareUpdatePhase;
    switch (phase) {
        case Phase::Idle: return "idle";
        case Phase::Connecting: return "connecting";
        case Phase::Uploading: return "uploading";
        case Phase::Installing: return "installing";
        case Phase::Restarting: return "restarting";
        case Phase::Succeeded: return "succeeded";
        case Phase::Failed: return "failed";
        case Phase::Cancelled: return "cancelled";
    }
    return "unknown";
}

const char* errorCodeToString(Error::Code code) {
    switch (code) {
        case Error::ConnectionFailed: return "ConnectionFailed";
        case Error::Timeout: return "Timeout";
        case Error::NotAuthenticated: return "NotAuthenticated";
        case Error::InvalidParameter: return "InvalidParameter";
        case Error::ServerError: return "ServerError";
        case Error::Disconnected: return "Disconnected";
        case Error::Unknown: return "Unknown";
    }
    return "Unknown";
}

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

}  // namespace

//=========================================================================
// Impl
//=========================================================================

struct ItscamRestClient::Impl {

    Transport   transport;
    std::string apiPrefix = "/api";

    struct OperationContext {
        std::string host;
        uint16_t    port = 0;
        std::string scheme;
        std::string bearerToken;
        std::string apiPrefix;
        detail::TlsConfig tls;
    };

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
    static Result<json> mapResponse(const Result<detail::HttpResponse>& res,
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
        return mapResponse(transport.request(req, timeoutMs), "GET", path);
    }

    Result<json> doPut(const std::string& path, const json& body,
                       uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method      = "PUT";
        req.path        = path;
        req.body        = body.dump();
        req.contentType = "application/json";
        return mapResponse(transport.request(req, timeoutMs), "PUT", path);
    }

    Result<json> doPost(const std::string& path, const json& body,
                        uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method      = "POST";
        req.path        = path;
        req.body        = body.dump();
        req.contentType = "application/json";
        return mapResponse(transport.request(req, timeoutMs), "POST", path);
    }

    Result<json> doPostMultipartFile(const std::string& path,
                                     const std::string& fieldName,
                                     const std::string& filePath,
                                     const std::string& contentType,
                                     uint32_t timeoutMs,
                                     detail::UploadProgress progress) {
        return mapResponse(transport.postMultipartFile(path, fieldName,
                                                       filePath, contentType,
                                                       timeoutMs,
                                                       std::move(progress)),
                           "POST", path);
    }

    OperationContext makeOperationContext() const {
        OperationContext ctx;
        ctx.host = transport.host();
        ctx.port = transport.port();
        ctx.scheme = transport.scheme();
        ctx.bearerToken = transport.bearerToken();
        ctx.apiPrefix = apiPrefix;
        ctx.tls = transport.tlsConfig();
        return ctx;
    }

    static void configureTransport(Transport& transport,
                                   const OperationContext& ctx) {
        transport.setBaseUrl(ctx.host, ctx.port, ctx.scheme);
        if (!ctx.bearerToken.empty()) {
            transport.setBearerToken(ctx.bearerToken);
        }
        transport.setVerifyServerCertificate(ctx.tls.verifyServerCert);
        if (!ctx.tls.caCertFile.empty()) {
            transport.setCaCertFile(ctx.tls.caCertFile);
        }
        if (!ctx.tls.caCertData.empty()) {
            transport.setCaCertData(ctx.tls.caCertData);
        }
        if (!ctx.tls.clientCertPem.empty() && !ctx.tls.clientKeyPem.empty()) {
            transport.setClientCertificate(ctx.tls.clientCertPem,
                                           ctx.tls.clientKeyPem);
        }
    }

    Result<json> doDelete(const std::string& path, uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method = "DELETE";
        req.path   = path;
        return mapResponse(transport.request(req, timeoutMs), "DELETE", path);
    }
};

//=========================================================================
// Software update operation internals
//=========================================================================

struct ItscamRestClient::SoftwareUpdateOperation::Impl
    : public std::enable_shared_from_this<Impl> {

    using Phase = ItscamRestClient::SoftwareUpdatePhase;
    using Status = ItscamRestClient::SoftwareUpdateStatus;
    using Options = ItscamRestClient::SoftwareUpdateOptions;
    using Callback = ItscamRestClient::SoftwareUpdateStatusCallback;

    ItscamRestClient::Impl::OperationContext context;
    Options options;
    Callback callback;

    mutable os::Mutex mtx;
    os::ConditionVariable cv;
    Status current;
    bool cancelRequested = false;
    bool finished = false;
    bool installTerminal = false;
    bool installSucceeded = false;
    Error readerError;

    std::shared_ptr<detail::WebSocketClient> websocket;
    os::Thread worker;
    os::Thread reader;

    Impl(ItscamRestClient::Impl::OperationContext ctx, Options opts,
         Callback cb)
        : context(std::move(ctx)), options(std::move(opts)),
          callback(std::move(cb)) {}

    void start() {
        auto self = shared_from_this();
        worker = os::Thread([self]() { self->run(); });
    }

    Status snapshot() const {
        os::LockGuard<os::Mutex> lk(mtx);
        return current;
    }

    bool isDone() const {
        os::LockGuard<os::Mutex> lk(mtx);
        return current.complete;
    }

    void setCallback(Callback cb) {
        Status copy;
        {
            os::LockGuard<os::Mutex> lk(mtx);
            callback = std::move(cb);
            copy = current;
        }
        emit(copy);
    }

    void emit(const Status& status) {
        Callback cb;
        {
            os::LockGuard<os::Mutex> lk(mtx);
            cb = callback;
        }
        if (cb) cb(status);
    }

    void updateStatus(const std::function<void(Status&)>& mutate,
                      bool notify = true) {
        Status copy;
        {
            os::LockGuard<os::Mutex> lk(mtx);
            mutate(current);
            copy = current;
        }
        cv.notifyAll();
        if (notify) emit(copy);
    }

    void finish(Phase phase, const Error& err = Error{}) {
        updateStatus([&](Status& status) {
            status.phase = phase;
            status.complete = true;
            status.error = err;
            finished = true;
        });
        if (websocket) websocket->cancel();
    }

    void cancel() {
        {
            os::LockGuard<os::Mutex> lk(mtx);
            if (current.complete) return;
            cancelRequested = true;
        }
        finish(Phase::Cancelled, Error{Error::Disconnected,
                                       "software update cancelled"});
    }

    Result<Status> wait(uint32_t timeoutMs) {
        os::UniqueLock<os::Mutex> lk(mtx);
        if (timeoutMs == 0) {
            cv.wait(lk, [&]() { return current.complete; });
        } else if (!cv.waitFor(lk, timeoutMs, [&]() { return current.complete; })) {
            return Error{Error::Timeout, "software update wait timed out"};
        }
        return current;
    }

    bool shouldCancel() const {
        os::LockGuard<os::Mutex> lk(mtx);
        return cancelRequested || finished;
    }

    Result<json> postMultipart(Transport& transport,
                               const std::string& path,
                               uint32_t timeoutMs) {
        return ItscamRestClient::Impl::mapResponse(
            transport.postMultipartFile(path, "file", options.swuPath,
                                        "application/octet-stream",
                                        timeoutMs,
                                        [this](size_t currentBytes,
                                               size_t totalBytes) {
                updateStatus([&](Status& status) {
                    status.phase = Phase::Uploading;
                    status.uploadCurrent = static_cast<uint64_t>(currentBytes);
                    status.uploadTotal = static_cast<uint64_t>(totalBytes);
                });
                return !shouldCancel();
            }),
            "POST", path);
    }

    Result<json> postRestart(Transport& transport,
                             const std::string& path,
                             uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method = "POST";
        req.path = path;
        req.body = "{}";
        req.contentType = "application/json";
        return ItscamRestClient::Impl::mapResponse(
            transport.request(req, timeoutMs), "POST", path);
    }

    void handleStatusMessage(const json& message, const std::string& raw) {
        if (!message.is_object()) return;
        const std::string type = message.value("type", "");
        if (type == "status") {
            const std::string value = message.value("status", "");
            updateStatus([&](Status& status) {
                status.installStatus = value;
                status.rawMessage = raw;
                if (value == "START") {
                    status.phase = Phase::Installing;
                } else if (value == "SUCCESS") {
                    installTerminal = true;
                    installSucceeded = true;
                } else if (value == "FAILURE") {
                    installTerminal = true;
                    installSucceeded = false;
                    status.phase = Phase::Failed;
                    status.complete = true;
                    status.error = Error{Error::ServerError,
                                         "software update failed"};
                    finished = true;
                }
            });
        } else if (type == "message") {
            updateStatus([&](Status& status) {
                status.message = message.value("text", "");
                status.messageLevel = message.value("level", "");
                status.rawMessage = raw;
            });
        } else if (type == "step") {
            updateStatus([&](Status& status) {
                status.phase = Phase::Installing;
                status.stepName = message.value("name", "");
                status.rawMessage = raw;
                if (message.contains("percent")) {
                    if (message["percent"].is_number_integer()) {
                        status.stepPercent = message["percent"].get<int>();
                    } else if (message["percent"].is_string()) {
                        status.stepPercent = std::atoi(message["percent"].get<std::string>().c_str());
                    }
                }
            });
        }
    }

    void readLoop() {
        while (!shouldCancel()) {
            auto msg = websocket->readText(1000);
            if (!msg) {
                if (msg.error().code == Error::Timeout) continue;
                os::LockGuard<os::Mutex> lk(mtx);
                if (!finished && !installTerminal && !cancelRequested) {
                    readerError = msg.error();
                    cv.notifyAll();
                }
                return;
            }
            try {
                handleStatusMessage(json::parse(msg.value()), msg.value());
            } catch (const std::exception&) {
                updateStatus([&](Status& status) {
                    status.rawMessage = msg.value();
                });
            }
        }
    }

    bool waitForInstallTerminal(uint32_t timeoutMs) {
        const uint64_t deadline = os::monotonicMs() + timeoutMs;
        os::UniqueLock<os::Mutex> lk(mtx);
        while (!installTerminal && !finished && !cancelRequested &&
               readerError.message.empty()) {
            uint64_t now = os::monotonicMs();
            if (now >= deadline) return false;
            cv.waitFor(lk, static_cast<uint32_t>(deadline - now));
        }
        return installTerminal;
    }

    void run() {
        updateStatus([](Status& status) {
            status.phase = Phase::Connecting;
        });

        websocket.reset(new detail::WebSocketClient());
        detail::WebSocketOptions wsOptions;
        wsOptions.host = context.host;
        wsOptions.port = context.port;
        wsOptions.scheme = context.scheme;
        wsOptions.path = context.apiPrefix + "/swupdate";
        wsOptions.bearerToken = context.bearerToken;

        auto connected = websocket->connect(wsOptions);
        if (!connected) {
            finish(Phase::Failed, connected.error());
            return;
        }

        auto self = shared_from_this();
        reader = os::Thread([self]() { self->readLoop(); });

        Transport transport;
        ItscamRestClient::Impl::configureTransport(transport, context);
        const std::string uploadPath = context.apiPrefix + "/swupdate/upload";
        auto uploaded = postMultipart(transport, uploadPath, options.uploadTimeoutMs);
        if (!uploaded) {
            if (shouldCancel()) return;
            finish(Phase::Failed, uploaded.error());
            return;
        }

        if (!waitForInstallTerminal(options.statusTimeoutMs)) {
            if (shouldCancel()) return;
            Error err = readerError.message.empty()
                ? Error{Error::Timeout, "software update status timed out"}
                : readerError;
            finish(Phase::Failed, err);
            return;
        }

        if (!installSucceeded) {
            finish(Phase::Failed, Error{Error::ServerError,
                                        "software update failed"});
            return;
        }

        if (options.requestRestart) {
            updateStatus([](Status& status) {
                status.phase = Phase::Restarting;
            });
            const std::string restartPath = context.apiPrefix + "/swupdate/restart";
            auto restarted = postRestart(transport, restartPath,
                                         options.restartTimeoutMs);
            if (!restarted) {
                finish(Phase::Failed, restarted.error());
                return;
            }
        }

        finish(Phase::Succeeded);
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
// Software update status helpers
//=========================================================================

nlohmann::json ItscamRestClient::SoftwareUpdateStatus::toJson() const {
    json out = {
        {"phase", phaseToString(phase)},
        {"complete", complete},
        {"uploadCurrent", uploadCurrent},
        {"uploadTotal", uploadTotal},
        {"installStatus", installStatus},
        {"stepName", stepName},
        {"stepPercent", stepPercent},
        {"message", message},
        {"messageLevel", messageLevel},
        {"rawMessage", rawMessage}
    };
    if (!error.message.empty()) {
        out["error"] = {
            {"code", errorCodeToString(error.code)},
            {"message", error.message}
        };
    }
    return out;
}

ItscamRestClient::SoftwareUpdateOperation::SoftwareUpdateOperation() = default;
ItscamRestClient::SoftwareUpdateOperation::~SoftwareUpdateOperation() = default;
ItscamRestClient::SoftwareUpdateOperation::SoftwareUpdateOperation(
    SoftwareUpdateOperation&&) noexcept = default;
ItscamRestClient::SoftwareUpdateOperation&
ItscamRestClient::SoftwareUpdateOperation::operator=(
    SoftwareUpdateOperation&&) noexcept = default;

ItscamRestClient::SoftwareUpdateOperation::SoftwareUpdateOperation(
    std::shared_ptr<Impl> impl)
    : mImpl(std::move(impl)) {}

ItscamRestClient::SoftwareUpdateStatus
ItscamRestClient::SoftwareUpdateOperation::status() const {
    if (!mImpl) return SoftwareUpdateStatus{};
    return mImpl->snapshot();
}

void ItscamRestClient::SoftwareUpdateOperation::setCallback(
    SoftwareUpdateStatusCallback callback) {
    if (!mImpl) return;
    mImpl->setCallback(std::move(callback));
}

Result<ItscamRestClient::SoftwareUpdateStatus>
ItscamRestClient::SoftwareUpdateOperation::wait(uint32_t timeoutMs) {
    if (!mImpl) {
        return Error{Error::InvalidParameter, "invalid software update operation"};
    }
    return mImpl->wait(timeoutMs);
}

bool ItscamRestClient::SoftwareUpdateOperation::isComplete() const {
    return mImpl && mImpl->isDone();
}

void ItscamRestClient::SoftwareUpdateOperation::cancel() {
    if (mImpl) mImpl->cancel();
}

//=========================================================================
// Connection
//=========================================================================

void ItscamRestClient::setBaseUrl(const std::string& host, uint16_t port,
                                  const std::string& scheme) {
    mImpl->transport.setBaseUrl(host, port, scheme);
}

//=========================================================================
// TLS
//=========================================================================

void ItscamRestClient::setCaCertFile(const std::string& pemPath) {
    mImpl->transport.setCaCertFile(pemPath);
}

void ItscamRestClient::setCaCertData(const std::string& pem) {
    mImpl->transport.setCaCertData(pem);
}

void ItscamRestClient::setVerifyServerCertificate(bool verify) {
    mImpl->transport.setVerifyServerCertificate(verify);
}

void ItscamRestClient::setClientCertificate(const std::string& certPem,
                                            const std::string& keyPem) {
    mImpl->transport.setClientCertificate(certPem, keyPem);
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
            mImpl->transport.setBearerToken(
                body["token"].get<std::string>());
            mImpl->transport.log(LogLevel::Info,
                                 "Authenticated as '" + username + "'");
        }
    }
    return result;
}

void ItscamRestClient::setAuthToken(const std::string& token) {
    mImpl->transport.setBearerToken(token);
}

void ItscamRestClient::clearAuthToken() {
    mImpl->transport.clearBearerToken();
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

Result<rt::ProfileConfig> ItscamRestClient::updateProfiles(
    const std::vector<rt::ProfileConfig>& profiles, uint32_t timeoutMs) {
    json arr = json::array();
    for (auto const& p : profiles) arr.push_back(rt::to_partial_json(p));
    return mapTyped<rt::ProfileConfig>(
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
// Software update
//=========================================================================

Result<json> ItscamRestClient::uploadSoftwareArchive(
    const std::string& swuPath,
    uint32_t timeoutMs,
    UploadProgressCallback progress) {
    return mImpl->doPostMultipartFile(mImpl->apiPrefix + "/swupdate/upload",
                                      "file",
                                      swuPath,
                                      "application/octet-stream",
                                      timeoutMs,
                                      std::move(progress));
}

Result<json> ItscamRestClient::restartSoftwareUpdate(uint32_t timeoutMs) {
    return mImpl->doPost(mImpl->apiPrefix + "/swupdate/restart",
                         json::object(), timeoutMs);
}

Result<ItscamRestClient::SoftwareUpdateOperation>
ItscamRestClient::startSoftwareUpdate(
    const SoftwareUpdateOptions& options,
    SoftwareUpdateStatusCallback callback) {
    if (options.swuPath.empty()) {
        return Error{Error::InvalidParameter, "SWU archive path is required"};
    }
    if (!mImpl->transport.configured()) {
        return Error{Error::InvalidParameter, "REST client base URL is not configured"};
    }

    auto impl = std::make_shared<SoftwareUpdateOperation::Impl>(
        mImpl->makeOperationContext(), options, std::move(callback));
    impl->start();
    return SoftwareUpdateOperation(std::move(impl));
}

Result<ItscamRestClient::SoftwareUpdateStatus>
ItscamRestClient::updateSoftware(
    const SoftwareUpdateOptions& options,
    SoftwareUpdateStatusCallback callback) {
    auto operation = startSoftwareUpdate(options, std::move(callback));
    if (!operation) return operation.error();
    return operation.value().wait();
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
    mImpl->transport.setLogHandler(std::move(cb));
}

} // namespace itscam
