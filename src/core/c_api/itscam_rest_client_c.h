/*
 *  itscam_rest_client_c.h
 *
 *  ITSCAM Client SDK - REST API C Wrapper
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  C-compatible API for ItscamRestClient.  All bodies are exchanged as
 *  UTF-8 JSON strings, simplifying FFI use from Python, Go and C#.
 *
 *  Memory ownership rules:
 *    - Strings returned through ITSCAM_String* must be released with
 *      ITSCAM_String_destroy().
 *    - The opaque ITSCAM_RestClient handle is created with
 *      ITSCAM_RestClient_create() and freed with ITSCAM_RestClient_destroy().
 */
#ifndef ITSCAM_REST_CLIENT_C_H
#define ITSCAM_REST_CLIENT_C_H

#include "itscam_sdk_c.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Opaque handles
 * ============================================================================ */

typedef struct ITSCAM_RestClient ITSCAM_RestClient;

/// UTF-8 NUL-terminated string buffer owned by the SDK.
typedef struct ITSCAM_String ITSCAM_String;

/* ============================================================================
 *  Lifecycle
 * ============================================================================ */

/// Allocate a new REST client.  Returns NULL on out-of-memory.
ITSCAM_C_API ITSCAM_RestClient* ITSCAM_RestClient_create(void);

/// Free a REST client.  Safe to call with NULL.
ITSCAM_C_API void ITSCAM_RestClient_destroy(ITSCAM_RestClient* client);

/* ============================================================================
 *  Connection / TLS
 * ============================================================================ */

/// Configure host, port and scheme.  scheme is "http" (default) or "https".
/// Pass port = 0 to use the protocol default (80 for http, 443 for https).
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_setBaseUrl(
    ITSCAM_RestClient* client,
    const char* host,
    uint16_t port,
    const char* scheme);

ITSCAM_C_API void ITSCAM_RestClient_setApiPrefix(
    ITSCAM_RestClient* client,
    const char* prefix);

ITSCAM_C_API void ITSCAM_RestClient_setCaCertFile(
    ITSCAM_RestClient* client,
    const char* pemPath);

ITSCAM_C_API void ITSCAM_RestClient_setCaCertData(
    ITSCAM_RestClient* client,
    const char* pem);

ITSCAM_C_API void ITSCAM_RestClient_setVerifyServerCertificate(
    ITSCAM_RestClient* client,
    int verify);

ITSCAM_C_API void ITSCAM_RestClient_setClientCertificate(
    ITSCAM_RestClient* client,
    const char* certPem,
    const char* keyPem);

/* ============================================================================
 *  Authentication
 * ============================================================================ */

/// POST /api/auth and store the returned bearer token internally.
/// outResponse receives the JSON response (caller must release with
/// ITSCAM_String_destroy).  Pass NULL to discard.
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_login(
    ITSCAM_RestClient* client,
    const char* username,
    const char* password,
    uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API void ITSCAM_RestClient_setAuthToken(
    ITSCAM_RestClient* client,
    const char* token);

ITSCAM_C_API void ITSCAM_RestClient_clearAuthToken(
    ITSCAM_RestClient* client);

/* ============================================================================
 *  Generic HTTP verbs (the typed helpers below are convenience wrappers)
 *
 *  All variants follow the same pattern:
 *      - path is taken verbatim
 *      - body (when applicable) is a UTF-8 JSON string
 *      - outResponse receives the parsed JSON response string (or NULL on
 *        failure); the caller must release it with ITSCAM_String_destroy
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_httpGet(
    ITSCAM_RestClient* client,
    const char* path,
    uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_httpPut(
    ITSCAM_RestClient* client,
    const char* path,
    const char* jsonBody,
    uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_httpPost(
    ITSCAM_RestClient* client,
    const char* path,
    const char* jsonBody,
    uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_httpDelete(
    ITSCAM_RestClient* client,
    const char* path,
    uint32_t timeoutMs,
    ITSCAM_String** outResponse);

/* ============================================================================
 *  Typed convenience wrappers
 *
 *  These call the matching ItscamRestClient methods and return the JSON
 *  response through outResponse.
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getProfiles(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getProfile(
    ITSCAM_RestClient* client, int profileId, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getProfileByName(
    ITSCAM_RestClient* client, const char* name, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_createProfile(
    ITSCAM_RestClient* client, const char* jsonProfile, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_updateProfile(
    ITSCAM_RestClient* client, const char* jsonProfile, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_updateProfileByName(
    ITSCAM_RestClient* client, const char* name, const char* jsonProfile,
    uint32_t timeoutMs, ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_deleteProfile(
    ITSCAM_RestClient* client, int profileId, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getVolatileInfo(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getGeneralConfig(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_setGeneralConfig(
    ITSCAM_RestClient* client, const char* jsonConfig, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getAnalyticsConfig(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_setAnalyticsConfig(
    ITSCAM_RestClient* client, const char* jsonConfig, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getOcrConfig(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_setOcrConfig(
    ITSCAM_RestClient* client, const char* jsonConfig, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getClassifierConfig(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_setClassifierConfig(
    ITSCAM_RestClient* client, const char* jsonConfig, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getLanesConfig(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_setLanesConfig(
    ITSCAM_RestClient* client, const char* jsonConfig, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getItscamproConfig(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_setItscamproConfig(
    ITSCAM_RestClient* client, const char* jsonConfig, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_RestClient_getItscamproStatus(
    ITSCAM_RestClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

/* ============================================================================
 *  Logging
 * ============================================================================ */

ITSCAM_C_API void ITSCAM_RestClient_onLog(
    ITSCAM_RestClient* client,
    ITSCAM_LogCallback callback,
    void* userData);

/* ============================================================================
 *  ITSCAM_String accessors
 * ============================================================================ */

ITSCAM_C_API const char* ITSCAM_String_data(const ITSCAM_String* s);
ITSCAM_C_API size_t      ITSCAM_String_size(const ITSCAM_String* s);
ITSCAM_C_API void        ITSCAM_String_destroy(ITSCAM_String* s);

#ifdef __cplusplus
}
#endif

#endif /* ITSCAM_REST_CLIENT_C_H */
