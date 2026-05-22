/*
 *  itscam_rest_client.cpp
 *
 *  ITSCAM Client SDK - REST API client implementation
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  HTTP client for the ITSCAM webapp backend REST API.
 *  Uses cpp-httplib (header-only) for HTTP transport.
 *  Requires: C++14, nlohmann/json, cpp-httplib.
 */

// ============================================================================
//  Platform configuration (must be before any includes)
// ============================================================================

#if defined(_WIN32) || defined(__MINGW32__)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    // Require Windows 10 or later for httplib compatibility
    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
    #endif
#endif

// ============================================================================
//  Includes
// ============================================================================

#include "itscam_rest_client.h"
#include "itscam_os.h"

#include <stdarg.h>

// cpp-httplib -- HTTP only (no TLS)
#include "3rdparty/httplib.h"

namespace itscam {

using json = nlohmann::json;
using Mutex = itscam_os::Mutex;
template<typename M> using LockGuard = itscam_os::LockGuard<M>;

// ============================================================================
//  Impl
// ============================================================================

struct ItscamRestClient::Impl {

    // --- State ---------------------------------------------------------------

    std::string host;
    uint16_t    port      = 80;
    bool        configured = false;

    std::string apiPrefix = "/api";
    std::string authToken;

    std::function<void(LogLevel, const std::string&)> logHandler;
    Mutex  logMtx;

    // --- Logging helpers -----------------------------------------------------

    void logInfo(const char* fmt, ...) {
        std::lock_guard<Mutex> lk(logMtx);
        if (!logHandler) return;
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        logHandler(LogLevel::Info, buf);
    }

    void logError(const char* fmt, ...) {
        std::lock_guard<Mutex> lk(logMtx);
        if (!logHandler) return;
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        logHandler(LogLevel::Error, buf);
    }

    // --- HTTP client factory -------------------------------------------------

    /// Create a short-lived httplib::Client configured with the stored
    /// host/port and the current auth token.
    std::unique_ptr<httplib::Client> makeClient(uint32_t timeoutMs) {
        auto cli = std::unique_ptr<httplib::Client>(
            new httplib::Client(host, port));

        cli->set_connection_timeout(std::chrono::milliseconds(timeoutMs));
        cli->set_read_timeout(std::chrono::milliseconds(timeoutMs));
        cli->set_write_timeout(std::chrono::milliseconds(timeoutMs));

        if (!authToken.empty()) {
            cli->set_bearer_token_auth(authToken);
        }

        return cli;
    }

    // --- Response processing -------------------------------------------------

    /// Map an httplib result to Result<json>.
    Result<json> processResponse(httplib::Result& res,
                                 const std::string& method,
                                 const std::string& path) {
        if (!res) {
            auto err = res.error();
            if (err == httplib::Error::ConnectionTimeout ||
                err == httplib::Error::Read) {
                logError("%s %s: timeout", method.c_str(), path.c_str());
                return Error{Error::Timeout,
                             method + " " + path + ": request timed out"};
            }
            logError("%s %s: connection failed (httplib error %d)",
                     method.c_str(), path.c_str(), static_cast<int>(err));
            return Error{Error::ConnectionFailed,
                         method + " " + path + ": connection failed"};
        }

        int status = res->status;

        logInfo("%s %s -> %d", method.c_str(), path.c_str(), status);

        // Parse response body as JSON (even for error responses, the backend
        // may include a JSON body with a message field).
        json body;
        if (!res->body.empty()) {
            try {
                body = json::parse(res->body);
            } catch (const json::parse_error&) {
                // Not JSON -- wrap raw body in a JSON string.
                body = res->body;
            }
        }

        // Map HTTP status to SDK error codes
        if (status == 401) {
            std::string msg = "not authenticated";
            if (body.is_object() && body.contains("message"))
                msg = body["message"].get<std::string>();
            logError("%s %s: 401 %s", method.c_str(), path.c_str(),
                     msg.c_str());
            return Error{Error::NotAuthenticated, msg};
        }

        if (status == 400 || status == 422) {
            std::string msg = "invalid parameter";
            if (body.is_object() && body.contains("message"))
                msg = body["message"].get<std::string>();
            logError("%s %s: %d %s", method.c_str(), path.c_str(), status,
                     msg.c_str());
            return Error{Error::InvalidParameter, msg};
        }

        if (status == 503) {
            logError("%s %s: 503 service unavailable", method.c_str(),
                     path.c_str());
            return Error{Error::ConnectionFailed,
                         "backend service unavailable (daemon offline?)"};
        }

        if (status >= 500) {
            std::string msg = "server error (" + std::to_string(status) + ")";
            if (body.is_object() && body.contains("message"))
                msg = body["message"].get<std::string>();
            logError("%s %s: %d %s", method.c_str(), path.c_str(), status,
                     msg.c_str());
            return Error{Error::ServerError, msg};
        }

        if (status >= 300) {
            logError("%s %s: unexpected status %d", method.c_str(),
                     path.c_str(), status);
            return Error{Error::Unknown,
                         "unexpected HTTP " + std::to_string(status)};
        }

        // 2xx -- success
        return body;
    }

    // --- Core HTTP verbs -----------------------------------------------------

    Result<json> doGet(const std::string& path, uint32_t timeoutMs) {
        if (!configured) {
            return Error{Error::ConnectionFailed,
                         "base URL not configured -- call setBaseUrl() first"};
        }
        auto cli = makeClient(timeoutMs);
        auto res = cli->Get(path);
        return processResponse(res, "GET", path);
    }

    Result<json> doPut(const std::string& path, const json& body,
                       uint32_t timeoutMs) {
        if (!configured) {
            return Error{Error::ConnectionFailed,
                         "base URL not configured -- call setBaseUrl() first"};
        }
        auto cli = makeClient(timeoutMs);
        auto res = cli->Put(path, body.dump(), "application/json");
        return processResponse(res, "PUT", path);
    }

    Result<json> doPost(const std::string& path, const json& body,
                        uint32_t timeoutMs) {
        if (!configured) {
            return Error{Error::ConnectionFailed,
                         "base URL not configured -- call setBaseUrl() first"};
        }
        auto cli = makeClient(timeoutMs);
        auto res = cli->Post(path, body.dump(), "application/json");
        return processResponse(res, "POST", path);
    }

    Result<json> doDelete(const std::string& path, uint32_t timeoutMs) {
        if (!configured) {
            return Error{Error::ConnectionFailed,
                         "base URL not configured -- call setBaseUrl() first"};
        }
        auto cli = makeClient(timeoutMs);
        auto res = cli->Delete(path);
        return processResponse(res, "DELETE", path);
    }
};

// ============================================================================
//  Constructor / destructor / move
// ============================================================================

ItscamRestClient::ItscamRestClient()
    : mImpl(new Impl()) {}

ItscamRestClient::~ItscamRestClient() = default;

ItscamRestClient::ItscamRestClient(ItscamRestClient&&) noexcept = default;
ItscamRestClient& ItscamRestClient::operator=(ItscamRestClient&&) noexcept
    = default;

// ============================================================================
//  Connection
// ============================================================================

void ItscamRestClient::setBaseUrl(const std::string& host, uint16_t port) {
    mImpl->host       = host;
    mImpl->port       = port;
    mImpl->configured = true;
    mImpl->logInfo("Base URL set to %s:%u", host.c_str(),
                   static_cast<unsigned>(port));
}

// ============================================================================
//  Authentication
// ============================================================================

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
        // Extract and store the token from the response
        auto& body = result.value();
        if (body.is_object() && body.contains("token")) {
            mImpl->authToken = body["token"].get<std::string>();
            mImpl->logInfo("Authenticated as '%s'", username.c_str());
        }
    }
    return result;
}

void ItscamRestClient::setAuthToken(const std::string& token) {
    mImpl->authToken = token;
}

void ItscamRestClient::clearAuthToken() {
    mImpl->authToken.clear();
}

// ============================================================================
//  Image profiles
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getProfiles(uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/image/profiles", timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::getProfile(int id,
                                                    uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/image/profiles?id=" +
                       std::to_string(id);
    return mImpl->doGet(path, timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::createProfile(
    const nlohmann::json& profile, uint32_t timeoutMs) {
    return mImpl->doPost(mImpl->apiPrefix + "/image/profiles", profile,
                         timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::updateProfile(
    const nlohmann::json& profile, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/image/profiles", profile,
                        timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::deleteProfile(int id,
                                                       uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/image/profiles?id=" +
                       std::to_string(id);
    return mImpl->doDelete(path, timeoutMs);
}

// ============================================================================
//  Equipment volatile info
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getVolatileInfo(
    uint32_t timeoutMs) {
    return mImpl->doGet(
        mImpl->apiPrefix + "/equipment/misc/readonly/volatile", timeoutMs);
}

// ============================================================================
//  Equipment general
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getGeneralConfig(
    uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/equipment/general", timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::setGeneralConfig(
    const nlohmann::json& config, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/equipment/general", config,
                        timeoutMs);
}

// ============================================================================
//  Analytics
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getAnalyticsConfig(
    uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/equipment/analytics", timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::setAnalyticsConfig(
    const nlohmann::json& config, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/equipment/analytics", config,
                        timeoutMs);
}

// ============================================================================
//  OCR
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getOcrConfig(uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/equipment/ocr", timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::setOcrConfig(
    const nlohmann::json& config, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/equipment/ocr", config,
                        timeoutMs);
}

// ============================================================================
//  Classifier
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getClassifierConfig(
    uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/equipment/classifier",
                        timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::setClassifierConfig(
    const nlohmann::json& config, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/equipment/classifier", config,
                        timeoutMs);
}

// ============================================================================
//  Lanes
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getLanesConfig(uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/equipment/lanes", timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::setLanesConfig(
    const nlohmann::json& config, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/equipment/lanes", config,
                        timeoutMs);
}

// ============================================================================
//  ITSCAM PRO
// ============================================================================

Result<nlohmann::json> ItscamRestClient::getItscamproConfig(
    uint32_t timeoutMs) {
    return mImpl->doGet(mImpl->apiPrefix + "/equipment/servers/itscampro",
                        timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::setItscamproConfig(
    const nlohmann::json& config, uint32_t timeoutMs) {
    return mImpl->doPut(mImpl->apiPrefix + "/equipment/servers/itscampro",
                        config, timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::getItscamproStatus(
    uint32_t timeoutMs) {
    return mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/itscampro/status", timeoutMs);
}

// ============================================================================
//  Generic HTTP methods
// ============================================================================

Result<nlohmann::json> ItscamRestClient::httpGet(const std::string& path,
                                                 uint32_t timeoutMs) {
    return mImpl->doGet(path, timeoutMs);
}

Result<nlohmann::json> ItscamRestClient::httpPut(const std::string& path,
                                                 const nlohmann::json& body,
                                                 uint32_t timeoutMs) {
    return mImpl->doPut(path, body, timeoutMs);
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

// ============================================================================
//  Settings
// ============================================================================

void ItscamRestClient::setApiPrefix(const std::string& prefix) {
    mImpl->apiPrefix = prefix;
}

void ItscamRestClient::setLogHandler(
    std::function<void(LogLevel, const std::string&)> cb) {
    std::lock_guard<Mutex> lk(mImpl->logMtx);
    mImpl->logHandler = std::move(cb);
}

} // namespace itscam
