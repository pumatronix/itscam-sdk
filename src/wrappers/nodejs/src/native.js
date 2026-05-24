/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Locate and load libitscam_sdk via koffi, then declare the C API
 * function signatures used by the high-level wrappers.
 *
 * koffi is a maintained FFI library for Node.js.  It is the modern
 * replacement for ffi-napi and works on Node 16+ on Linux, macOS and
 * Windows.
 */
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const koffi = require('koffi');

function findNativeLibrary() {
    const override = process.env.ITSCAM_SDK_LIBRARY;
    if (override && fs.existsSync(override)) {
        return override;
    }

    const platform = process.platform;
    const arch = process.arch;
    let names;
    if (platform === 'win32') {
        names = ['itscam_sdk.dll'];
    } else if (platform === 'darwin') {
        names = ['libitscam_sdk.dylib'];
    } else {
        names = ['libitscam_sdk.so'];
    }

    const candidates = [
        path.join(__dirname, '..', 'native', `${platform}-${arch}`),
        path.join(__dirname, '..', 'native'),
        // dev tree: SDK shared library produced by `make lib`
        path.join(__dirname, '..', '..', '..', 'core', 'build', 'linux'),
        path.join(__dirname, '..', '..', '..', 'core', 'build', 'win-x64'),
    ];

    if (process.env.LD_LIBRARY_PATH) {
        for (const p of process.env.LD_LIBRARY_PATH.split(path.delimiter)) {
            if (p) candidates.push(p);
        }
    }
    if (platform === 'win32' && process.env.PATH) {
        for (const p of process.env.PATH.split(path.delimiter)) {
            if (p) candidates.push(p);
        }
    }

    for (const dir of candidates) {
        for (const name of names) {
            const full = path.join(dir, name);
            if (fs.existsSync(full)) return full;
        }
    }

    // Let koffi try the system loader as a last resort.
    return names[0];
}

const LIB_PATH = findNativeLibrary();
const lib = koffi.load(LIB_PATH);

// ============================================================================
//  koffi struct layouts (must match itscam_sdk_c.h)
// ============================================================================

const ITSCAM_Timestamp = koffi.struct('ITSCAM_Timestamp', {
    year: 'uint16',
    month: 'uint16',
    day: 'uint16',
    hour: 'uint16',
    minute: 'uint16',
    second: 'uint16',
    millisecond: 'uint16',
    timezoneOffset: 'int32',
});

const ITSCAM_FrameInfo = koffi.struct('ITSCAM_FrameInfo', {
    requestId: 'uint64',
    frameCount: 'uint64',
    multiExpIndex: 'int32',
    multiExpLength: 'int32',
    shutter: 'int32',
    gain: 'float',
    width: 'uint32',
    height: 'uint32',
    timestamp: ITSCAM_Timestamp,
});

const ITSCAM_AutoReconnectConfig = koffi.struct(
    'ITSCAM_AutoReconnectConfig', {
        enabled: 'int',
        intervalMs: 'uint32',
        maxRetries: 'uint32',
    });

const ITSCAM_EventSubscription = koffi.struct(
    'ITSCAM_EventSubscription', {
        pipeline: 'int',
        triggerMetadata: 'int',
        triggerImage: 'int',
        snapshotMetadata: 'int',
        snapshotImage: 'int',
        previewMetadata: 'int',
        previewImage: 'int',
        gpio: 'int',
        serial1: 'int',
        serial2: 'int',
    });

const ITSCAM_CaptureSubscriptionConfig = koffi.struct(
    'ITSCAM_CaptureSubscriptionConfig', {
        includeTrigger: 'int',
        includeSnapshot: 'int',
        includeMetadata: 'int',
        embedComments: 'int',
        embedExif: 'int',
        embedSignature: 'int',
        triggerQuality: 'int',
        snapshotQuality: 'int',
    });

const ITSCAM_SnapshotRequest = koffi.struct('ITSCAM_SnapshotRequest', {
    reserved: 'int',
});

const ITSCAM_ProfileInfo = koffi.struct('ITSCAM_ProfileInfo', {
    id: 'uint32',
    name: 'string',
    description: 'string',
    isActive: 'int',
});

const ITSCAM_CgiSnapshotRequest = koffi.struct('ITSCAM_CgiSnapshotRequest', {
    shutters: 'int32 *',
    shuttersLen: 'size_t',
    gains: 'int32 *',
    gainsLen: 'size_t',
    quality: 'int32',
    mosaic: 'int32',
    format: 'string',
    scenario: 'int32',
    crop: 'string',
    textOverlay: 'string',
    userMetadataKeys: 'string *',
    userMetadataValues: 'string *',
});

const ITSCAM_CgiStreamFrame = koffi.struct('ITSCAM_CgiStreamFrame', {
    sequence: 'uint64',
    mimeType: 'string',
    data: 'uint8 *',
    dataLen: 'size_t',
});

// Callback types (koffi proto + register pattern)
const CaptureCb = koffi.proto(
    'CaptureCallback',
    'void',
    ['void *', 'void *']);
const DisconnectCb = koffi.proto(
    'DisconnectCallback',
    'void',
    ['string', 'void *']);
const ConnStateCb = koffi.proto(
    'ConnectionStateCallback',
    'void',
    ['int', 'string', 'void *']);
const LogCb = koffi.proto(
    'LogCallback',
    'void',
    ['int', 'string', 'void *']);
const StreamCb = koffi.proto(
    'CgiStreamCallback',
    'void',
    [koffi.pointer(ITSCAM_CgiStreamFrame), 'void *']);

// ============================================================================
//  Function bindings
// ============================================================================

const fns = {
    // ----- ITSCAM_Client -----------------------------------------------------
    Client_create: lib.func(
        'void *ITSCAM_Client_create()'),
    Client_destroy: lib.func(
        'void ITSCAM_Client_destroy(void *client)'),
    Client_connect: lib.func(
        'int ITSCAM_Client_connect(void *client, const char *address, '
        + 'uint16 port, uint32 timeoutMs, '
        + koffi.pointer(ITSCAM_AutoReconnectConfig) + ' reconnect)'),
    Client_disconnect: lib.func(
        'void ITSCAM_Client_disconnect(void *client)'),
    Client_isConnected: lib.func(
        'int ITSCAM_Client_isConnected(void *client)'),
    Client_authenticate: lib.func(
        'int ITSCAM_Client_authenticate(void *client, '
        + 'const char *password, uint32 timeoutMs)'),
    Client_subscribe: lib.func(
        'int ITSCAM_Client_subscribe(void *client, '
        + koffi.pointer(ITSCAM_EventSubscription) + ' events, '
        + 'uint32 timeoutMs)'),
    Client_subscribeCaptures: lib.func(
        'int ITSCAM_Client_subscribeCaptures(void *client, '
        + koffi.pointer(ITSCAM_CaptureSubscriptionConfig) + ' config, '
        + 'uint32 timeoutMs)'),
    Client_captureSnapshot: lib.func(
        'int ITSCAM_Client_captureSnapshot(void *client, '
        + koffi.pointer(ITSCAM_SnapshotRequest) + ' request, '
        + 'uint32 timeoutMs, _Out_ void **outResult)'),
    Client_getLastFrame: lib.func(
        'int ITSCAM_Client_getLastFrame(void *client, int quality, '
        + 'uint32 timeoutMs, _Out_ void **outJpeg)'),
    Client_getActiveProfileId: lib.func(
        'int ITSCAM_Client_getActiveProfileId(void *client, '
        + 'uint32 timeoutMs, _Out_ uint32 *outId)'),
    Client_setActiveProfile: lib.func(
        'int ITSCAM_Client_setActiveProfile(void *client, '
        + 'uint32 profileId, uint32 timeoutMs)'),
    Client_listProfiles: lib.func(
        'int ITSCAM_Client_listProfiles(void *client, uint32 timeoutMs, '
        + '_Out_ void **outProfiles)'),
    Client_reboot: lib.func(
        'int ITSCAM_Client_reboot(void *client, uint32 timeoutMs)'),
    Client_onTriggerImage: lib.func(
        'void ITSCAM_Client_onTriggerImage(void *client, CaptureCallback *cb, void *userData)'),
    Client_onSnapshotImage: lib.func(
        'void ITSCAM_Client_onSnapshotImage(void *client, CaptureCallback *cb, void *userData)'),
    Client_onDisconnect: lib.func(
        'void ITSCAM_Client_onDisconnect(void *client, DisconnectCallback *cb, void *userData)'),
    Client_onConnectionState: lib.func(
        'void ITSCAM_Client_onConnectionState(void *client, ConnectionStateCallback *cb, void *userData)'),
    Client_onLog: lib.func(
        'void ITSCAM_Client_onLog(void *client, LogCallback *cb, void *userData)'),

    // CaptureResult accessors
    CaptureResultArray_size: lib.func(
        'size_t ITSCAM_CaptureResultArray_size(void *array)'),
    CaptureResultArray_get: lib.func(
        'void *ITSCAM_CaptureResultArray_get(void *array, size_t index)'),
    CaptureResultArray_destroy: lib.func(
        'void ITSCAM_CaptureResultArray_destroy(void *array)'),
    CaptureResult_getInfo: lib.func(
        ITSCAM_FrameInfo + ' ITSCAM_CaptureResult_getInfo(void *result)'),
    CaptureResult_getJpeg: lib.func(
        'uint8 *ITSCAM_CaptureResult_getJpeg(void *result, _Out_ size_t *outSize)'),
    CaptureResult_getPlateCount: lib.func(
        'size_t ITSCAM_CaptureResult_getPlateCount(void *result)'),
    CaptureResult_getPlate: lib.func(
        'const char *ITSCAM_CaptureResult_getPlate(void *result, size_t index)'),

    ByteArray_size: lib.func(
        'size_t ITSCAM_ByteArray_size(void *array)'),
    ByteArray_data: lib.func(
        'uint8 *ITSCAM_ByteArray_data(void *array)'),
    ByteArray_destroy: lib.func(
        'void ITSCAM_ByteArray_destroy(void *array)'),

    ProfileArray_size: lib.func(
        'size_t ITSCAM_ProfileArray_size(void *array)'),
    ProfileArray_get: lib.func(
        'int ITSCAM_ProfileArray_get(void *array, size_t index, '
        + '_Out_ ' + ITSCAM_ProfileInfo + ' *outInfo)'),
    ProfileArray_destroy: lib.func(
        'void ITSCAM_ProfileArray_destroy(void *array)'),

    // System utilities
    getSystemLocalTime: lib.func(
        ITSCAM_Timestamp + ' ITSCAM_getSystemLocalTime()'),
    getSystemUtcTime: lib.func(
        ITSCAM_Timestamp + ' ITSCAM_getSystemUtcTime()'),
    getEpochTime: lib.func('uint64 ITSCAM_getEpochTime()'),
    getEpochTimeMs: lib.func('uint64 ITSCAM_getEpochTimeMs()'),
    storeFile: lib.func(
        'int ITSCAM_storeFile(const char *path, const uint8 *data, '
        + 'size_t size, int overwrite)'),
    createFolder: lib.func(
        'int ITSCAM_createFolder(const char *path, int recursive)'),
    fileExists: lib.func('int ITSCAM_fileExists(const char *path)'),
    folderExists: lib.func('int ITSCAM_folderExists(const char *path)'),
    getLastError: lib.func('const char *ITSCAM_getLastError()'),
    getVersion: lib.func('const char *ITSCAM_getVersion()'),

    // ----- ITSCAM_RestClient -------------------------------------------------
    Rest_create: lib.func('void *ITSCAM_RestClient_create()'),
    Rest_destroy: lib.func('void ITSCAM_RestClient_destroy(void *client)'),
    Rest_setBaseUrl: lib.func(
        'int ITSCAM_RestClient_setBaseUrl(void *client, const char *host, '
        + 'uint16 port, const char *scheme)'),
    Rest_setApiPrefix: lib.func(
        'void ITSCAM_RestClient_setApiPrefix(void *client, const char *prefix)'),
    Rest_setCaCertFile: lib.func(
        'void ITSCAM_RestClient_setCaCertFile(void *client, const char *pemPath)'),
    Rest_setCaCertData: lib.func(
        'void ITSCAM_RestClient_setCaCertData(void *client, const char *pem)'),
    Rest_setVerifyServerCertificate: lib.func(
        'void ITSCAM_RestClient_setVerifyServerCertificate(void *client, int verify)'),
    Rest_setClientCertificate: lib.func(
        'void ITSCAM_RestClient_setClientCertificate(void *client, '
        + 'const char *certPem, const char *keyPem)'),

    Rest_login: lib.func(
        'int ITSCAM_RestClient_login(void *client, const char *username, '
        + 'const char *password, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_setAuthToken: lib.func(
        'void ITSCAM_RestClient_setAuthToken(void *client, const char *token)'),
    Rest_clearAuthToken: lib.func(
        'void ITSCAM_RestClient_clearAuthToken(void *client)'),

    Rest_httpGet: lib.func(
        'int ITSCAM_RestClient_httpGet(void *client, const char *path, '
        + 'uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_httpPut: lib.func(
        'int ITSCAM_RestClient_httpPut(void *client, const char *path, '
        + 'const char *jsonBody, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_httpPost: lib.func(
        'int ITSCAM_RestClient_httpPost(void *client, const char *path, '
        + 'const char *jsonBody, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_httpDelete: lib.func(
        'int ITSCAM_RestClient_httpDelete(void *client, const char *path, '
        + 'uint32 timeoutMs, _Out_ void **outResponse)'),

    Rest_getProfiles: lib.func(
        'int ITSCAM_RestClient_getProfiles(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Rest_getProfile: lib.func(
        'int ITSCAM_RestClient_getProfile(void *client, int profileId, '
        + 'uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_createProfile: lib.func(
        'int ITSCAM_RestClient_createProfile(void *client, '
        + 'const char *jsonProfile, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_updateProfile: lib.func(
        'int ITSCAM_RestClient_updateProfile(void *client, '
        + 'const char *jsonProfile, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_deleteProfile: lib.func(
        'int ITSCAM_RestClient_deleteProfile(void *client, int profileId, '
        + 'uint32 timeoutMs, _Out_ void **outResponse)'),

    Rest_getVolatileInfo: lib.func(
        'int ITSCAM_RestClient_getVolatileInfo(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Rest_getOcrConfig: lib.func(
        'int ITSCAM_RestClient_getOcrConfig(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Rest_setOcrConfig: lib.func(
        'int ITSCAM_RestClient_setOcrConfig(void *client, '
        + 'const char *jsonConfig, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_getAnalyticsConfig: lib.func(
        'int ITSCAM_RestClient_getAnalyticsConfig(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Rest_setAnalyticsConfig: lib.func(
        'int ITSCAM_RestClient_setAnalyticsConfig(void *client, '
        + 'const char *jsonConfig, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_getClassifierConfig: lib.func(
        'int ITSCAM_RestClient_getClassifierConfig(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Rest_setClassifierConfig: lib.func(
        'int ITSCAM_RestClient_setClassifierConfig(void *client, '
        + 'const char *jsonConfig, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_getLanesConfig: lib.func(
        'int ITSCAM_RestClient_getLanesConfig(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Rest_setLanesConfig: lib.func(
        'int ITSCAM_RestClient_setLanesConfig(void *client, '
        + 'const char *jsonConfig, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_getItscamproConfig: lib.func(
        'int ITSCAM_RestClient_getItscamproConfig(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Rest_setItscamproConfig: lib.func(
        'int ITSCAM_RestClient_setItscamproConfig(void *client, '
        + 'const char *jsonConfig, uint32 timeoutMs, _Out_ void **outResponse)'),
    Rest_getItscamproStatus: lib.func(
        'int ITSCAM_RestClient_getItscamproStatus(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),

    String_data: lib.func('const char *ITSCAM_String_data(void *s)'),
    String_size: lib.func('size_t ITSCAM_String_size(void *s)'),
    String_destroy: lib.func('void ITSCAM_String_destroy(void *s)'),

    // ----- ITSCAM_CgiClient --------------------------------------------------
    Cgi_create: lib.func('void *ITSCAM_CgiClient_create()'),
    Cgi_destroy: lib.func('void ITSCAM_CgiClient_destroy(void *client)'),
    Cgi_setBaseUrl: lib.func(
        'int ITSCAM_CgiClient_setBaseUrl(void *client, const char *host, '
        + 'uint16 port, const char *scheme)'),
    Cgi_setApiPrefix: lib.func(
        'void ITSCAM_CgiClient_setApiPrefix(void *client, const char *prefix)'),
    Cgi_setCaCertFile: lib.func(
        'void ITSCAM_CgiClient_setCaCertFile(void *client, const char *pemPath)'),
    Cgi_setCaCertData: lib.func(
        'void ITSCAM_CgiClient_setCaCertData(void *client, const char *pem)'),
    Cgi_setVerifyServerCertificate: lib.func(
        'void ITSCAM_CgiClient_setVerifyServerCertificate(void *client, int verify)'),
    Cgi_setClientCertificate: lib.func(
        'void ITSCAM_CgiClient_setClientCertificate(void *client, '
        + 'const char *certPem, const char *keyPem)'),
    Cgi_login: lib.func(
        'int ITSCAM_CgiClient_login(void *client, const char *username, '
        + 'const char *password, uint32 timeoutMs)'),
    Cgi_setAuthToken: lib.func(
        'void ITSCAM_CgiClient_setAuthToken(void *client, const char *token)'),
    Cgi_clearAuthToken: lib.func(
        'void ITSCAM_CgiClient_clearAuthToken(void *client)'),
    Cgi_setBasicAuth: lib.func(
        'void ITSCAM_CgiClient_setBasicAuth(void *client, '
        + 'const char *user, const char *password)'),
    Cgi_clearBasicAuth: lib.func(
        'void ITSCAM_CgiClient_clearBasicAuth(void *client)'),

    Cgi_getLastFrame: lib.func(
        'int ITSCAM_CgiClient_getLastFrame(void *client, uint32 timeoutMs, '
        + '_Out_ void **outImage)'),
    Cgi_getSnapshot: lib.func(
        'int ITSCAM_CgiClient_getSnapshot(void *client, '
        + koffi.pointer(ITSCAM_CgiSnapshotRequest) + ' request, '
        + 'uint32 timeoutMs, _Out_ void **outImages)'),
    Cgi_startMjpegStream: lib.func(
        'int ITSCAM_CgiClient_startMjpegStream(void *client, '
        + 'CgiStreamCallback *cb, void *userData, uint32 timeoutMs)'),
    Cgi_stopMjpegStream: lib.func(
        'void ITSCAM_CgiClient_stopMjpegStream(void *client)'),
    Cgi_isMjpegStreamRunning: lib.func(
        'int ITSCAM_CgiClient_isMjpegStreamRunning(void *client)'),
    Cgi_forceTrigger: lib.func(
        'int ITSCAM_CgiClient_forceTrigger(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),
    Cgi_reboot: lib.func(
        'int ITSCAM_CgiClient_reboot(void *client, uint32 timeoutMs, '
        + '_Out_ void **outResponse)'),

    CgiImage_mimeType: lib.func(
        'const char *ITSCAM_CgiImage_mimeType(void *img)'),
    CgiImage_data: lib.func(
        'uint8 *ITSCAM_CgiImage_data(void *img)'),
    CgiImage_size: lib.func('size_t ITSCAM_CgiImage_size(void *img)'),
    CgiImage_destroy: lib.func('void ITSCAM_CgiImage_destroy(void *img)'),

    CgiImageArray_size: lib.func(
        'size_t ITSCAM_CgiImageArray_size(void *arr)'),
    CgiImageArray_get: lib.func(
        'void *ITSCAM_CgiImageArray_get(void *arr, size_t index)'),
    CgiImageArray_destroy: lib.func(
        'void ITSCAM_CgiImageArray_destroy(void *arr)'),
};

module.exports = {
    koffi,
    lib,
    libraryPath: LIB_PATH,
    fns,
    structs: {
        ITSCAM_Timestamp,
        ITSCAM_FrameInfo,
        ITSCAM_AutoReconnectConfig,
        ITSCAM_EventSubscription,
        ITSCAM_CaptureSubscriptionConfig,
        ITSCAM_SnapshotRequest,
        ITSCAM_ProfileInfo,
        ITSCAM_CgiSnapshotRequest,
        ITSCAM_CgiStreamFrame,
    },
    callbacks: {
        CaptureCb,
        DisconnectCb,
        ConnStateCb,
        LogCb,
        StreamCb,
    },
};
