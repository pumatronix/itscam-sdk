/*
 *  itscam_sdk_c.h
 *
 *  ITSCAM Client SDK - C API Wrapper
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  C-compatible API for use with FFI bindings (Python/Go).
 *  All functions use opaque handles and basic C types.
 */
#ifndef ITSCAM_SDK_C_H
#define ITSCAM_SDK_C_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Export Macros
 * ============================================================================ */

#if defined(__GNUC__) && defined(__linux__)
#   define ITSCAM_C_API __attribute__((visibility("default")))
#elif defined(_WIN32) || defined(__MINGW32__)
#   ifdef ITSCAM_SDK_BUILDING
#       define ITSCAM_C_API __declspec(dllexport)
#   elif defined(ITSCAM_SDK_STATIC)
#       define ITSCAM_C_API
#   else
#       define ITSCAM_C_API __declspec(dllimport)
#   endif
#else
#   define ITSCAM_C_API
#endif

/* ============================================================================
 *  Error Codes
 * ============================================================================ */

typedef enum ITSCAM_ErrorCode {
    ITSCAM_OK = 0,
    ITSCAM_ERROR_CONNECTION_FAILED = 1,
    ITSCAM_ERROR_TIMEOUT = 2,
    ITSCAM_ERROR_NOT_AUTHENTICATED = 3,
    ITSCAM_ERROR_INVALID_PARAMETER = 4,
    ITSCAM_ERROR_SERVER_ERROR = 5,
    ITSCAM_ERROR_DISCONNECTED = 6,
    ITSCAM_ERROR_UNKNOWN = 7,
    ITSCAM_ERROR_NULL_HANDLE = 8,
    ITSCAM_ERROR_ALLOCATION_FAILED = 9
} ITSCAM_ErrorCode;

/* ============================================================================
 *  Opaque Handles
 * ============================================================================ */

typedef struct ITSCAM_Client ITSCAM_Client;
typedef struct ITSCAM_CaptureResult ITSCAM_CaptureResult;
typedef struct ITSCAM_CaptureResultArray ITSCAM_CaptureResultArray;
typedef struct ITSCAM_ByteArray ITSCAM_ByteArray;
typedef struct ITSCAM_ProfileArray ITSCAM_ProfileArray;
typedef struct ITSCAM_JsonObject ITSCAM_JsonObject;

/* ============================================================================
 *  Timestamp Structure
 * ============================================================================ */

typedef struct ITSCAM_Timestamp {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t millisecond;
    int32_t  timezone_offset;
} ITSCAM_Timestamp;

/* ============================================================================
 *  Frame Info Structure (simplified)
 * ============================================================================ */

typedef struct ITSCAM_FrameInfo {
    uint64_t requestId;
    uint64_t frameCount;
    int32_t  multiExpIndex;
    int32_t  multiExpLength;
    int32_t  shutter;
    float    gain;
    uint32_t width;          /* Original image width */
    uint32_t height;         /* Original image height */
    ITSCAM_Timestamp timestamp;
} ITSCAM_FrameInfo;

/* ============================================================================
 *  Auto-Reconnect Configuration
 * ============================================================================ */

typedef struct ITSCAM_AutoReconnectConfig {
    int      enabled;       /* 0 = false, nonzero = true */
    uint32_t intervalMs;    /* Milliseconds between reconnect attempts */
    uint32_t maxRetries;    /* 0 = unlimited */
} ITSCAM_AutoReconnectConfig;

/* ============================================================================
 *  Event Subscription Configuration
 * ============================================================================ */

typedef struct ITSCAM_EventSubscription {
    int pipeline;           /* 0 = false, nonzero = true */
    int triggerMetadata;
    int triggerImage;
    int snapshotMetadata;
    int snapshotImage;
    int previewMetadata;
    int previewImage;
    int gpio;
    int serial1;
    int serial2;
} ITSCAM_EventSubscription;

/* ============================================================================
 *  High-Level Capture Subscription Configuration
 * ============================================================================ */

typedef struct ITSCAM_CaptureSubscriptionConfig {
    int includeTrigger;     /* 0 = false, nonzero = true */
    int includeSnapshot;
    int includeMetadata;
    int embedComments;
    int embedExif;
    int embedSignature;
    int triggerQuality;     /* -1 = unchanged */
    int snapshotQuality;    /* -1 = unchanged */
} ITSCAM_CaptureSubscriptionConfig;

/* ============================================================================
 *  Snapshot Request Configuration
 * ============================================================================ */

typedef struct ITSCAM_SnapshotRequest {
    int reserved;           /* Reserved for future use, set to 0 */
    /* Note: For advanced multi-exposure/overlay settings, use C++ API */
} ITSCAM_SnapshotRequest;

/* ============================================================================
 *  Profile Info Structure
 * ============================================================================ */

typedef struct ITSCAM_ProfileInfo {
    uint32_t    id;
    const char* name;       /* Valid until ITSCAM_ProfileArray_destroy */
    const char* description;
    int         isActive;
} ITSCAM_ProfileInfo;

/* ============================================================================
 *  Callback Function Types
 * ============================================================================ */

typedef void (*ITSCAM_CaptureCallback)(const ITSCAM_CaptureResult* result, 
                                        void* userData);
typedef void (*ITSCAM_FrameInfoCallback)(const ITSCAM_FrameInfo* info,
                                          void* userData);
typedef void (*ITSCAM_DisconnectCallback)(const char* reason, void* userData);
typedef void (*ITSCAM_ConnectionStateCallback)(int state, const char* reason,
                                                void* userData);
typedef void (*ITSCAM_LogCallback)(int level, const char* message, void* userData);

/* ============================================================================
 *  Client Lifecycle
 * ============================================================================ */

/**
 * @brief Create a new ITSCAM client instance.
 * @return Handle to the client, or NULL on failure.
 */
ITSCAM_C_API ITSCAM_Client* ITSCAM_Client_create(void);

/**
 * @brief Destroy a client instance and free resources.
 * @param client Handle to destroy (may be NULL).
 */
ITSCAM_C_API void ITSCAM_Client_destroy(ITSCAM_Client* client);

/* ============================================================================
 *  Connection
 * ============================================================================ */

/**
 * @brief Connect to an ITSCAM device.
 * @param client   Client handle.
 * @param address  IP address (e.g., "192.168.1.100").
 * @param port     TCP port (default 60000).
 * @param timeoutMs Connection timeout in milliseconds.
 * @param reconnect Auto-reconnect configuration (may be NULL for defaults).
 * @return ITSCAM_OK on success, error code on failure.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_connect(
    ITSCAM_Client* client,
    const char* address,
    uint16_t port,
    uint32_t timeoutMs,
    const ITSCAM_AutoReconnectConfig* reconnect);

/**
 * @brief Disconnect from the device.
 * @param client Client handle.
 */
ITSCAM_C_API void ITSCAM_Client_disconnect(ITSCAM_Client* client);

/**
 * @brief Check if the client is connected.
 * @param client Client handle.
 * @return Nonzero if connected, 0 if disconnected.
 */
ITSCAM_C_API int ITSCAM_Client_isConnected(ITSCAM_Client* client);

/* ============================================================================
 *  Authentication
 * ============================================================================ */

/**
 * @brief Authenticate with the device.
 * @param client    Client handle.
 * @param password  Password string.
 * @param timeoutMs Request timeout in milliseconds.
 * @return ITSCAM_OK on success, error code on failure.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_authenticate(
    ITSCAM_Client* client,
    const char* password,
    uint32_t timeoutMs);

/* ============================================================================
 *  Event Subscription
 * ============================================================================ */

/**
 * @brief Subscribe to events.
 * @param client    Client handle.
 * @param events    Event subscription configuration.
 * @param timeoutMs Request timeout in milliseconds.
 * @return ITSCAM_OK on success, error code on failure.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_subscribe(
    ITSCAM_Client* client,
    const ITSCAM_EventSubscription* events,
    uint32_t timeoutMs);

/**
 * @brief Subscribe to capture events using the SDK high-level defaults.
 * @param client    Client handle.
 * @param config    Capture subscription configuration (may be NULL for defaults).
 * @param timeoutMs Request timeout in milliseconds.
 * @return ITSCAM_OK on success, error code on failure.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_subscribeCaptures(
    ITSCAM_Client* client,
    const ITSCAM_CaptureSubscriptionConfig* config,
    uint32_t timeoutMs);

/* ============================================================================
 *  Capture
 * ============================================================================ */

/**
 * @brief Capture a snapshot.
 * @param client    Client handle.
 * @param request   Snapshot request configuration (may be NULL for defaults).
 * @param timeoutMs Request timeout in milliseconds.
 * @param outResult Pointer to receive the result array handle.
 * @return ITSCAM_OK on success, error code on failure.
 * @note Caller must call ITSCAM_CaptureResultArray_destroy on outResult.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_captureSnapshot(
    ITSCAM_Client* client,
    const ITSCAM_SnapshotRequest* request,
    uint32_t timeoutMs,
    ITSCAM_CaptureResultArray** outResult);

/**
 * @brief Get the last captured frame.
 * @param client    Client handle.
 * @param quality   JPEG quality (1-100).
 * @param timeoutMs Request timeout in milliseconds.
 * @param outJpeg   Pointer to receive the JPEG data handle.
 * @return ITSCAM_OK on success, error code on failure.
 * @note Caller must call ITSCAM_ByteArray_destroy on outJpeg.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_getLastFrame(
    ITSCAM_Client* client,
    int quality,
    uint32_t timeoutMs,
    ITSCAM_ByteArray** outJpeg);

/* ============================================================================
 *  Profile Management
 * ============================================================================ */

/**
 * @brief Get the active profile ID.
 * @param client    Client handle.
 * @param timeoutMs Request timeout.
 * @param outId     Pointer to receive the profile ID.
 * @return ITSCAM_OK on success, error code on failure.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_getActiveProfileId(
    ITSCAM_Client* client,
    uint32_t timeoutMs,
    uint32_t* outId);

/**
 * @brief Set the active profile.
 * @param client    Client handle.
 * @param profileId Profile ID to activate.
 * @param timeoutMs Request timeout.
 * @return ITSCAM_OK on success, error code on failure.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_setActiveProfile(
    ITSCAM_Client* client,
    uint32_t profileId,
    uint32_t timeoutMs);

/**
 * @brief List all profiles.
 * @param client    Client handle.
 * @param timeoutMs Request timeout.
 * @param outProfiles Pointer to receive the profile array handle.
 * @return ITSCAM_OK on success, error code on failure.
 * @note Caller must call ITSCAM_ProfileArray_destroy on outProfiles.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_listProfiles(
    ITSCAM_Client* client,
    uint32_t timeoutMs,
    ITSCAM_ProfileArray** outProfiles);

/* ============================================================================
 *  System
 * ============================================================================ */

/**
 * @brief Reboot the device.
 * @param client    Client handle.
 * @param timeoutMs Request timeout.
 * @return ITSCAM_OK on success, error code on failure.
 */
ITSCAM_C_API ITSCAM_ErrorCode ITSCAM_Client_reboot(
    ITSCAM_Client* client,
    uint32_t timeoutMs);

/* ============================================================================
 *  Callbacks
 * ============================================================================ */

/**
 * @brief Set the trigger image callback.
 * @param client   Client handle.
 * @param callback Callback function (NULL to unset).
 * @param userData User data passed to the callback.
 */
ITSCAM_C_API void ITSCAM_Client_onTriggerImage(
    ITSCAM_Client* client,
    ITSCAM_CaptureCallback callback,
    void* userData);

/**
 * @brief Set the snapshot image callback.
 * @param client   Client handle.
 * @param callback Callback function (NULL to unset).
 * @param userData User data passed to the callback.
 */
ITSCAM_C_API void ITSCAM_Client_onSnapshotImage(
    ITSCAM_Client* client,
    ITSCAM_CaptureCallback callback,
    void* userData);

/**
 * @brief Set the disconnect callback.
 * @param client   Client handle.
 * @param callback Callback function (NULL to unset).
 * @param userData User data passed to the callback.
 */
ITSCAM_C_API void ITSCAM_Client_onDisconnect(
    ITSCAM_Client* client,
    ITSCAM_DisconnectCallback callback,
    void* userData);

/**
 * @brief Set the connection state callback.
 * @param client   Client handle.
 * @param callback Callback function (NULL to unset).
 * @param userData User data passed to the callback.
 */
ITSCAM_C_API void ITSCAM_Client_onConnectionState(
    ITSCAM_Client* client,
    ITSCAM_ConnectionStateCallback callback,
    void* userData);

/**
 * @brief Set the log callback.
 * @param client   Client handle.
 * @param callback Callback function (NULL to unset).
 * @param userData User data passed to the callback.
 */
ITSCAM_C_API void ITSCAM_Client_onLog(
    ITSCAM_Client* client,
    ITSCAM_LogCallback callback,
    void* userData);

/* ============================================================================
 *  CaptureResult Accessors
 * ============================================================================ */

/**
 * @brief Get the number of results in an array.
 */
ITSCAM_C_API size_t ITSCAM_CaptureResultArray_size(
    const ITSCAM_CaptureResultArray* array);

/**
 * @brief Get a result from an array.
 * @param array Array handle.
 * @param index Index (0-based).
 * @return Result handle, or NULL if out of bounds.
 * @note The returned handle is valid until the array is destroyed.
 */
ITSCAM_C_API const ITSCAM_CaptureResult* ITSCAM_CaptureResultArray_get(
    const ITSCAM_CaptureResultArray* array,
    size_t index);

/**
 * @brief Destroy a capture result array.
 */
ITSCAM_C_API void ITSCAM_CaptureResultArray_destroy(
    ITSCAM_CaptureResultArray* array);

/**
 * @brief Get the frame info from a capture result.
 */
ITSCAM_C_API ITSCAM_FrameInfo ITSCAM_CaptureResult_getInfo(
    const ITSCAM_CaptureResult* result);

/**
 * @brief Get the JPEG data pointer from a capture result.
 * @param result Result handle.
 * @param outSize Pointer to receive the size (may be NULL).
 * @return Pointer to JPEG data, or NULL if empty.
 * @note The pointer is valid until the result array is destroyed.
 */
ITSCAM_C_API const uint8_t* ITSCAM_CaptureResult_getJpeg(
    const ITSCAM_CaptureResult* result,
    size_t* outSize);

/**
 * @brief Get the number of plates recognized.
 */
ITSCAM_C_API size_t ITSCAM_CaptureResult_getPlateCount(
    const ITSCAM_CaptureResult* result);

/**
 * @brief Get a recognized plate string.
 * @param result Result handle.
 * @param index  Plate index (0-based).
 * @return Plate string, or NULL if out of bounds.
 */
ITSCAM_C_API const char* ITSCAM_CaptureResult_getPlate(
    const ITSCAM_CaptureResult* result,
    size_t index);

/* ============================================================================
 *  ByteArray Accessors
 * ============================================================================ */

/**
 * @brief Get the size of a byte array.
 */
ITSCAM_C_API size_t ITSCAM_ByteArray_size(const ITSCAM_ByteArray* array);

/**
 * @brief Get the data pointer from a byte array.
 */
ITSCAM_C_API const uint8_t* ITSCAM_ByteArray_data(const ITSCAM_ByteArray* array);

/**
 * @brief Destroy a byte array.
 */
ITSCAM_C_API void ITSCAM_ByteArray_destroy(ITSCAM_ByteArray* array);

/* ============================================================================
 *  ProfileArray Accessors
 * ============================================================================ */

/**
 * @brief Get the number of profiles in an array.
 */
ITSCAM_C_API size_t ITSCAM_ProfileArray_size(const ITSCAM_ProfileArray* array);

/**
 * @brief Get a profile from an array.
 * @param array Array handle.
 * @param index Index (0-based).
 * @param outInfo Pointer to receive the profile info.
 * @return Nonzero on success, 0 if out of bounds.
 */
ITSCAM_C_API int ITSCAM_ProfileArray_get(
    const ITSCAM_ProfileArray* array,
    size_t index,
    ITSCAM_ProfileInfo* outInfo);

/**
 * @brief Destroy a profile array.
 */
ITSCAM_C_API void ITSCAM_ProfileArray_destroy(ITSCAM_ProfileArray* array);

/* ============================================================================
 *  Utility Functions
 * ============================================================================ */

/**
 * @brief Get the current local time.
 */
ITSCAM_C_API ITSCAM_Timestamp ITSCAM_getSystemLocalTime(void);

/**
 * @brief Get the current UTC time.
 */
ITSCAM_C_API ITSCAM_Timestamp ITSCAM_getSystemUtcTime(void);

/**
 * @brief Get the epoch time in seconds.
 */
ITSCAM_C_API uint64_t ITSCAM_getEpochTime(void);

/**
 * @brief Get the epoch time in milliseconds.
 */
ITSCAM_C_API uint64_t ITSCAM_getEpochTimeMs(void);

/**
 * @brief Store data to a file.
 * @param path      File path.
 * @param data      Data buffer.
 * @param size      Data size in bytes.
 * @param overwrite Overwrite if file exists.
 * @return Nonzero on success, 0 on failure.
 */
ITSCAM_C_API int ITSCAM_storeFile(
    const char* path,
    const uint8_t* data,
    size_t size,
    int overwrite);

/**
 * @brief Create a directory.
 * @param path      Directory path.
 * @param recursive Create parent directories if needed.
 * @return Nonzero on success, 0 on failure.
 */
ITSCAM_C_API int ITSCAM_createFolder(const char* path, int recursive);

/**
 * @brief Check if a file exists.
 */
ITSCAM_C_API int ITSCAM_fileExists(const char* path);

/**
 * @brief Check if a directory exists.
 */
ITSCAM_C_API int ITSCAM_folderExists(const char* path);

/**
 * @brief Get the last error message.
 * @return Error message string (thread-local, valid until next SDK call).
 */
ITSCAM_C_API const char* ITSCAM_getLastError(void);

/**
 * @brief Get the SDK version string.
 */
ITSCAM_C_API const char* ITSCAM_getVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* ITSCAM_SDK_C_H */
