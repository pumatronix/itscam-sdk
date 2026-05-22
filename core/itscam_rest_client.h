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
 *  Requires: C++14, nlohmann/json 3.x, cpp-httplib 0.18+ (header-only).
 */
#pragma once

#include "itscam_types.h"

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#include "3rdparty/nlohmann/json.hpp"
#endif

#include <functional>
#include <memory>
#include <string>

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

    /// Configure the target host and port for the webapp backend.
    /// Must be called before any HTTP method.  Does not open a persistent
    /// connection -- each request uses a short-lived HTTP transaction.
    void setBaseUrl(const std::string& host, uint16_t port = 80);

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
    //  Image profiles
    //  Endpoint: /api/image/profiles
    // =========================================================================

    /// GET /api/image/profiles -- list all profiles.
    Result<nlohmann::json> getProfiles(uint32_t timeoutMs = 10000);

    /// GET /api/image/profiles?id=<id> -- get a single profile by id.
    Result<nlohmann::json> getProfile(int id, uint32_t timeoutMs = 10000);

    /// POST /api/image/profiles -- create a new profile.
    Result<nlohmann::json> createProfile(const nlohmann::json& profile,
                                         uint32_t timeoutMs = 10000);

    /// PUT /api/image/profiles -- update an existing profile.
    Result<nlohmann::json> updateProfile(const nlohmann::json& profile,
                                         uint32_t timeoutMs = 10000);

    /// DELETE /api/image/profiles?id=<id> -- delete a profile by id.
    Result<nlohmann::json> deleteProfile(int id, uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Equipment volatile info  (read-only)
    //  Endpoint: /api/equipment/misc/readonly/volatile
    // =========================================================================

    /// GET /api/equipment/misc/readonly/volatile
    Result<nlohmann::json> getVolatileInfo(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Equipment general configuration
    //  Endpoint: /api/equipment/general
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
    Result<nlohmann::json> getAnalyticsConfig(uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/analytics
    Result<nlohmann::json> setAnalyticsConfig(const nlohmann::json& config,
                                              uint32_t timeoutMs = 10000);

    // =========================================================================
    //  OCR configuration
    //  Endpoint: /api/equipment/ocr
    // =========================================================================

    /// GET /api/equipment/ocr
    Result<nlohmann::json> getOcrConfig(uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/ocr
    Result<nlohmann::json> setOcrConfig(const nlohmann::json& config,
                                        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Classifier configuration
    //  Endpoint: /api/equipment/classifier
    // =========================================================================

    /// GET /api/equipment/classifier
    Result<nlohmann::json> getClassifierConfig(uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/classifier
    Result<nlohmann::json> setClassifierConfig(const nlohmann::json& config,
                                               uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Lanes configuration
    //  Endpoint: /api/equipment/lanes
    // =========================================================================

    /// GET /api/equipment/lanes
    Result<nlohmann::json> getLanesConfig(uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/lanes
    Result<nlohmann::json> setLanesConfig(const nlohmann::json& config,
                                          uint32_t timeoutMs = 10000);

    // =========================================================================
    //  ITSCAM PRO server configuration
    //  Endpoint: /api/equipment/servers/itscampro
    // =========================================================================

    /// GET /api/equipment/servers/itscampro
    Result<nlohmann::json> getItscamproConfig(uint32_t timeoutMs = 10000);

    /// PUT /api/equipment/servers/itscampro
    Result<nlohmann::json> setItscamproConfig(const nlohmann::json& config,
                                              uint32_t timeoutMs = 10000);

    /// GET /api/equipment/servers/itscampro/status
    Result<nlohmann::json> getItscamproStatus(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Generic HTTP methods  (escape hatch for endpoints not covered above)
    // =========================================================================

    Result<nlohmann::json> httpGet(const std::string& path,
                                   uint32_t timeoutMs = 10000);

    Result<nlohmann::json> httpPut(const std::string& path,
                                   const nlohmann::json& body,
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

    void setLogHandler(std::function<void(LogLevel, const std::string&)> cb);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace itscam
