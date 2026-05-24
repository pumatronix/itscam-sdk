/*
 *  itscam_sdk_c.cpp
 *
 *  ITSCAM Client SDK - C API Wrapper Implementation
 *
 *  Copyright (c) 2026 Pumatronix
 */

#include "itscam_sdk_c.h"
#include "../itscam_sdk_version.h"
#include "../itscam_client.h"
#include "../itscam_sdk_utils.h"
#include "../itscam_jpeg_utils.h"
#include "impl/itscam_c_internal.h"

#include <cstdarg>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// Thread-local error buffer shared by every C ABI translation unit through
// itscam_c_internal::setLastError().
static thread_local char s_lastError[512] = {0};

namespace itscam_c_internal {
void setLastError(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(s_lastError, sizeof(s_lastError), fmt, args);
    va_end(args);
}
}  // namespace itscam_c_internal

// Local alias so the existing call sites in this file remain readable.
using itscam_c_internal::setLastError;

static ITSCAM_ErrorCode translateError(const itscam::Error& err) {
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

//=========================================================================
// Opaque Structure Definitions
//=========================================================================

struct ITSCAM_Client {
    std::unique_ptr<itscam::ItscamClient> impl;
    
    // Callback storage (prevent GC in bound languages)
    ITSCAM_CaptureCallback           triggerImageCb = nullptr;
    void*                            triggerImageUserData = nullptr;
    ITSCAM_CaptureCallback           snapshotImageCb = nullptr;
    void*                            snapshotImageUserData = nullptr;
    ITSCAM_DisconnectCallback        disconnectCb = nullptr;
    void*                            disconnectUserData = nullptr;
    ITSCAM_ConnectionStateCallback   connectionStateCb = nullptr;
    void*                            connectionStateUserData = nullptr;
    ITSCAM_LogCallback               logCb = nullptr;
    void*                            logUserData = nullptr;
    
    ITSCAM_Client() : impl(std::make_unique<itscam::ItscamClient>()) {}
};

struct ITSCAM_CaptureResult {
    itscam::CaptureResult data;
};

struct ITSCAM_CaptureResultArray {
    std::vector<itscam::CaptureResult> results;
};

struct ITSCAM_ByteArray {
    std::vector<uint8_t> data;
};

struct ITSCAM_ProfileArray {
    std::vector<itscam::ProfileInfo> profiles;
    // String storage (since ProfileInfo has std::string, we need to keep copies)
    std::vector<std::string> names;
    std::vector<std::string> descriptions;
};

//=========================================================================
// Client Lifecycle
//=========================================================================

ITSCAM_Client* ITSCAM_Client_create(void) {
    try {
        return new ITSCAM_Client();
    } catch (const std::exception& e) {
        setLastError("Failed to create client: %s", e.what());
        return nullptr;
    } catch (...) {
        setLastError("Failed to create client: unknown error");
        return nullptr;
    }
}

void ITSCAM_Client_destroy(ITSCAM_Client* client) {
    delete client;
}

//=========================================================================
// Connection
//=========================================================================

ITSCAM_ErrorCode ITSCAM_Client_connect(
    ITSCAM_Client* client,
    const char* address,
    uint16_t port,
    uint32_t timeoutMs,
    const ITSCAM_AutoReconnectConfig* reconnect)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    if (!address) {
        setLastError("Null address");
        return ITSCAM_ERROR_INVALID_PARAMETER;
    }
    
    itscam::AutoReconnectConfig cfg;
    if (reconnect) {
        cfg.enabled = reconnect->enabled != 0;
        cfg.intervalMs = reconnect->intervalMs;
        cfg.maxRetries = reconnect->maxRetries;
    }
    
    auto result = client->impl->connect(address, port, timeoutMs, cfg);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    return ITSCAM_OK;
}

void ITSCAM_Client_disconnect(ITSCAM_Client* client) {
    if (client && client->impl) {
        client->impl->disconnect();
    }
}

int ITSCAM_Client_isConnected(ITSCAM_Client* client) {
    if (!client || !client->impl) return 0;
    return client->impl->isConnected() ? 1 : 0;
}

//=========================================================================
// Authentication
//=========================================================================

ITSCAM_ErrorCode ITSCAM_Client_authenticate(
    ITSCAM_Client* client,
    const char* password,
    uint32_t timeoutMs)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    if (!password) {
        setLastError("Null password");
        return ITSCAM_ERROR_INVALID_PARAMETER;
    }
    
    auto result = client->impl->authenticate(password, timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    return ITSCAM_OK;
}

//=========================================================================
// Event Subscription
//=========================================================================

ITSCAM_ErrorCode ITSCAM_Client_subscribe(
    ITSCAM_Client* client,
    const ITSCAM_EventSubscription* events,
    uint32_t timeoutMs)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    if (!events) {
        setLastError("Null events");
        return ITSCAM_ERROR_INVALID_PARAMETER;
    }
    
    itscam::EventSubscription sub;
    sub.pipeline         = events->pipeline != 0;
    sub.triggerMetadata  = events->triggerMetadata != 0;
    sub.triggerImage     = events->triggerImage != 0;
    sub.snapshotMetadata = events->snapshotMetadata != 0;
    sub.snapshotImage    = events->snapshotImage != 0;
    sub.previewMetadata  = events->previewMetadata != 0;
    sub.previewImage     = events->previewImage != 0;
    sub.gpio             = events->gpio != 0;
    sub.serial1          = events->serial1 != 0;
    sub.serial2          = events->serial2 != 0;
    
    auto result = client->impl->subscribe(sub, timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    return ITSCAM_OK;
}

ITSCAM_ErrorCode ITSCAM_Client_subscribeCaptures(
    ITSCAM_Client* client,
    const ITSCAM_CaptureSubscriptionConfig* config,
    uint32_t timeoutMs)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }

    itscam::CaptureSubscriptionConfig captureCfg;
    if (config) {
        captureCfg.includeTrigger = config->includeTrigger != 0;
        captureCfg.includeSnapshot = config->includeSnapshot != 0;
        captureCfg.includeMetadata = config->includeMetadata != 0;
        captureCfg.embedComments = config->embedComments != 0;
        captureCfg.embedExif = config->embedExif != 0;
        captureCfg.embedSignature = config->embedSignature != 0;
        captureCfg.triggerQuality = config->triggerQuality;
        captureCfg.snapshotQuality = config->snapshotQuality;
    }

    auto result = client->impl->subscribeCaptures(captureCfg, timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    return ITSCAM_OK;
}

//=========================================================================
// Capture
//=========================================================================

ITSCAM_ErrorCode ITSCAM_Client_captureSnapshot(
    ITSCAM_Client* client,
    const ITSCAM_SnapshotRequest* request,
    uint32_t timeoutMs,
    ITSCAM_CaptureResultArray** outResult)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    if (!outResult) {
        setLastError("Null output pointer");
        return ITSCAM_ERROR_INVALID_PARAMETER;
    }
    
    itscam::SnapshotRequest req;
    // C API uses defaults; for advanced options use C++ API
    (void)request;  // Reserved for future use
    
    auto result = client->impl->captureSnapshot(req, timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    
    try {
        auto* arr = new ITSCAM_CaptureResultArray();
        arr->results = std::move(result.value());
        *outResult = arr;
        return ITSCAM_OK;
    } catch (...) {
        setLastError("Memory allocation failed");
        return ITSCAM_ERROR_ALLOCATION_FAILED;
    }
}

ITSCAM_ErrorCode ITSCAM_Client_getLastFrame(
    ITSCAM_Client* client,
    int quality,
    uint32_t timeoutMs,
    ITSCAM_ByteArray** outJpeg)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    if (!outJpeg) {
        setLastError("Null output pointer");
        return ITSCAM_ERROR_INVALID_PARAMETER;
    }
    
    auto result = client->impl->getLastFrame(quality, timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    
    try {
        auto* arr = new ITSCAM_ByteArray();
        arr->data = std::move(result.value());
        *outJpeg = arr;
        return ITSCAM_OK;
    } catch (...) {
        setLastError("Memory allocation failed");
        return ITSCAM_ERROR_ALLOCATION_FAILED;
    }
}

//=========================================================================
// Profile Management
//=========================================================================

ITSCAM_ErrorCode ITSCAM_Client_getActiveProfileId(
    ITSCAM_Client* client,
    uint32_t timeoutMs,
    uint32_t* outId)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    if (!outId) {
        setLastError("Null output pointer");
        return ITSCAM_ERROR_INVALID_PARAMETER;
    }
    
    auto result = client->impl->getActiveProfileId(timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    
    *outId = result.value();
    return ITSCAM_OK;
}

ITSCAM_ErrorCode ITSCAM_Client_setActiveProfile(
    ITSCAM_Client* client,
    uint32_t profileId,
    uint32_t timeoutMs)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    
    auto result = client->impl->setActiveProfile(profileId, timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    return ITSCAM_OK;
}

ITSCAM_ErrorCode ITSCAM_Client_listProfiles(
    ITSCAM_Client* client,
    uint32_t timeoutMs,
    ITSCAM_ProfileArray** outProfiles)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    if (!outProfiles) {
        setLastError("Null output pointer");
        return ITSCAM_ERROR_INVALID_PARAMETER;
    }
    
    auto result = client->impl->listProfiles(timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    
    try {
        auto* arr = new ITSCAM_ProfileArray();
        arr->profiles = std::move(result.value());
        // Copy strings for stable pointers
        for (const auto& p : arr->profiles) {
            arr->names.push_back(p.name);
            arr->descriptions.push_back(p.description);
        }
        *outProfiles = arr;
        return ITSCAM_OK;
    } catch (...) {
        setLastError("Memory allocation failed");
        return ITSCAM_ERROR_ALLOCATION_FAILED;
    }
}

//=========================================================================
// System
//=========================================================================

ITSCAM_ErrorCode ITSCAM_Client_reboot(
    ITSCAM_Client* client,
    uint32_t timeoutMs)
{
    if (!client || !client->impl) {
        setLastError("Null client handle");
        return ITSCAM_ERROR_NULL_HANDLE;
    }
    
    auto result = client->impl->reboot(timeoutMs);
    if (!result) {
        setLastError("%s", result.error().message.c_str());
        return translateError(result.error());
    }
    return ITSCAM_OK;
}

//=========================================================================
// Callbacks
//=========================================================================

void ITSCAM_Client_onTriggerImage(
    ITSCAM_Client* client,
    ITSCAM_CaptureCallback callback,
    void* userData)
{
    if (!client || !client->impl) return;
    
    client->triggerImageCb = callback;
    client->triggerImageUserData = userData;
    
    if (callback) {
        client->impl->onTriggerImage([client](const itscam::CaptureResult& cr) {
            if (client->triggerImageCb) {
                ITSCAM_CaptureResult wrapped;
                wrapped.data = cr;
                client->triggerImageCb(&wrapped, client->triggerImageUserData);
            }
        });
    } else {
        client->impl->onTriggerImage(nullptr);
    }
}

void ITSCAM_Client_onSnapshotImage(
    ITSCAM_Client* client,
    ITSCAM_CaptureCallback callback,
    void* userData)
{
    if (!client || !client->impl) return;
    
    client->snapshotImageCb = callback;
    client->snapshotImageUserData = userData;
    
    if (callback) {
        client->impl->onSnapshotImage([client](const itscam::CaptureResult& cr) {
            if (client->snapshotImageCb) {
                ITSCAM_CaptureResult wrapped;
                wrapped.data = cr;
                client->snapshotImageCb(&wrapped, client->snapshotImageUserData);
            }
        });
    } else {
        client->impl->onSnapshotImage(nullptr);
    }
}

void ITSCAM_Client_onDisconnect(
    ITSCAM_Client* client,
    ITSCAM_DisconnectCallback callback,
    void* userData)
{
    if (!client || !client->impl) return;
    
    client->disconnectCb = callback;
    client->disconnectUserData = userData;
    
    if (callback) {
        client->impl->onDisconnect([client](const std::string& reason) {
            if (client->disconnectCb) {
                client->disconnectCb(reason.c_str(), client->disconnectUserData);
            }
        });
    } else {
        client->impl->onDisconnect(nullptr);
    }
}

void ITSCAM_Client_onConnectionState(
    ITSCAM_Client* client,
    ITSCAM_ConnectionStateCallback callback,
    void* userData)
{
    if (!client || !client->impl) return;
    
    client->connectionStateCb = callback;
    client->connectionStateUserData = userData;
    
    if (callback) {
        client->impl->onConnectionStateChanged(
            [client](itscam::ConnectionState state, const std::string& reason) {
                if (client->connectionStateCb) {
                    client->connectionStateCb(static_cast<int>(state), reason.c_str(),
                                              client->connectionStateUserData);
                }
            });
    } else {
        client->impl->onConnectionStateChanged(nullptr);
    }
}

void ITSCAM_Client_onLog(
    ITSCAM_Client* client,
    ITSCAM_LogCallback callback,
    void* userData)
{
    if (!client || !client->impl) return;
    
    client->logCb = callback;
    client->logUserData = userData;
    
    if (callback) {
        client->impl->setLogHandler([client](itscam::LogLevel level,
                                              const std::string& msg) {
            if (client->logCb) {
                client->logCb(static_cast<int>(level), msg.c_str(),
                              client->logUserData);
            }
        });
    } else {
        client->impl->setLogHandler(nullptr);
    }
}

//=========================================================================
// CaptureResult Accessors
//=========================================================================

size_t ITSCAM_CaptureResultArray_size(const ITSCAM_CaptureResultArray* array) {
    return array ? array->results.size() : 0;
}

const ITSCAM_CaptureResult* ITSCAM_CaptureResultArray_get(
    const ITSCAM_CaptureResultArray* array,
    size_t index)
{
    if (!array || index >= array->results.size()) return nullptr;
    // Return pointer to internal storage (valid until array is destroyed)
    static thread_local ITSCAM_CaptureResult s_result;
    s_result.data = array->results[index];
    return &s_result;
}

void ITSCAM_CaptureResultArray_destroy(ITSCAM_CaptureResultArray* array) {
    delete array;
}

ITSCAM_FrameInfo ITSCAM_CaptureResult_getInfo(const ITSCAM_CaptureResult* result) {
    ITSCAM_FrameInfo info = {};
    if (result) {
        info.requestId = result->data.info.requestId;
        info.frameCount = result->data.info.frameCount;
        info.multiExpIndex = result->data.info.multiExpIndex;
        info.multiExpLength = result->data.info.multiExpLength;
        info.shutter = result->data.info.shutter;
        info.gain = result->data.info.gain;
        info.width = result->data.info.originalSize.width;
        info.height = result->data.info.originalSize.height;
        // Copy timestamp (itscam::Timestamp uses min, sec, msec)
        info.timestamp.year = result->data.info.timestamp.year;
        info.timestamp.month = result->data.info.timestamp.month;
        info.timestamp.day = result->data.info.timestamp.day;
        info.timestamp.hour = result->data.info.timestamp.hour;
        info.timestamp.minute = result->data.info.timestamp.min;
        info.timestamp.second = result->data.info.timestamp.sec;
        info.timestamp.millisecond = result->data.info.timestamp.msec;
        info.timestamp.timezone_offset = 0;  // itscam::Timestamp doesn't have timezone
    }
    return info;
}

const uint8_t* ITSCAM_CaptureResult_getJpeg(
    const ITSCAM_CaptureResult* result,
    size_t* outSize)
{
    if (!result) {
        if (outSize) *outSize = 0;
        return nullptr;
    }
    if (outSize) *outSize = result->data.jpeg.size();
    return result->data.jpeg.empty() ? nullptr : result->data.jpeg.data();
}

size_t ITSCAM_CaptureResult_getPlateCount(const ITSCAM_CaptureResult* result) {
    return result ? result->data.info.plates.size() : 0;
}

const char* ITSCAM_CaptureResult_getPlate(
    const ITSCAM_CaptureResult* result,
    size_t index)
{
    if (!result || index >= result->data.info.plates.size()) return nullptr;
    return result->data.info.plates[index].plate.c_str();
}

//=========================================================================
// ByteArray Accessors
//=========================================================================

size_t ITSCAM_ByteArray_size(const ITSCAM_ByteArray* array) {
    return array ? array->data.size() : 0;
}

const uint8_t* ITSCAM_ByteArray_data(const ITSCAM_ByteArray* array) {
    return array && !array->data.empty() ? array->data.data() : nullptr;
}

void ITSCAM_ByteArray_destroy(ITSCAM_ByteArray* array) {
    delete array;
}

//=========================================================================
// ProfileArray Accessors
//=========================================================================

size_t ITSCAM_ProfileArray_size(const ITSCAM_ProfileArray* array) {
    return array ? array->profiles.size() : 0;
}

int ITSCAM_ProfileArray_get(
    const ITSCAM_ProfileArray* array,
    size_t index,
    ITSCAM_ProfileInfo* outInfo)
{
    if (!array || !outInfo || index >= array->profiles.size()) return 0;
    
    const auto& p = array->profiles[index];
    outInfo->id = p.id;
    outInfo->name = array->names[index].c_str();
    outInfo->description = array->descriptions[index].c_str();
    outInfo->isActive = p.active ? 1 : 0;
    return 1;
}

void ITSCAM_ProfileArray_destroy(ITSCAM_ProfileArray* array) {
    delete array;
}

//=========================================================================
// JPEG Comment Utilities
//=========================================================================

size_t ITSCAM_Jpeg_extractComment(
    const uint8_t* jpegData,
    size_t jpegSize,
    char* outBuf,
    size_t bufSize)
{
    auto comment = itscam::extractJpegComment(jpegData, jpegSize);
    if (comment.empty()) return 0;

    if (outBuf && bufSize > 0) {
        size_t copyLen = comment.size() < bufSize ? comment.size() : bufSize - 1;
        std::memcpy(outBuf, comment.data(), copyLen);
        outBuf[copyLen] = '\0';
    }
    return comment.size();
}

//=========================================================================
// Utility Functions
//=========================================================================

ITSCAM_Timestamp ITSCAM_getSystemLocalTime(void) {
    auto ts = itscam_utils::getSystemLocalTime();
    ITSCAM_Timestamp result;
    result.year = ts.year;
    result.month = ts.month;
    result.day = ts.day;
    result.hour = ts.hour;
    result.minute = ts.minute;
    result.second = ts.second;
    result.millisecond = ts.millisecond;
    result.timezone_offset = ts.timezone_offset;
    return result;
}

ITSCAM_Timestamp ITSCAM_getSystemUtcTime(void) {
    auto ts = itscam_utils::getSystemUtcTime();
    ITSCAM_Timestamp result;
    result.year = ts.year;
    result.month = ts.month;
    result.day = ts.day;
    result.hour = ts.hour;
    result.minute = ts.minute;
    result.second = ts.second;
    result.millisecond = ts.millisecond;
    result.timezone_offset = ts.timezone_offset;
    return result;
}

uint64_t ITSCAM_getEpochTime(void) {
    return itscam_utils::getEpochTime();
}

uint64_t ITSCAM_getEpochTimeMs(void) {
    return itscam_utils::getEpochTimeMs();
}

int ITSCAM_storeFile(
    const char* path,
    const uint8_t* data,
    size_t size,
    int overwrite)
{
    return itscam_utils::storeFile(path, data, size, overwrite != 0) ? 1 : 0;
}

int ITSCAM_createFolder(const char* path, int recursive) {
    return itscam_utils::createFolder(path, recursive != 0) ? 1 : 0;
}

int ITSCAM_fileExists(const char* path) {
    return itscam_utils::fileExists(path) ? 1 : 0;
}

int ITSCAM_folderExists(const char* path) {
    return itscam_utils::folderExists(path) ? 1 : 0;
}

const char* ITSCAM_getLastError(void) {
    return s_lastError;
}

const char* ITSCAM_getVersion(void) {
    return ITSCAM_SDK_VERSION_FULL;
}
