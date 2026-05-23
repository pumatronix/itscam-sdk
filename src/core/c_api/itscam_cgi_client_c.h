/*
 *  itscam_cgi_client_c.h
 *
 *  ITSCAM Client SDK - CGI Client C Wrapper
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  C-compatible API for ItscamCgiClient.  Images are exchanged through
 *  ITSCAM_CgiImage / ITSCAM_CgiImageArray opaque handles which the SDK
 *  owns and the caller releases through the matching destroy functions.
 */
#ifndef ITSCAM_CGI_CLIENT_C_H
#define ITSCAM_CGI_CLIENT_C_H

#include "itscam_sdk_c.h"
#include "itscam_rest_client_c.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Opaque handles
 * ============================================================================ */

typedef struct ITSCAM_CgiClient      ITSCAM_CgiClient;
typedef struct ITSCAM_CgiImage       ITSCAM_CgiImage;
typedef struct ITSCAM_CgiImageArray  ITSCAM_CgiImageArray;

/* ============================================================================
 *  Snapshot request
 *
 *  Mirror of itscam::SnapshotCgiRequest.  All pointer/array fields are
 *  optional; pass NULL or 0 to leave them at their defaults.
 * ============================================================================ */

typedef struct ITSCAM_CgiSnapshotRequest {
    const int32_t* shutters;          /* CSV when serialised; len = shuttersLen */
    size_t         shuttersLen;
    const int32_t* gains;
    size_t         gainsLen;
    int32_t        quality;           /* -1 = camera default */
    int32_t        mosaic;            /* 0 = false, nonzero = true */
    const char*    format;            /* NULL, "" or "png" */
    int32_t        scenario;          /* -1 = unchanged */
    const char*    crop;              /* "x0,y0,x1,y1" */
    const char*    textOverlay;
    /* For optional User_* metadata pass two parallel NULL-terminated
     * arrays of strings (key0,key1,...,NULL  /  val0,val1,...,NULL). */
    const char* const* userMetadataKeys;
    const char* const* userMetadataValues;
} ITSCAM_CgiSnapshotRequest;

/* ============================================================================
 *  Streaming frame
 * ============================================================================ */

typedef struct ITSCAM_CgiStreamFrame {
    uint64_t       sequence;
    const char*    mimeType;          /* valid for the duration of the callback */
    const uint8_t* data;
    size_t         dataLen;
} ITSCAM_CgiStreamFrame;

typedef void (*ITSCAM_CgiStreamCallback)(
    const ITSCAM_CgiStreamFrame* frame, void* userData);

/* ============================================================================
 *  Lifecycle
 * ============================================================================ */

ITSCAM_C_API ITSCAM_CgiClient* ITSCAM_CgiClient_create(void);
ITSCAM_C_API void              ITSCAM_CgiClient_destroy(ITSCAM_CgiClient*);

/* ============================================================================
 *  Connection / TLS
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_CgiClient_setBaseUrl(
    ITSCAM_CgiClient* client, const char* host, uint16_t port,
    const char* scheme);

ITSCAM_C_API void ITSCAM_CgiClient_setApiPrefix(
    ITSCAM_CgiClient* client, const char* prefix);

ITSCAM_C_API void ITSCAM_CgiClient_setCaCertFile(
    ITSCAM_CgiClient* client, const char* pemPath);

ITSCAM_C_API void ITSCAM_CgiClient_setCaCertData(
    ITSCAM_CgiClient* client, const char* pem);

ITSCAM_C_API void ITSCAM_CgiClient_setVerifyServerCertificate(
    ITSCAM_CgiClient* client, int verify);

ITSCAM_C_API void ITSCAM_CgiClient_setClientCertificate(
    ITSCAM_CgiClient* client, const char* certPem, const char* keyPem);

/* ============================================================================
 *  Authentication
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_CgiClient_login(
    ITSCAM_CgiClient* client, const char* username, const char* password,
    uint32_t timeoutMs);

ITSCAM_C_API void ITSCAM_CgiClient_setAuthToken(
    ITSCAM_CgiClient* client, const char* token);

ITSCAM_C_API void ITSCAM_CgiClient_clearAuthToken(
    ITSCAM_CgiClient* client);

ITSCAM_C_API void ITSCAM_CgiClient_setBasicAuth(
    ITSCAM_CgiClient* client, const char* user, const char* password);

ITSCAM_C_API void ITSCAM_CgiClient_clearBasicAuth(
    ITSCAM_CgiClient* client);

/* ============================================================================
 *  /api/lastframe.cgi
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_CgiClient_getLastFrame(
    ITSCAM_CgiClient* client, uint32_t timeoutMs,
    ITSCAM_CgiImage** outImage);

/* ============================================================================
 *  /api/snapshot.cgi
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_CgiClient_getSnapshot(
    ITSCAM_CgiClient* client,
    const ITSCAM_CgiSnapshotRequest* request,
    uint32_t timeoutMs,
    ITSCAM_CgiImageArray** outImages);

/* ============================================================================
 *  /api/mjpegvideo.cgi
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_CgiClient_startMjpegStream(
    ITSCAM_CgiClient* client,
    ITSCAM_CgiStreamCallback callback,
    void* userData,
    uint32_t timeoutMs);

ITSCAM_C_API void ITSCAM_CgiClient_stopMjpegStream(
    ITSCAM_CgiClient* client);

ITSCAM_C_API int ITSCAM_CgiClient_isMjpegStreamRunning(
    ITSCAM_CgiClient* client);

/* ============================================================================
 *  /api/trigger.cgi (forceTrigger only)
 *  /api/reboot.cgi
 * ============================================================================ */

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_CgiClient_forceTrigger(
    ITSCAM_CgiClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_CgiClient_reboot(
    ITSCAM_CgiClient* client, uint32_t timeoutMs,
    ITSCAM_String** outResponse);

/* ============================================================================
 *  Logging
 * ============================================================================ */

ITSCAM_C_API void ITSCAM_CgiClient_onLog(
    ITSCAM_CgiClient* client, ITSCAM_LogCallback callback, void* userData);

/* ============================================================================
 *  ITSCAM_CgiImage accessors
 * ============================================================================ */

ITSCAM_C_API const char*    ITSCAM_CgiImage_mimeType(const ITSCAM_CgiImage* img);
ITSCAM_C_API const uint8_t* ITSCAM_CgiImage_data(const ITSCAM_CgiImage* img);
ITSCAM_C_API size_t         ITSCAM_CgiImage_size(const ITSCAM_CgiImage* img);
ITSCAM_C_API void           ITSCAM_CgiImage_destroy(ITSCAM_CgiImage* img);

/* ============================================================================
 *  ITSCAM_CgiImageArray accessors
 * ============================================================================ */

ITSCAM_C_API size_t                 ITSCAM_CgiImageArray_size(
    const ITSCAM_CgiImageArray* arr);

/// Borrowed reference -- valid until the array is destroyed.  Do not call
/// ITSCAM_CgiImage_destroy on the returned pointer.
ITSCAM_C_API const ITSCAM_CgiImage* ITSCAM_CgiImageArray_get(
    const ITSCAM_CgiImageArray* arr, size_t index);

ITSCAM_C_API void                   ITSCAM_CgiImageArray_destroy(
    ITSCAM_CgiImageArray* arr);

#ifdef __cplusplus
}
#endif

#endif /* ITSCAM_CGI_CLIENT_C_H */
