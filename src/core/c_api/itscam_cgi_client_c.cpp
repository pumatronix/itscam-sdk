/*
 *  itscam_cgi_client_c.cpp
 *
 *  ITSCAM Client SDK - CGI Client C Wrapper Implementation
 *
 *  Copyright (c) 2026 Pumatronix
 */

#include "itscam_cgi_client_c.h"
#include "../itscam_cgi_client.h"
#include "impl/itscam_c_internal.h"

#include <new>
#include <string>
#include <vector>

//=========================================================================
// Opaque types
//=========================================================================

struct ITSCAM_CgiClient {
    itscam::ItscamCgiClient impl;

    ITSCAM_CgiStreamCallback streamCb = nullptr;
    void*                    streamUd = nullptr;

    ITSCAM_LogCallback       logCb = nullptr;
    void*                    logUd = nullptr;
};

struct ITSCAM_CgiImage {
    itscam::CgiImage data;
};

struct ITSCAM_CgiImageArray {
    std::vector<ITSCAM_CgiImage> items;
};

//=========================================================================
// Helpers
//=========================================================================

namespace {

ITSCAM_ErrorCode translateError(const itscam::Error& err) {
    // Surface the upstream message via ITSCAM_getLastError() so wrappers
    // (C#, Python, Go) can include the server-side detail in their
    // exception/error types.
    itscam_c_internal::setLastError(err.message);
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

itscam::SnapshotCgiRequest fromC(const ITSCAM_CgiSnapshotRequest* in) {
    itscam::SnapshotCgiRequest out;
    if (!in) return out;
    if (in->shutters && in->shuttersLen) {
        out.shutters.assign(in->shutters, in->shutters + in->shuttersLen);
    }
    if (in->gains && in->gainsLen) {
        out.gains.assign(in->gains, in->gains + in->gainsLen);
    }
    out.quality     = in->quality;
    out.mosaic      = in->mosaic != 0;
    if (in->format)      out.format      = in->format;
    out.scenario    = in->scenario;
    if (in->crop)        out.crop        = in->crop;
    if (in->textOverlay) out.textOverlay = in->textOverlay;

    if (in->userMetadataKeys && in->userMetadataValues) {
        for (size_t i = 0; ; ++i) {
            const char* k = in->userMetadataKeys[i];
            const char* v = in->userMetadataValues[i];
            if (!k || !v) break;
            out.userMetadata[k] = v;
        }
    }
    return out;
}

}  // namespace

//=========================================================================
// Lifecycle
//=========================================================================

extern "C" {

ITSCAM_CgiClient* ITSCAM_CgiClient_create(void) {
    return new (std::nothrow) ITSCAM_CgiClient();
}

void ITSCAM_CgiClient_destroy(ITSCAM_CgiClient* c) {
    delete c;
}

//=========================================================================
// Connection / TLS
//=========================================================================

ITSCAM_ErrorCode ITSCAM_CgiClient_setBaseUrl(ITSCAM_CgiClient* c,
                                             const char* host,
                                             uint16_t port,
                                             const char* scheme) {
    if (!c || !host) return ITSCAM_ERROR_NULL_HANDLE;
    c->impl.setBaseUrl(host, port,
                       scheme && *scheme ? std::string(scheme)
                                          : std::string("http"));
    return ITSCAM_OK;
}

void ITSCAM_CgiClient_setApiPrefix(ITSCAM_CgiClient* c, const char* p) {
    if (c && p) c->impl.setApiPrefix(p);
}

void ITSCAM_CgiClient_setCaCertFile(ITSCAM_CgiClient* c, const char* p) {
    if (c && p) c->impl.setCaCertFile(p);
}

void ITSCAM_CgiClient_setCaCertData(ITSCAM_CgiClient* c, const char* p) {
    if (c && p) c->impl.setCaCertData(p);
}

void ITSCAM_CgiClient_setVerifyServerCertificate(ITSCAM_CgiClient* c,
                                                 int verify) {
    if (c) c->impl.setVerifyServerCertificate(verify != 0);
}

void ITSCAM_CgiClient_setClientCertificate(ITSCAM_CgiClient* c,
                                           const char* certPem,
                                           const char* keyPem) {
    if (c && certPem && keyPem) c->impl.setClientCertificate(certPem, keyPem);
}

//=========================================================================
// Authentication
//=========================================================================

ITSCAM_ErrorCode ITSCAM_CgiClient_login(ITSCAM_CgiClient* c,
                                        const char* username,
                                        const char* password,
                                        uint32_t timeoutMs) {
    if (!c || !username || !password) return ITSCAM_ERROR_NULL_HANDLE;
    auto r = c->impl.login(username, password, timeoutMs);
    return r ? ITSCAM_OK : translateError(r.error());
}

void ITSCAM_CgiClient_setAuthToken(ITSCAM_CgiClient* c, const char* t) {
    if (c && t) c->impl.setAuthToken(t);
}

void ITSCAM_CgiClient_clearAuthToken(ITSCAM_CgiClient* c) {
    if (c) c->impl.clearAuthToken();
}

void ITSCAM_CgiClient_setBasicAuth(ITSCAM_CgiClient* c, const char* u,
                                   const char* p) {
    if (c && u && p) c->impl.setBasicAuth(u, p);
}

void ITSCAM_CgiClient_clearBasicAuth(ITSCAM_CgiClient* c) {
    if (c) c->impl.clearBasicAuth();
}

//=========================================================================
// /api/lastframe.cgi
//=========================================================================

ITSCAM_ErrorCode ITSCAM_CgiClient_getLastFrame(ITSCAM_CgiClient* c,
                                               uint32_t timeoutMs,
                                               ITSCAM_CgiImage** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    if (out) *out = nullptr;
    auto r = c->impl.getLastFrame(timeoutMs);
    if (!r) return translateError(r.error());
    if (out) {
        auto* img = new (std::nothrow) ITSCAM_CgiImage();
        if (!img) return ITSCAM_ERROR_ALLOCATION_FAILED;
        img->data = std::move(r.value());
        *out = img;
    }
    return ITSCAM_OK;
}

//=========================================================================
// /api/snapshot.cgi
//=========================================================================

ITSCAM_ErrorCode ITSCAM_CgiClient_getSnapshot(ITSCAM_CgiClient* c,
                                              const ITSCAM_CgiSnapshotRequest* req,
                                              uint32_t timeoutMs,
                                              ITSCAM_CgiImageArray** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    if (out) *out = nullptr;
    auto cppReq = fromC(req);
    auto r = c->impl.getSnapshot(cppReq, timeoutMs);
    if (!r) return translateError(r.error());
    if (out) {
        auto* arr = new (std::nothrow) ITSCAM_CgiImageArray();
        if (!arr) return ITSCAM_ERROR_ALLOCATION_FAILED;
        arr->items.reserve(r.value().size());
        for (auto& img : r.value()) {
            ITSCAM_CgiImage entry;
            entry.data = std::move(img);
            arr->items.push_back(std::move(entry));
        }
        *out = arr;
    }
    return ITSCAM_OK;
}

//=========================================================================
// /api/mjpegvideo.cgi
//=========================================================================

ITSCAM_ErrorCode ITSCAM_CgiClient_startMjpegStream(ITSCAM_CgiClient* c,
                                                   ITSCAM_CgiStreamCallback cb,
                                                   void* ud,
                                                   uint32_t timeoutMs) {
    if (!c || !cb) return ITSCAM_ERROR_NULL_HANDLE;
    c->streamCb = cb;
    c->streamUd = ud;
    auto* holder = c;
    auto r = c->impl.startMjpegStream(
        [holder](const itscam::CgiStreamFrame& f) {
            if (!holder->streamCb) return;
            ITSCAM_CgiStreamFrame frame;
            frame.sequence = f.sequence;
            frame.mimeType = f.mimeType.c_str();
            frame.data     = f.data.empty() ? nullptr : f.data.data();
            frame.dataLen  = f.data.size();
            holder->streamCb(&frame, holder->streamUd);
        }, timeoutMs);
    return r ? ITSCAM_OK : translateError(r.error());
}

void ITSCAM_CgiClient_stopMjpegStream(ITSCAM_CgiClient* c) {
    if (c) c->impl.stopMjpegStream();
}

int ITSCAM_CgiClient_isMjpegStreamRunning(ITSCAM_CgiClient* c) {
    return (c && c->impl.isMjpegStreamRunning()) ? 1 : 0;
}

//=========================================================================
// /api/trigger.cgi force / /api/reboot.cgi
//=========================================================================

static ITSCAM_ErrorCode wrapStringResult(
    const itscam::Result<std::string>& r, ITSCAM_String** out) {
    if (!r) return translateError(r.error());
    if (out) {
        auto* s = itscam_c_internal::makeString(r.value());
        if (!s) return ITSCAM_ERROR_ALLOCATION_FAILED;
        *out = s;
    }
    return ITSCAM_OK;
}

ITSCAM_ErrorCode ITSCAM_CgiClient_forceTrigger(ITSCAM_CgiClient* c,
                                               uint32_t timeoutMs,
                                               ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    if (out) *out = nullptr;
    return wrapStringResult(c->impl.forceTrigger(timeoutMs), out);
}

ITSCAM_ErrorCode ITSCAM_CgiClient_reboot(ITSCAM_CgiClient* c,
                                         uint32_t timeoutMs,
                                         ITSCAM_String** out) {
    if (!c) return ITSCAM_ERROR_NULL_HANDLE;
    if (out) *out = nullptr;
    return wrapStringResult(c->impl.reboot(timeoutMs), out);
}

//=========================================================================
// Logging
//=========================================================================

void ITSCAM_CgiClient_onLog(ITSCAM_CgiClient* c, ITSCAM_LogCallback cb,
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
// ITSCAM_CgiImage / ITSCAM_CgiImageArray accessors
//=========================================================================

const char* ITSCAM_CgiImage_mimeType(const ITSCAM_CgiImage* img) {
    return img ? img->data.mimeType.c_str() : "";
}

const uint8_t* ITSCAM_CgiImage_data(const ITSCAM_CgiImage* img) {
    if (!img || img->data.data.empty()) return nullptr;
    return img->data.data.data();
}

size_t ITSCAM_CgiImage_size(const ITSCAM_CgiImage* img) {
    return img ? img->data.data.size() : 0;
}

void ITSCAM_CgiImage_destroy(ITSCAM_CgiImage* img) {
    delete img;
}

size_t ITSCAM_CgiImageArray_size(const ITSCAM_CgiImageArray* arr) {
    return arr ? arr->items.size() : 0;
}

const ITSCAM_CgiImage* ITSCAM_CgiImageArray_get(
    const ITSCAM_CgiImageArray* arr, size_t index) {
    if (!arr || index >= arr->items.size()) return nullptr;
    return &arr->items[index];
}

void ITSCAM_CgiImageArray_destroy(ITSCAM_CgiImageArray* arr) {
    delete arr;
}

}  // extern "C"
