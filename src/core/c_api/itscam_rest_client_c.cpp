/*
 *  itscam_rest_client_c.cpp
 *
 *  ITSCAM Client SDK - REST API C Wrapper Implementation
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Thin C facade over itscam::ItscamRestClient.  Uses nlohmann::json for
 *  body serialization/deserialization between the C++ surface and the
 *  C UTF-8 string surface.
 */

#include "itscam_rest_client_c.h"
#include "../itscam_rest_client.h"
#include "impl/itscam_c_internal.h"

#include "../3rdparty/nlohmann/json.hpp"

#include <cstring>
#include <memory>
#include <new>
#include <string>

using json = nlohmann::json;

//=========================================================================
// Opaque types
//=========================================================================

struct ITSCAM_RestClient {
    itscam::ItscamRestClient impl;

    ITSCAM_LogCallback logCb = nullptr;
    void*              logUd = nullptr;
};

//=========================================================================
// Helpers
//=========================================================================

namespace {

ITSCAM_ErrorCode translateError(const itscam::Error& err) {
    // Surface the upstream message via ITSCAM_getLastError() so wrappers
    // (C#, Python, Go) can include the server-side detail in their
    // exception/error types.
    itscam::c_internal::setLastError(err.message);
    switch (err.code) {
        case itscam::Error::ConnectionFailed: return ITSCAM_ERROR_CONNECTION_FAILED;
        case itscam::Error::Timeout:          return ITSCAM_ERROR_TIMEOUT;
        case itscam::Error::NotAuthenticated: return ITSCAM_ERROR_NOT_AUTHENTICATED;
        case itscam::Error::InvalidParameter: return ITSCAM_ERROR_INVALID_PARAMETER;
        case itscam::Error::ServerError:      return ITSCAM_ERROR_SERVER_ERROR;
        case itscam::Error::Disconnected:     return ITSCAM_ERROR_DISCONNECTED;
        default:                              return ITSCAM_ERROR_UNKNOWN;
    }
}

void assignOut(ITSCAM_String** out, const json& body) {
    if (!out) return;
    *out = itscam::c_internal::makeString(
        body.is_null() ? std::string("") : body.dump());
}

ITSCAM_ErrorCode handleResult(const itscam::Result<json>& res,
                              ITSCAM_String** out) {
    if (res) {
        assignOut(out, res.value());
        return ITSCAM_OK;
    }
    if (out) *out = nullptr;
    return translateError(res.error());
}

/// Parse a UTF-8 JSON string supplied by the caller; on failure returns
/// a JSON null and an error message via setLastError-equivalent (we
/// simply pass null forward and let the server reject it).
json parseJsonBody(const char* body) {
    if (!body || !*body) return json::object();
    try {
        return json::parse(body);
    } catch (const json::parse_error&) {
        // Treat the input as a raw string -- most setters accept that.
        return json(body);
    }
}

/// Concatenate the client's current API prefix with a `tail` such as
/// "/equipment/ocr".  Lets the C ABI route through the generic httpGet /
/// httpPut helpers while still honouring `setApiPrefix(...)`.
inline std::string apiPath(ITSCAM_RestClient* c, const char* tail) {
    return c->impl.apiPrefix() + tail;
}

}  // namespace

//=========================================================================
// Lifecycle
//=========================================================================

extern "C" {

ITSCAM_RestClient* ITSCAM_RestClient_create(void) {
    return new (std::nothrow) ITSCAM_RestClient();
}

void ITSCAM_RestClient_destroy(ITSCAM_RestClient* client) {
    delete client;
}

//=========================================================================
// Connection / TLS
//=========================================================================

ITSCAM_ErrorCode ITSCAM_RestClient_setBaseUrl(ITSCAM_RestClient* c,
                                              const char* host,
                                              uint16_t port,
                                              const char* scheme) {
    if (!c || !host) return ITSCAM_ERROR_NULL_HANDLE;
    c->impl.setBaseUrl(host, port,
                       scheme && *scheme ? std::string(scheme) :
                                            std::string("http"));
    return ITSCAM_OK;
}

void ITSCAM_RestClient_setApiPrefix(ITSCAM_RestClient* c, const char* prefix) {
    if (c && prefix) c->impl.setApiPrefix(prefix);
}

void ITSCAM_RestClient_setCaCertFile(ITSCAM_RestClient* c, const char* p) {
    if (c && p) c->impl.setCaCertFile(p);
}

void ITSCAM_RestClient_setCaCertData(ITSCAM_RestClient* c, const char* p) {
    if (c && p) c->impl.setCaCertData(p);
}

void ITSCAM_RestClient_setVerifyServerCertificate(ITSCAM_RestClient* c,
                                                  int verify) {
    if (c) c->impl.setVerifyServerCertificate(verify != 0);
}

void ITSCAM_RestClient_setClientCertificate(ITSCAM_RestClient* c,
                                            const char* certPem,
                                            const char* keyPem) {
    if (c && certPem && keyPem) c->impl.setClientCertificate(certPem, keyPem);
}

//=========================================================================
// Authentication
//=========================================================================

ITSCAM_ErrorCode ITSCAM_RestClient_login(ITSCAM_RestClient* c,
                                         const char* username,
                                         const char* password,
                                         uint32_t timeoutMs,
                                         ITSCAM_String** outResponse) {
    if (!c || !username || !password) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.login(username, password, timeoutMs),
                        outResponse);
}

void ITSCAM_RestClient_setAuthToken(ITSCAM_RestClient* c, const char* t) {
    if (c && t) c->impl.setAuthToken(t);
}

void ITSCAM_RestClient_clearAuthToken(ITSCAM_RestClient* c) {
    if (c) c->impl.clearAuthToken();
}

//=========================================================================
// Generic HTTP verbs
//=========================================================================

ITSCAM_ErrorCode ITSCAM_RestClient_httpGet(ITSCAM_RestClient* c,
                                           const char* path,
                                           uint32_t timeoutMs,
                                           ITSCAM_String** out) {
    if (!c || !path) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpGet(path, timeoutMs), out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_httpPut(ITSCAM_RestClient* c,
                                           const char* path,
                                           const char* body,
                                           uint32_t timeoutMs,
                                           ITSCAM_String** out) {
    if (!c || !path) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpPut(path, parseJsonBody(body),
                                        timeoutMs), out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_httpPost(ITSCAM_RestClient* c,
                                            const char* path,
                                            const char* body,
                                            uint32_t timeoutMs,
                                            ITSCAM_String** out) {
    if (!c || !path) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpPost(path, parseJsonBody(body),
                                         timeoutMs), out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_httpDelete(ITSCAM_RestClient* c,
                                              const char* path,
                                              uint32_t timeoutMs,
                                              ITSCAM_String** out) {
    if (!c || !path) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpDelete(path, timeoutMs), out);
}

//=========================================================================
// Typed convenience wrappers
//
// All these wrappers route through the generic httpGet / httpPut escape
// hatch on the C++ side so the C ABI stays JSON-string-based (the C++
// surface returns strongly-typed C++ structs which the C ABI cannot
// marshal directly).  `setApiPrefix(...)` is honoured by reading the
// current prefix back via `apiPrefix()`.
//=========================================================================

ITSCAM_ErrorCode ITSCAM_RestClient_getProfiles(ITSCAM_RestClient* c,
                                               uint32_t t,
                                               ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpGet(apiPath(c, "/image/profiles"), t),
                        out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_getProfile(ITSCAM_RestClient* c, int id,
                                              uint32_t t,
                                              ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    std::string path =
        apiPath(c, "/image/profiles?id=") + std::to_string(id);
    return handleResult(c->impl.httpGet(path, t), out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_createProfile(ITSCAM_RestClient* c,
                                                 const char* body,
                                                 uint32_t t,
                                                 ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpPost(apiPath(c, "/image/profiles"),
                                          parseJsonBody(body), t), out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_updateProfile(ITSCAM_RestClient* c,
                                                 const char* body,
                                                 uint32_t t,
                                                 ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpPut(apiPath(c, "/image/profiles"),
                                         parseJsonBody(body), t), out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_deleteProfile(ITSCAM_RestClient* c, int id,
                                                 uint32_t t,
                                                 ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    std::string path =
        apiPath(c, "/image/profiles?id=") + std::to_string(id);
    return handleResult(c->impl.httpDelete(path, t), out);
}

ITSCAM_ErrorCode ITSCAM_RestClient_getVolatileInfo(ITSCAM_RestClient* c,
                                                   uint32_t t,
                                                   ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    return handleResult(c->impl.httpGet(
        apiPath(c, "/equipment/misc/readonly/volatile"), t), out);
}

#define DEFINE_TYPED_GET(Name, Path)                                     \
    ITSCAM_ErrorCode ITSCAM_RestClient_##Name(ITSCAM_RestClient* c,       \
                                              uint32_t t,                 \
                                              ITSCAM_String** out) {      \
        if (!c) return ITSCAM_ERROR_NULL_HANDLE;                          \
        return handleResult(c->impl.httpGet(apiPath(c, Path), t), out);   \
    }

#define DEFINE_TYPED_SET(Name, Path)                                     \
    ITSCAM_ErrorCode ITSCAM_RestClient_##Name(ITSCAM_RestClient* c,       \
                                              const char* body,           \
                                              uint32_t t,                 \
                                              ITSCAM_String** out) {      \
        if (!c) return ITSCAM_ERROR_NULL_HANDLE;                          \
        return handleResult(c->impl.httpPut(apiPath(c, Path),             \
                                            parseJsonBody(body), t), out);\
    }

DEFINE_TYPED_GET(getGeneralConfig,    "/equipment/general")
DEFINE_TYPED_SET(setGeneralConfig,    "/equipment/general")
DEFINE_TYPED_GET(getAnalyticsConfig,  "/equipment/analytics")
DEFINE_TYPED_SET(setAnalyticsConfig,  "/equipment/analytics")
DEFINE_TYPED_GET(getOcrConfig,        "/equipment/ocr")
DEFINE_TYPED_SET(setOcrConfig,        "/equipment/ocr")
DEFINE_TYPED_GET(getClassifierConfig, "/equipment/classifier")
DEFINE_TYPED_SET(setClassifierConfig, "/equipment/classifier")
DEFINE_TYPED_GET(getLanesConfig,      "/equipment/lanes")
DEFINE_TYPED_SET(setLanesConfig,      "/equipment/lanes")
DEFINE_TYPED_GET(getItscamproConfig,  "/equipment/servers/itscampro")
DEFINE_TYPED_SET(setItscamproConfig,  "/equipment/servers/itscampro")
DEFINE_TYPED_GET(getItscamproStatus,  "/equipment/servers/itscampro/status")

#undef DEFINE_TYPED_GET
#undef DEFINE_TYPED_SET

//=========================================================================
// Logging
//=========================================================================

void ITSCAM_RestClient_onLog(ITSCAM_RestClient* c, ITSCAM_LogCallback cb,
                             void* ud) {
    if (!c) return;
    c->logCb = cb;
    c->logUd = ud;
    if (!cb) {
        c->impl.setLogHandler(nullptr);
        return;
    }
    auto* holder = c;
    c->impl.setLogHandler([holder](itscam::LogLevel lvl,
                                   const std::string& msg) {
        if (holder->logCb) {
            holder->logCb(lvl == itscam::LogLevel::Error ? 1 : 0,
                          msg.c_str(), holder->logUd);
        }
    });
}

//=========================================================================
// ITSCAM_String accessors
//=========================================================================

const char* ITSCAM_String_data(const ITSCAM_String* s) {
    return s ? s->data.c_str() : "";
}

size_t ITSCAM_String_size(const ITSCAM_String* s) {
    return s ? s->data.size() : 0;
}

void ITSCAM_String_destroy(ITSCAM_String* s) {
    delete s;
}

}  // extern "C"
