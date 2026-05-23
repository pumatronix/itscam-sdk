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

#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace itscam {

using json      = nlohmann::json;
using Transport = detail::HttpTransport;
namespace rt    = pumatronix::itscam;

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

}  // namespace

// ============================================================================
//  Impl
// ============================================================================

struct ItscamRestClient::Impl {

    Transport   transport;
    std::string apiPrefix = "/api";

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

    Result<json> doDelete(const std::string& path, uint32_t timeoutMs) {
        detail::HttpRequest req;
        req.method = "DELETE";
        req.path   = path;
        return mapResponse(transport.request(req, timeoutMs), "DELETE", path);
    }
};

// ============================================================================
//  Constructor / destructor / move
// ============================================================================

ItscamRestClient::ItscamRestClient() : mImpl(new Impl()) {}
ItscamRestClient::~ItscamRestClient() = default;

ItscamRestClient::ItscamRestClient(ItscamRestClient&&) noexcept = default;
ItscamRestClient& ItscamRestClient::operator=(ItscamRestClient&&) noexcept
    = default;

// ============================================================================
//  Connection
// ============================================================================

void ItscamRestClient::setBaseUrl(const std::string& host, uint16_t port,
                                  const std::string& scheme) {
    mImpl->transport.setBaseUrl(host, port, scheme);
}

// ============================================================================
//  TLS
// ============================================================================

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

// ============================================================================
//  Image profiles
// ============================================================================

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

Result<rt::ProfileConfig> ItscamRestClient::updateProfileById(
    int id, const rt::ProfileConfig& profile, uint32_t timeoutMs) {
    std::string path =
        mImpl->apiPrefix + "/image/profiles/" + std::to_string(id);
    return mapTyped<rt::ProfileConfig>(
        mImpl->doPut(path, json(profile), timeoutMs));
}

Result<rt::ProfileConfig> ItscamRestClient::updateProfiles(
    const std::vector<rt::ProfileConfig>& profiles, uint32_t timeoutMs) {
    return mapTyped<rt::ProfileConfig>(
        mImpl->doPut(mImpl->apiPrefix + "/image/profiles",
                     json(profiles), timeoutMs));
}

Result<nlohmann::json> ItscamRestClient::deleteProfile(int id,
                                                       uint32_t timeoutMs) {
    std::string path =
        mImpl->apiPrefix + "/image/profiles?id=" + std::to_string(id);
    return mImpl->doDelete(path, timeoutMs);
}

// ============================================================================
//  Equipment volatile info
// ============================================================================

Result<rt::MiscVolatile> ItscamRestClient::getVolatileInfo(
    uint32_t timeoutMs) {
    return mapTyped<rt::MiscVolatile>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/misc/readonly/volatile", timeoutMs));
}

// ============================================================================
//  Equipment general (untyped until Phase 2)
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

Result<rt::AnalyticsConfig> ItscamRestClient::getAnalyticsConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::AnalyticsConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/analytics", timeoutMs));
}

Result<rt::AnalyticsConfig> ItscamRestClient::setAnalyticsConfig(
    const rt::AnalyticsConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::AnalyticsConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/analytics", json(config), timeoutMs));
}

// ============================================================================
//  OCR
// ============================================================================

Result<rt::OcrConfig> ItscamRestClient::getOcrConfig(uint32_t timeoutMs) {
    return mapTyped<rt::OcrConfig>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/ocr", timeoutMs));
}

Result<rt::OcrConfig> ItscamRestClient::setOcrConfig(
    const rt::OcrConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::OcrConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/ocr", json(config), timeoutMs));
}

// ============================================================================
//  Classifier
// ============================================================================

Result<rt::ClassifierConfig> ItscamRestClient::getClassifierConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ClassifierConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/classifier", timeoutMs));
}

Result<rt::ClassifierConfig> ItscamRestClient::setClassifierConfig(
    const rt::ClassifierConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::ClassifierConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/classifier", json(config), timeoutMs));
}

// ============================================================================
//  Lanes
// ============================================================================

Result<rt::LanesConfig> ItscamRestClient::getLanesConfig(uint32_t timeoutMs) {
    return mapTyped<rt::LanesConfig>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/lanes", timeoutMs));
}

Result<rt::LanesConfig> ItscamRestClient::setLanesConfig(
    const rt::LanesConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::LanesConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/lanes", json(config), timeoutMs));
}

// ============================================================================
//  ITSCAM PRO
// ============================================================================

Result<rt::ItscamproConfig> ItscamRestClient::getItscamproConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ItscamproConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/itscampro", timeoutMs));
}

Result<rt::ItscamproConfig> ItscamRestClient::setItscamproConfig(
    const rt::ItscamproConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::ItscamproConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/itscampro", json(config),
        timeoutMs));
}

Result<rt::ItscamproStatus> ItscamRestClient::getItscamproStatus(
    uint32_t timeoutMs) {
    return mapTyped<rt::ItscamproStatus>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/itscampro/status", timeoutMs));
}

// ============================================================================
//  AutoFocus
// ============================================================================

Result<rt::AutoFocus> ItscamRestClient::getAutoFocus(uint32_t timeoutMs) {
    return mapTyped<rt::AutoFocus>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/autofocus", timeoutMs));
}

Result<rt::AutoFocus> ItscamRestClient::setAutoFocus(
    const rt::AutoFocus& config, uint32_t timeoutMs) {
    return mapTyped<rt::AutoFocus>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/autofocus", json(config), timeoutMs));
}

// ============================================================================
//  Stream config
// ============================================================================

Result<rt::StreamConfig> ItscamRestClient::getStreamConfig(uint32_t timeoutMs) {
    return mapTyped<rt::StreamConfig>(
        mImpl->doGet(mImpl->apiPrefix + "/video/streams", timeoutMs));
}

Result<rt::StreamConfig> ItscamRestClient::setStreamConfig(
    const rt::StreamConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::StreamConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/video/streams", json(config), timeoutMs));
}

// ============================================================================
//  Misc
// ============================================================================

Result<rt::Misc> ItscamRestClient::getMisc(uint32_t timeoutMs) {
    return mapTyped<rt::Misc>(
        mImpl->doGet(mImpl->apiPrefix + "/equipment/misc", timeoutMs));
}

Result<rt::Misc> ItscamRestClient::setMisc(const rt::Misc& config,
                                           uint32_t timeoutMs) {
    return mapTyped<rt::Misc>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/misc", json(config), timeoutMs));
}

// ============================================================================
//  Image sign
// ============================================================================

Result<rt::ImageSignConfig> ItscamRestClient::getImageSignConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ImageSignConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/imageSign", timeoutMs));
}

// ============================================================================
//  FTP
// ============================================================================

Result<rt::FtpConfig> ItscamRestClient::getFtpConfig(uint32_t timeoutMs) {
    return mapTyped<rt::FtpConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/ftp", timeoutMs));
}

Result<rt::FtpConfig> ItscamRestClient::setFtpConfig(
    const rt::FtpConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::FtpConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/ftp", json(config), timeoutMs));
}

// ============================================================================
//  Lince
// ============================================================================

Result<rt::LinceConfig> ItscamRestClient::getLinceConfig(uint32_t timeoutMs) {
    return mapTyped<rt::LinceConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/lince", timeoutMs));
}

Result<rt::LinceConfig> ItscamRestClient::setLinceConfig(
    const rt::LinceConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::LinceConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/lince", json(config),
        timeoutMs));
}

Result<rt::LinceStatus> ItscamRestClient::getLinceStatus(uint32_t timeoutMs) {
    return mapTyped<rt::LinceStatus>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/lince/status", timeoutMs));
}

// ============================================================================
//  Vehicle indicator
// ============================================================================

Result<rt::VehicleIndicatorConfig>
ItscamRestClient::getVehicleIndicatorConfig(uint32_t timeoutMs) {
    return mapTyped<rt::VehicleIndicatorConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/vehicleIndicator", timeoutMs));
}

Result<rt::VehicleIndicatorConfig>
ItscamRestClient::setVehicleIndicatorConfig(
    const rt::VehicleIndicatorConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::VehicleIndicatorConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/vehicleIndicator", json(config),
        timeoutMs));
}

// ============================================================================
//  Protocols
// ============================================================================

Result<rt::ProtocolsConfig> ItscamRestClient::getProtocolsConfig(
    uint32_t timeoutMs) {
    return mapTyped<rt::ProtocolsConfig>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/servers/protocols", timeoutMs));
}

Result<rt::ProtocolsConfig> ItscamRestClient::setProtocolsConfig(
    const rt::ProtocolsConfig& config, uint32_t timeoutMs) {
    return mapTyped<rt::ProtocolsConfig>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/servers/protocols", json(config),
        timeoutMs));
}

// ============================================================================
//  Profile transitioner
// ============================================================================

Result<rt::ProfileTransitioner> ItscamRestClient::getProfileTransitioner(
    uint32_t timeoutMs) {
    return mapTyped<rt::ProfileTransitioner>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/transitioner", timeoutMs));
}

Result<rt::ProfileTransitioner> ItscamRestClient::setProfileTransitioner(
    const rt::ProfileTransitioner& config, uint32_t timeoutMs) {
    return mapTyped<rt::ProfileTransitioner>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/transitioner", json(config),
        timeoutMs));
}

// ============================================================================
//  I/O ports
// ============================================================================

Result<std::vector<rt::IoConfig>> ItscamRestClient::getIoPorts(
    uint32_t timeoutMs) {
    return mapTyped<std::vector<rt::IoConfig>>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/ioPorts", timeoutMs));
}

Result<std::vector<rt::IoConfig>> ItscamRestClient::setIoPorts(
    const std::vector<rt::IoConfig>& ports, uint32_t timeoutMs) {
    return mapTyped<std::vector<rt::IoConfig>>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/ioPorts", json(ports), timeoutMs));
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
    return mapTyped<rt::IoConfig>(mImpl->doPut(path, json(port), timeoutMs));
}

Result<std::vector<rt::IoBasic>> ItscamRestClient::getIoBasic(
    uint32_t timeoutMs) {
    return mapTyped<std::vector<rt::IoBasic>>(mImpl->doGet(
        mImpl->apiPrefix + "/equipment/ioBasic", timeoutMs));
}

Result<std::vector<rt::IoBasic>> ItscamRestClient::setIoBasic(
    const std::vector<rt::IoBasic>& ports, uint32_t timeoutMs) {
    return mapTyped<std::vector<rt::IoBasic>>(mImpl->doPut(
        mImpl->apiPrefix + "/equipment/ioBasic", json(ports), timeoutMs));
}

// ============================================================================
//  REST API client (webhook) servers
// ============================================================================

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
        mImpl->doPut(path, json(config), timeoutMs));
}

Result<rt::RestApiClientStatus> ItscamRestClient::getRestApiClientStatus(
    int id, uint32_t timeoutMs) {
    std::string path = mImpl->apiPrefix + "/equipment/servers/restapiclient/"
                       + std::to_string(id) + "/status";
    return mapTyped<rt::RestApiClientStatus>(mImpl->doGet(path, timeoutMs));
}

// ============================================================================
//  Licenses
// ============================================================================

Result<rt::Licenses> ItscamRestClient::getLicenses(uint32_t timeoutMs) {
    return mapTyped<rt::Licenses>(
        mImpl->doGet(mImpl->apiPrefix + "/system/licenses", timeoutMs));
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

// ============================================================================
//  Settings
// ============================================================================

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
