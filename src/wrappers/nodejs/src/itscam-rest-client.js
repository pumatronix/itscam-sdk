/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * REST/JSON client for the ITSCAM webapp on port 80/443.  Always
 * requires a `login()` call before any other method.
 */
'use strict';

const { koffi, fns, callbacks } = require('./native');
const { ItscamError, ErrorCode } = require('./errors');
const { takeString, checkCall } = require('./utils');

class SoftwareUpdateOperation {
    constructor(handle, callbackPtr = null) {
        this._handle = handle;
        this._callbackPtr = callbackPtr;
    }

    [Symbol.dispose]() { this.close(); }

    close() {
        if (!this._handle) return;
        fns.SoftwareUpdateOperation_setCallback(this._handle, null, null);
        fns.SoftwareUpdateOperation_destroy(this._handle);
        this._handle = null;
        this._unregisterCallback();
    }

    _requireOpen() {
        if (!this._handle) throw new Error('SoftwareUpdateOperation closed');
    }

    _unregisterCallback() {
        if (this._callbackPtr) {
            koffi.unregister(this._callbackPtr);
            this._callbackPtr = null;
        }
    }

    status() {
        this._requireOpen();
        const out = [null];
        const rc = fns.SoftwareUpdateOperation_status(this._handle, out);
        const body = takeString(out[0]);
        checkCall(rc, 'softwareUpdateStatus');
        return _decodeJson(body);
    }

    setCallback(callback) {
        this._requireOpen();
        this._unregisterCallback();
        this._callbackPtr = _registerStatusCallback(callback);
        fns.SoftwareUpdateOperation_setCallback(this._handle,
            this._callbackPtr, null);
    }

    wait(timeoutMs = 0) {
        this._requireOpen();
        const out = [null];
        const rc = fns.SoftwareUpdateOperation_wait(this._handle,
            timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'softwareUpdateWait');
        return _decodeJson(body);
    }

    waitAsync(timeoutMs = 0) { return _async(() => this.wait(timeoutMs)); }

    isComplete() {
        this._requireOpen();
        return fns.SoftwareUpdateOperation_isComplete(this._handle) !== 0;
    }

    cancel() {
        this._requireOpen();
        fns.SoftwareUpdateOperation_cancel(this._handle);
    }
}

class ItscamRestClient {
    constructor() {
        this._handle = fns.Rest_create();
        if (!this._handle) {
            throw new ItscamError(ErrorCode.ALLOCATION_FAILED,
                'ITSCAM_RestClient_create returned NULL');
        }
    }

    [Symbol.dispose]() { this.close(); }

    close() {
        if (this._handle) {
            fns.Rest_destroy(this._handle);
            this._handle = null;
        }
    }

    _requireOpen() {
        if (!this._handle) {
            throw new Error('ItscamRestClient closed');
        }
    }

    // ====================================================================
    //  Configuration / TLS
    // ====================================================================

    setBaseUrl(host, port = 80, scheme = 'http') {
        this._requireOpen();
        const rc = fns.Rest_setBaseUrl(this._handle, host, port, scheme);
        checkCall(rc, 'setBaseUrl(' + host + ':' + port + ')');
    }

    setApiPrefix(prefix) {
        this._requireOpen();
        fns.Rest_setApiPrefix(this._handle, prefix || '');
    }

    setCaCertFile(pemPath) {
        this._requireOpen();
        fns.Rest_setCaCertFile(this._handle, pemPath || '');
    }

    setCaCertData(pem) {
        this._requireOpen();
        fns.Rest_setCaCertData(this._handle, pem || '');
    }

    setVerifyServerCertificate(verify) {
        this._requireOpen();
        fns.Rest_setVerifyServerCertificate(this._handle, verify ? 1 : 0);
    }

    setClientCertificate(certPem, keyPem) {
        this._requireOpen();
        fns.Rest_setClientCertificate(this._handle,
            certPem || '', keyPem || '');
    }

    // ====================================================================
    //  Authentication
    // ====================================================================

    login(username, password, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_login(this._handle, username, password,
            timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'login');
        return _decodeJson(body);
    }

    loginAsync(username, password, timeoutMs) {
        return new Promise((resolve, reject) => {
            try { resolve(this.login(username, password, timeoutMs)); }
            catch (e) { reject(e); }
        });
    }

    setAuthToken(token) {
        this._requireOpen();
        fns.Rest_setAuthToken(this._handle, token || '');
    }

    clearAuthToken() {
        this._requireOpen();
        fns.Rest_clearAuthToken(this._handle);
    }

    // ====================================================================
    //  Generic HTTP verbs (parsed JSON, raw fallback for non-JSON bodies)
    // ====================================================================

    get(path, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_httpGet(this._handle, path, timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'GET ' + path);
        return _decodeJson(body);
    }

    put(path, body, timeoutMs = 10000) {
        return this._withBody('PUT', path, body, timeoutMs,
            (h, p, b, t, o) => fns.Rest_httpPut(h, p, b, t, o));
    }

    /**
     * Alias of {@link ItscamRestClient#put} -- send a partial JSON body
     * via HTTP PUT.  The ITSCAM daemon merges the supplied object into
     * the existing configuration; full-document PUT on
     * {@code /api/image/profiles/{id}} is rejected with HTTP 500.
     */
    patchJson(path, partialBody, timeoutMs = 10000) {
        return this.put(path, partialBody, timeoutMs);
    }

    post(path, body, timeoutMs = 10000) {
        return this._withBody('POST', path, body, timeoutMs,
            (h, p, b, t, o) => fns.Rest_httpPost(h, p, b, t, o));
    }

    delete(path, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_httpDelete(this._handle, path, timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'DELETE ' + path);
        return _decodeJson(body);
    }

    uploadSoftwareArchive(swuPath, timeoutMs = 300000, progress = null) {
        this._requireOpen();
        const out = [null];
        let cb = null;
        if (typeof progress === 'function') {
            cb = koffi.register((current, total, _ud) =>
                progress(Number(current), Number(total)) ? 1 : 0,
                koffi.pointer(callbacks.UploadProgressCb));
        }
        try {
            const rc = fns.Rest_uploadSoftwareArchive(this._handle, swuPath,
                timeoutMs, cb, null, out);
            const body = takeString(out[0]);
            checkCall(rc, 'uploadSoftwareArchive');
            return _decodeJson(body);
        } finally {
            if (cb) koffi.unregister(cb);
        }
    }

    restartSoftwareUpdate(timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_restartSoftwareUpdate(this._handle,
            timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'restartSoftwareUpdate');
        return _decodeJson(body);
    }

    startSoftwareUpdate(options, statusCallback = null) {
        this._requireOpen();
        const opts = _softwareUpdateOptions(options);
        const callbackPtr = _registerStatusCallback(statusCallback);
        const out = [null];
        try {
            const rc = fns.Rest_startSoftwareUpdate(this._handle,
                opts.swuPath, opts.uploadTimeoutMs, opts.statusTimeoutMs,
                opts.restartTimeoutMs, opts.requestRestart ? 1 : 0,
                callbackPtr, null, out);
            checkCall(rc, 'startSoftwareUpdate');
            return new SoftwareUpdateOperation(out[0], callbackPtr);
        } catch (err) {
            if (callbackPtr) koffi.unregister(callbackPtr);
            throw err;
        }
    }

    updateSoftware(options, statusCallback = null) {
        this._requireOpen();
        const opts = _softwareUpdateOptions(options);
        const callbackPtr = _registerStatusCallback(statusCallback);
        const out = [null];
        try {
            const rc = fns.Rest_updateSoftware(this._handle,
                opts.swuPath, opts.uploadTimeoutMs, opts.statusTimeoutMs,
                opts.restartTimeoutMs, opts.requestRestart ? 1 : 0,
                callbackPtr, null, out);
            const body = takeString(out[0]);
            checkCall(rc, 'updateSoftware');
            return _decodeJson(body);
        } finally {
            if (callbackPtr) koffi.unregister(callbackPtr);
        }
    }

    getAsync(path, timeoutMs)        { return _async(() => this.get(path, timeoutMs)); }
    putAsync(path, body, timeoutMs)  { return _async(() => this.put(path, body, timeoutMs)); }
    postAsync(path, body, timeoutMs) { return _async(() => this.post(path, body, timeoutMs)); }
    deleteAsync(path, timeoutMs)     { return _async(() => this.delete(path, timeoutMs)); }
    uploadSoftwareArchiveAsync(swuPath, timeoutMs, progress) {
        return _async(() => this.uploadSoftwareArchive(swuPath, timeoutMs,
            progress));
    }
    restartSoftwareUpdateAsync(timeoutMs) {
        return _async(() => this.restartSoftwareUpdate(timeoutMs));
    }
    startSoftwareUpdateAsync(options, statusCallback) {
        return _async(() => this.startSoftwareUpdate(options, statusCallback));
    }
    updateSoftwareAsync(options, statusCallback) {
        return _async(() => this.updateSoftware(options, statusCallback));
    }

    // ====================================================================
    //  Typed convenience helpers
    // ====================================================================

    getProfiles(timeoutMs = 10000) {
        return _typedGet(this, 'getProfiles', timeoutMs,
            (h, t, o) => fns.Rest_getProfiles(h, t, o));
    }

    getProfile(profileId, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_getProfile(this._handle, profileId, timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'getProfile(' + profileId + ')');
        return _decodeJson(body);
    }

    createProfile(profileObj, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_createProfile(this._handle,
            JSON.stringify(profileObj || {}), timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'createProfile');
        return _decodeJson(body);
    }

    updateProfile(profileObj, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_updateProfile(this._handle,
            JSON.stringify(profileObj || {}), timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'updateProfile');
        return _decodeJson(body);
    }

    deleteProfile(profileId, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Rest_deleteProfile(this._handle, profileId,
            timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'deleteProfile(' + profileId + ')');
        return _decodeJson(body);
    }

    getVolatileInfo(timeoutMs = 10000) {
        return _typedGet(this, 'getVolatileInfo', timeoutMs,
            (h, t, o) => fns.Rest_getVolatileInfo(h, t, o));
    }

    getOcrConfig(timeoutMs = 10000) {
        return _typedGet(this, 'getOcrConfig', timeoutMs,
            (h, t, o) => fns.Rest_getOcrConfig(h, t, o));
    }
    setOcrConfig(cfg, timeoutMs = 10000) {
        return _typedPut(this, 'setOcrConfig', cfg, timeoutMs,
            (h, j, t, o) => fns.Rest_setOcrConfig(h, j, t, o));
    }

    getAnalyticsConfig(timeoutMs = 10000) {
        return _typedGet(this, 'getAnalyticsConfig', timeoutMs,
            (h, t, o) => fns.Rest_getAnalyticsConfig(h, t, o));
    }
    setAnalyticsConfig(cfg, timeoutMs = 10000) {
        return _typedPut(this, 'setAnalyticsConfig', cfg, timeoutMs,
            (h, j, t, o) => fns.Rest_setAnalyticsConfig(h, j, t, o));
    }

    getClassifierConfig(timeoutMs = 10000) {
        return _typedGet(this, 'getClassifierConfig', timeoutMs,
            (h, t, o) => fns.Rest_getClassifierConfig(h, t, o));
    }
    setClassifierConfig(cfg, timeoutMs = 10000) {
        return _typedPut(this, 'setClassifierConfig', cfg, timeoutMs,
            (h, j, t, o) => fns.Rest_setClassifierConfig(h, j, t, o));
    }

    getLanesConfig(timeoutMs = 10000) {
        return _typedGet(this, 'getLanesConfig', timeoutMs,
            (h, t, o) => fns.Rest_getLanesConfig(h, t, o));
    }
    setLanesConfig(cfg, timeoutMs = 10000) {
        return _typedPut(this, 'setLanesConfig', cfg, timeoutMs,
            (h, j, t, o) => fns.Rest_setLanesConfig(h, j, t, o));
    }

    getItscamproConfig(timeoutMs = 10000) {
        return _typedGet(this, 'getItscamproConfig', timeoutMs,
            (h, t, o) => fns.Rest_getItscamproConfig(h, t, o));
    }
    setItscamproConfig(cfg, timeoutMs = 10000) {
        return _typedPut(this, 'setItscamproConfig', cfg, timeoutMs,
            (h, j, t, o) => fns.Rest_setItscamproConfig(h, j, t, o));
    }
    getItscamproStatus(timeoutMs = 10000) {
        return _typedGet(this, 'getItscamproStatus', timeoutMs,
            (h, t, o) => fns.Rest_getItscamproStatus(h, t, o));
    }

    // ====================================================================
    //  internal helpers
    // ====================================================================

    _withBody(verb, path, body, timeoutMs, fn) {
        this._requireOpen();
        let bodyStr;
        if (body == null) bodyStr = '';
        else if (typeof body === 'string') bodyStr = body;
        else bodyStr = JSON.stringify(body);

        const out = [null];
        const rc = fn(this._handle, path, bodyStr, timeoutMs, out);
        const respBody = takeString(out[0]);
        checkCall(rc, verb + ' ' + path);
        return _decodeJson(respBody);
    }
}

function _typedGet(client, label, timeoutMs, fn) {
    client._requireOpen();
    const out = [null];
    const rc = fn(client._handle, timeoutMs, out);
    const body = takeString(out[0]);
    checkCall(rc, label);
    return _decodeJson(body);
}

function _typedPut(client, label, cfg, timeoutMs, fn) {
    client._requireOpen();
    const out = [null];
    const json = (cfg == null)
        ? ''
        : (typeof cfg === 'string' ? cfg : JSON.stringify(cfg));
    const rc = fn(client._handle, json, timeoutMs, out);
    const body = takeString(out[0]);
    checkCall(rc, label);
    return _decodeJson(body);
}

function _decodeJson(body) {
    if (!body) return null;
    try {
        return JSON.parse(body);
    } catch (_) {
        return body; // non-JSON body, return raw
    }
}

function _softwareUpdateOptions(options) {
    if (typeof options === 'string') options = { swuPath: options };
    if (!options || !options.swuPath) {
        throw new Error('software update options require swuPath');
    }
    return {
        swuPath: options.swuPath,
        uploadTimeoutMs: options.uploadTimeoutMs ?? 300000,
        statusTimeoutMs: options.statusTimeoutMs ?? 900000,
        restartTimeoutMs: options.restartTimeoutMs ?? 10000,
        requestRestart: !!options.requestRestart,
    };
}

function _registerStatusCallback(callback) {
    if (typeof callback !== 'function') return null;
    return koffi.register((statusJson, _ud) => callback(_decodeJson(statusJson)),
        koffi.pointer(callbacks.SoftwareUpdateStatusCb));
}

function _async(fn) {
    return new Promise((resolve, reject) => {
        try { resolve(fn()); }
        catch (e) { reject(e); }
    });
}

module.exports = { ItscamRestClient, SoftwareUpdateOperation };
