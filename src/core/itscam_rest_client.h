/*
 *  itscam_rest_client.h
 *
 *  ITSCAM Client SDK - REST API client
 *
 *  HTTP/JSON client for the ITSCAM webapp backend REST API.
 *  Complements ItscamClient (binary TCP) with standard HTTP access
 *  to configuration and status endpoints.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Requires: nlohmann/json 3.x, cpp-httplib 0.18+ (header-only).
 *
 *  Typed REST helpers (e.g. `getOcrConfig() -> Result<OcrConfig>`) are
 *  generated from the camera's OpenAPI document into `itscam_rest_types.hpp`.
 *  See [`tools/codegen/`](../../tools/codegen/) for the refresh / regeneration
 *  workflow.
 *
 *  **Partial PUT:** the ITSCAM daemon merges PUT bodies into the existing
 *  configuration.  Send only the fields you want to change via
 *  `patchJson()` / `httpPut()`.  Typed setters and `updateProfileById()`
 *  serialise the full object and are rejected with HTTP 500 on several
 *  endpoints (notably `PUT /api/image/profiles/{id}`).
 */
#pragma once

#include "itscam_types.h"
#include "itscam_rest_types.hpp"

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#include "3rdparty/nlohmann/json.hpp"
#endif

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace itscam {

class ItscamRestClient {
public:
    ItscamRestClient();
    ~ItscamRestClient();

    // Move-only (PIMPL)
    ItscamRestClient(ItscamRestClient&&) noexcept;
    ItscamRestClient& operator=(ItscamRestClient&&) noexcept;
    ItscamRestClient(const ItscamRestClient&) = delete;
    ItscamRestClient& operator=(const ItscamRestClient&) = delete;

    // =========================================================================
    //  Connection
    // =========================================================================

    /// Configure the target host, port and scheme for the webapp backend.
    /// Must be called before any HTTP method.  Does not open a persistent
    /// connection -- each request uses a short-lived HTTP transaction.
    ///
    /// @param scheme  "http" (default) or "https".  When "https" the SDK uses
    ///                the statically-linked mbedTLS backend; see the TLS
    ///                setters below for certificate configuration.  Passing
    ///                port = 0 selects the protocol default (80 for http,
    ///                443 for https).
    void setBaseUrl(const std::string& host,
                    uint16_t port = 80,
                    const std::string& scheme = "http");

    // =========================================================================
    //  TLS (HTTPS) configuration
    //  All setters are no-ops when scheme is "http".  Defaults are safe:
    //  server certificate verification is enabled, and the OS-supplied CA
    //  bundle is used when found.
    // =========================================================================

    /// Path to a PEM file containing trusted CA certificates.
    void setCaCertFile(const std::string& pemPath);

    /// PEM-encoded CA certificate bundle as a string (alternative to
    /// setCaCertFile -- the last call wins).
    void setCaCertData(const std::string& pem);

    /// Enable / disable server certificate verification (default: enabled).
    /// Disabling this is convenient for self-signed cameras on a trusted
    /// LAN, but exposes the connection to MITM attacks.
    void setVerifyServerCertificate(bool verify);

    /// Configure a client certificate and private key for mutual TLS.
    /// Both arguments must be PEM-encoded.
    void setClientCertificate(const std::string& certPem,
                              const std::string& keyPem);

    // =========================================================================
    //  Authentication  (JWT)
    // =========================================================================

    /// POST /api/auth -- obtain a JWT token from the webapp backend.
    /// On success the token is stored internally and attached to every
    /// subsequent request as "Authorization: Bearer <token>".
    /// The response JSON contains id, username, isAdmin, lastLoggedIn, token.
    Result<nlohmann::json> login(const std::string& username,
                                 const std::string& password,
                                 uint32_t timeoutMs = 10000);

    /// Manually set a pre-existing JWT token (e.g. obtained externally).
    void setAuthToken(const std::string& token);

    /// Clear the stored JWT token.
    void clearAuthToken();

    // =========================================================================
    //  Image profiles                                    Endpoint: /image/profiles
    //
    //  ProfileConfig and friends live in itscam_rest_types.hpp (auto-generated).
    // =========================================================================

    /// GET /api/image/profiles -- list all profiles.
    Result<std::vector<pumatronix::itscam::ProfileConfig>> getProfiles(
        uint32_t timeoutMs = 10000);

    /// GET /api/image/profiles?id=<id> -- filter profiles by id.
    /// Returns an array because the server may return more than one entry
    /// when the id is ambiguous (e.g. a NaN response).
    Result<std::vector<pumatronix::itscam::ProfileConfig>> getProfile(
        int id, uint32_t timeoutMs = 10000);

    /// POST /api/image/profiles -- create a new profile.
    Result<pumatronix::itscam::ProfileConfig> createProfile(
        const pumatronix::itscam::ProfileConfig& profile,
        uint32_t timeoutMs = 10000);

    /// PUT /api/image/profiles/{id} -- update a single profile by id.
    ///
    /// Sends a full `ProfileConfig` document.  The daemon expects a
    /// **partial** body on this endpoint; round-tripping a GET response
    /// (or any complete profile object) returns HTTP 500.  To change a
    /// subset of fields use `patchJson("/api/image/profiles/{id}", patch)`.
    Result<pumatronix::itscam::ProfileConfig> updateProfileById(
        int id,
        const pumatronix::itscam::ProfileConfig& profile,
        uint32_t timeoutMs = 10000);

    /// PUT /api/image/profiles -- bulk update.
    ///
    /// Sends a JSON array of full profiles.  Same partial-PUT caveat as
    /// `updateProfileById()` applies; prefer `patchJson()` for subset changes.
    Result<pumatronix::itscam::ProfileConfig> updateProfiles(
        const std::vector<pumatronix::itscam::ProfileConfig>& profiles,
        uint32_t timeoutMs = 10000);

    /// DELETE /api/image/profiles?id=<id> -- delete a profile by id.
    /// Returns the server's raw response body (per-id status objects).
    Result<nlohmann::json> deleteProfile(int id, uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Equipment volatile info  (read-only)
    //  Endpoint: /api/equipment/misc/readonly/volatile
    // =========================================================================

    /// GET /api/equipment/misc/readonly/volatile
    Result<pumatronix::itscam::MiscVolatile> getVolatileInfo(
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Equipment general configuration
    //  Endpoint: /api/equipment/general
    //
    //  GeneralData is not part of Phase 1; these methods still surface raw
    //  JSON.  They will be promoted to typed once GeneralData is added.
    // =========================================================================

    /// GET /api/equipment/general
    Result<nlohmann::json> getGeneralConfig(uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/general
    Result<nlohmann::json> setGeneralConfig(const nlohmann::json& config,
                                            uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Analytics configuration
    //  Endpoint: /api/equipment/analytics
    // =========================================================================

    /// GET /api/equipment/analytics
    Result<pumatronix::itscam::AnalyticsConfig> getAnalyticsConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/analytics
    Result<pumatronix::itscam::AnalyticsConfig> setAnalyticsConfig(
        const pumatronix::itscam::AnalyticsConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  OCR configuration
    //  Endpoint: /api/equipment/ocr
    // =========================================================================

    /// GET /api/equipment/ocr
    Result<pumatronix::itscam::OcrConfig> getOcrConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/ocr
    Result<pumatronix::itscam::OcrConfig> setOcrConfig(
        const pumatronix::itscam::OcrConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Classifier configuration
    //  Endpoint: /api/equipment/classifier
    // =========================================================================

    /// GET /api/equipment/classifier
    Result<pumatronix::itscam::ClassifierConfig> getClassifierConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/classifier
    Result<pumatronix::itscam::ClassifierConfig> setClassifierConfig(
        const pumatronix::itscam::ClassifierConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Lanes configuration
    //  Endpoint: /api/equipment/lanes
    //
    //  LanesConfig is not part of Phase 1; raw-JSON methods remain in place.
    // =========================================================================

    /// GET /api/equipment/lanes
    Result<pumatronix::itscam::LanesConfig> getLanesConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/lanes
    Result<pumatronix::itscam::LanesConfig> setLanesConfig(
        const pumatronix::itscam::LanesConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  ITSCAM PRO server configuration
    //  Endpoint: /api/equipment/servers/itscampro
    // =========================================================================

    /// GET /api/equipment/servers/itscampro
    Result<pumatronix::itscam::ItscamproConfig> getItscamproConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/servers/itscampro
    Result<pumatronix::itscam::ItscamproConfig> setItscamproConfig(
        const pumatronix::itscam::ItscamproConfig& config,
        uint32_t timeoutMs = 10000);

    /// GET /api/equipment/servers/itscampro/status
    Result<pumatronix::itscam::ItscamproStatus> getItscamproStatus(
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Auto-focus configuration
    //  Endpoint: /api/equipment/autofocus
    // =========================================================================

    /// GET /api/equipment/autofocus
    Result<pumatronix::itscam::AutoFocus> getAutoFocus(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/autofocus
    Result<pumatronix::itscam::AutoFocus> setAutoFocus(
        const pumatronix::itscam::AutoFocus& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Video stream configuration
    //  Endpoint: /api/video/streams
    // =========================================================================

    /// GET /api/video/streams
    Result<pumatronix::itscam::StreamConfig> getStreamConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/video/streams
    Result<pumatronix::itscam::StreamConfig> setStreamConfig(
        const pumatronix::itscam::StreamConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Equipment Misc configuration
    //  Endpoint: /api/equipment/misc
    // =========================================================================

    /// GET /api/equipment/misc
    Result<pumatronix::itscam::Misc> getMisc(uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/misc
    Result<pumatronix::itscam::Misc> setMisc(
        const pumatronix::itscam::Misc& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Image-sign (frame watermarking)
    //  Endpoint: /api/equipment/imageSign
    // =========================================================================

    /// GET /api/equipment/imageSign  (the endpoint is read-only on most cameras)
    Result<pumatronix::itscam::ImageSignConfig> getImageSignConfig(
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  FTP upload server
    //  Endpoint: /api/equipment/servers/ftp
    // =========================================================================

    /// GET /api/equipment/servers/ftp
    Result<pumatronix::itscam::FtpConfig> getFtpConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/servers/ftp
    Result<pumatronix::itscam::FtpConfig> setFtpConfig(
        const pumatronix::itscam::FtpConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Lince integration
    //  Endpoint: /api/equipment/servers/lince
    // =========================================================================

    /// GET /api/equipment/servers/lince
    Result<pumatronix::itscam::LinceConfig> getLinceConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/servers/lince
    Result<pumatronix::itscam::LinceConfig> setLinceConfig(
        const pumatronix::itscam::LinceConfig& config,
        uint32_t timeoutMs = 10000);

    /// GET /api/equipment/servers/lince/status
    Result<pumatronix::itscam::LinceStatus> getLinceStatus(
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Vehicle indicator
    //  Endpoint: /api/equipment/vehicleIndicator
    // =========================================================================

    /// GET /api/equipment/vehicleIndicator
    Result<pumatronix::itscam::VehicleIndicatorConfig> getVehicleIndicatorConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/vehicleIndicator
    Result<pumatronix::itscam::VehicleIndicatorConfig> setVehicleIndicatorConfig(
        const pumatronix::itscam::VehicleIndicatorConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Output protocols (Cougar / ITSCAM proprietary / auth / CGI flags)
    //  Endpoint: /api/equipment/servers/protocols
    // =========================================================================

    /// GET /api/equipment/servers/protocols
    Result<pumatronix::itscam::ProtocolsConfig> getProtocolsConfig(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/servers/protocols
    Result<pumatronix::itscam::ProtocolsConfig> setProtocolsConfig(
        const pumatronix::itscam::ProtocolsConfig& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Profile transitioner  (day/night switching policy)
    //  Endpoint: /api/equipment/transitioner
    // =========================================================================

    /// GET /api/equipment/transitioner
    Result<pumatronix::itscam::ProfileTransitioner> getProfileTransitioner(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/transitioner
    Result<pumatronix::itscam::ProfileTransitioner> setProfileTransitioner(
        const pumatronix::itscam::ProfileTransitioner& config,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  I/O ports
    //  Endpoint: /api/equipment/ioPorts(/{id})
    //  Endpoint: /api/equipment/ioBasic       (lightweight pin metadata array)
    // =========================================================================

    /// GET /api/equipment/ioPorts (returns one IoConfig per pin).
    Result<std::vector<pumatronix::itscam::IoConfig>> getIoPorts(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/ioPorts (bulk update of all pins).
    Result<std::vector<pumatronix::itscam::IoConfig>> setIoPorts(
        const std::vector<pumatronix::itscam::IoConfig>& ports,
        uint32_t timeoutMs = 10000);

    /// GET /api/equipment/ioPorts/{id}
    Result<pumatronix::itscam::IoConfig> getIoPort(int id,
                                                   uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/ioPorts/{id}
    Result<pumatronix::itscam::IoConfig> setIoPort(
        int id,
        const pumatronix::itscam::IoConfig& port,
        uint32_t timeoutMs = 10000);

    /// GET /api/equipment/ioBasic (returns one IoBasic per pin).
    Result<std::vector<pumatronix::itscam::IoBasic>> getIoBasic(
        uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/ioBasic (bulk update of all pins).
    Result<std::vector<pumatronix::itscam::IoBasic>> setIoBasic(
        const std::vector<pumatronix::itscam::IoBasic>& ports,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  REST API client (HTTP webhook) servers
    //  Endpoint: /api/equipment/servers/restapiclient/{id}/config
    //  Endpoint: /api/equipment/servers/restapiclient/{id}/status
    // =========================================================================

    /// GET /api/equipment/servers/restapiclient/{id}/config
    Result<pumatronix::itscam::RestApiClientConfig> getRestApiClientConfig(
        int id, uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/servers/restapiclient/{id}/config
    Result<pumatronix::itscam::RestApiClientConfig> setRestApiClientConfig(
        int id,
        const pumatronix::itscam::RestApiClientConfig& config,
        uint32_t timeoutMs = 10000);

    /// GET /api/equipment/servers/restapiclient/{id}/status
    Result<pumatronix::itscam::RestApiClientStatus> getRestApiClientStatus(
        int id, uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Licenses
    //  Endpoint: /api/system/licenses
    // =========================================================================

    /// GET /api/system/licenses  (read-only summary of installed licenses).
    Result<pumatronix::itscam::Licenses> getLicenses(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Generic HTTP methods  (escape hatch for endpoints not covered above)
    // =========================================================================

    Result<nlohmann::json> httpGet(const std::string& path,
                                   uint32_t timeoutMs = 10000);

    /// PUT a JSON body to `path`.  The ITSCAM daemon treats most endpoints
    /// as partial updates and merges the supplied fields into the existing
    /// configuration.
    Result<nlohmann::json> httpPut(const std::string& path,
                                   const nlohmann::json& body,
                                   uint32_t timeoutMs = 10000);

    /// PUT a partial JSON document.  Semantically identical to `httpPut()`
    /// but documents intent: send only the fields being changed.
    Result<nlohmann::json> patchJson(const std::string& path,
                                     const nlohmann::json& patch,
                                     uint32_t timeoutMs = 10000);

    Result<nlohmann::json> httpPost(const std::string& path,
                                    const nlohmann::json& body,
                                    uint32_t timeoutMs = 10000);

    Result<nlohmann::json> httpDelete(const std::string& path,
                                      uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Settings
    // =========================================================================

    /// Override the API prefix prepended to all typed endpoint paths.
    /// Default: "/api".  Affects only the typed helper methods, not the
    /// generic httpGet/httpPut/httpPost/httpDelete methods.
    void setApiPrefix(const std::string& prefix);

    /// Read back the current API prefix.  Useful when bridging through the
    /// generic httpGet/httpPut/... escape hatch (or from the C ABI) and you
    /// still want to honour an application-supplied override.
    std::string apiPrefix() const;

    void setLogHandler(std::function<void(LogLevel, const std::string&)> cb);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace itscam
