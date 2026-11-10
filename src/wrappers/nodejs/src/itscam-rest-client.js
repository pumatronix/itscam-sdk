/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * REST/JSON client for the ITSCAM webapp on port 80/443.  Always
 * requires a `login()` call before any other method.
 */
'use strict';

const { koffi, fns, structs, callbacks } = require('./native');
const { ItscamError, ErrorCode } = require('./errors');
const { takeString, checkCall } = require('./utils');

class ItscamRestClient {
    constructor() {
        this._handle = fns.Rest_create();
        if (!this._handle) {
            throw new ItscamError(ErrorCode.ALLOCATION_FAILED,
                'ITSCAM_RestClient_create returned NULL');
        }
        this._telemetry = null;            // { cb, stopped } of the running stream
        this._telemetryStopping = null;    // Promise while a stop is in progress
    }

    [Symbol.dispose]() { this.close(); }

    close() {
        if (this._handle) {
            const handle = this._handle;
            this.stopTelemetryStream();
            this._handle = null;
            // Destroying the client also joins the stream thread: wait for
            // the stop to finish it first (see stopTelemetryStream()).
            if (this._telemetryStopping) {
                this._telemetryStopping.then(() => fns.Rest_destroy(handle));
            } else {
                fns.Rest_destroy(handle);
            }
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
    //  System telemetry
    //    GET       /api/system/telemetry            (read it once)
    //    WebSocket /api/statusws?format=telemetry   (pushed every 5 s)
    // ====================================================================

    /** GET /api/system/telemetry: the state of the whole camera. */
    getSystemTelemetry(timeoutMs = 10000) {
        return _typedGet(this, 'getSystemTelemetry', timeoutMs,
            (h, t, o) => fns.Rest_getSystemTelemetry(h, t, o));
    }

    getSystemTelemetryAsync(timeoutMs) {
        return _async(() => this.getSystemTelemetry(timeoutMs));
    }

    /**
     * Receive the telemetry the camera pushes: one document at once, then
     * one every 5 s.  The stream reconnects by itself with the client's
     * current token.  `onUpdate({ sequence, receivedAtMs, document })` runs
     * on the JS thread once the event loop is free; while it is busy, older
     * documents are dropped.  Without `onUpdate` the stream only keeps the
     * latest document (latestTelemetry()).
     */
    startTelemetryStream(onUpdate, timeoutMs = 15000) {
        this._requireOpen();
        if (this._telemetry) {
            throw new Error('telemetry stream already running');
        }
        if (this._telemetryStopping) {
            throw new Error('telemetry stream still stopping: await stopTelemetryStream() first');
        }
        const state = { cb: null, stopped: false };
        if (onUpdate) {
            state.cb = koffi.register((updatePtr, _ud) => {
                if (state.stopped || !updatePtr) return;
                try {
                    const u = koffi.decode(updatePtr, structs.ITSCAM_TelemetryUpdate);
                    onUpdate({
                        sequence: Number(u.sequence),
                        receivedAtMs: Number(u.receivedAtMs),
                        document: JSON.parse(u.json),
                    });
                } catch (_) { /* never let exceptions cross FFI */ }
            }, koffi.pointer(callbacks.TelemetryCb));
        }
        this._telemetry = state;
        const rc = fns.Rest_startTelemetryStream(this._handle, state.cb, null,
            timeoutMs);
        if (rc !== 0) {
            if (state.cb) {
                try { koffi.unregister(state.cb); } catch (_) {}
            }
            this._telemetry = null;
            checkCall(rc, 'startTelemetryStream');
        }
    }

    /**
     * Stop the telemetry stream.  Documents still arriving are dropped.
     * Returns a Promise that resolves once the stream thread has finished.
     */
    stopTelemetryStream() {
        if (this._telemetryStopping) return this._telemetryStopping;
        if (!this._handle || !this._telemetry) return Promise.resolve();
        const state = this._telemetry;
        state.stopped = true;
        this._telemetry = null;
        // The native stop waits for the callback in progress, which runs on
        // this (the JS) thread: stop on a worker thread, so this one stays
        // free to deliver (and drop) that document.
        this._telemetryStopping = new Promise((resolve) => {
            fns.Rest_stopTelemetryStream.async(this._handle, () => {
                if (state.cb) {
                    try { koffi.unregister(state.cb); } catch (_) {}
                }
                this._telemetryStopping = null;
                resolve();
            });
        });
        return this._telemetryStopping;
    }

    isTelemetryStreamRunning() {
        return Boolean(this._handle && this._telemetry && !this._telemetryStopping
            && fns.Rest_isTelemetryStreamRunning(this._handle));
    }

    isTelemetryStreamConnected() {
        return Boolean(this._handle && this._telemetry
            && fns.Rest_isTelemetryStreamConnected(this._handle));
    }

    /**
     * The last document the stream received, also after it stopped (check
     * receivedAtMs to tell how old it is); null before the first one.
     */
    latestTelemetry() {
        this._requireOpen();
        const sequence = [0];
        const receivedAtMs = [0];
        const out = [null];
        const rc = fns.Rest_latestTelemetry(this._handle, sequence,
            receivedAtMs, out);
        const body = takeString(out[0]);
        if (rc === ErrorCode.DISCONNECTED) return null;
        checkCall(rc, 'latestTelemetry');
        return {
            sequence: Number(sequence[0]),
            receivedAtMs: Number(receivedAtMs[0]),
            document: _decodeJson(body),
        };
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
