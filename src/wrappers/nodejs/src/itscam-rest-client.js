/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * REST/JSON client for the ITSCAM webapp on port 80/443.  Always
 * requires a `login()` call before any other method.
 */
'use strict';

const { fns } = require('./native');
const { ItscamError, ErrorCode } = require('./errors');
const { takeString, checkCall } = require('./utils');

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

    getAsync(path, timeoutMs)        { return _async(() => this.get(path, timeoutMs)); }
    putAsync(path, body, timeoutMs)  { return _async(() => this.put(path, body, timeoutMs)); }
    postAsync(path, body, timeoutMs) { return _async(() => this.post(path, body, timeoutMs)); }
    deleteAsync(path, timeoutMs)     { return _async(() => this.delete(path, timeoutMs)); }

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

function _async(fn) {
    return new Promise((resolve, reject) => {
        try { resolve(fn()); }
        catch (e) { reject(e); }
    });
}

module.exports = { ItscamRestClient };
