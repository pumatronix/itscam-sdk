/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Binary TCP client (Cougar protocol on port 60000).
 */
'use strict';

const { koffi, fns, structs, callbacks } = require('./native');
const { ItscamError, ErrorCode } = require('./errors');
const { readBytes, checkCall } = require('./utils');

const ConnectionState = Object.freeze({
    CONNECTED: 0,
    DISCONNECTED: 1,
    RECONNECTING: 2,
    RECONNECTED: 3,
});

const LogLevel = Object.freeze({
    INFO: 0,
    ERROR: 1,
});

class ItscamClient {
    constructor() {
        this._handle = fns.Client_create();
        if (!this._handle) {
            throw new ItscamError(ErrorCode.ALLOCATION_FAILED,
                'ITSCAM_Client_create returned NULL');
        }
        // Strong references for native callbacks so V8 doesn't reclaim
        // them while a worker thread is still calling in.
        this._callbackRefs = new Map();
        // The JS handler each trampoline calls
        this._handlers = {};
        this._closed = false;
        // Native calls in progress, see _begin(): synchronous ones too, since
        // a callback run on this thread during one may close the client
        this._calls = 0;
    }

    [Symbol.dispose]() { this.close(); }

    /**
     * Releases the client.  Native calls still in progress (e.g. a
     * connectAsync()) end first: the last of them releases the client.
     */
    close() {
        if (this._closed) return;
        // Calls still arriving from SDK threads are dropped from now on
        this._closed = true;
        if (this._calls === 0) this._destroy();
    }

    _destroy() {
        const handle = this._handle;
        this._handle = null;
        // Client_destroy() returns once no callback is running or can
        // still start. Koffi runs the callbacks of SDK threads on this
        // thread, so destroy on a worker thread and keep the callbacks
        // registered until it is done: unregistered earlier, a callback
        // registered meanwhile (e.g. by the next client) could take a
        // slot and receive a late call with the wrong arguments.
        const refs = Array.from(this._callbackRefs.values());
        this._callbackRefs.clear();
        fns.Client_destroy.async(handle, () => {
            for (const cb of refs) {
                try { koffi.unregister(cb); } catch (_) { /* ignore */ }
            }
        });
    }

    // Starts a native call and returns the handle; throws once closed.
    // Call _end() once the native call has returned.
    _begin() {
        if (this._closed) {
            throw new Error('ItscamClient has been closed');
        }
        this._calls++;
        return this._handle;
    }

    // Like _begin(), but returns null once closed
    _tryBegin() {
        return this._closed ? null : this._begin();
    }

    // The last call to end after close() destroys the client
    _end() {
        if (--this._calls === 0 && this._closed) this._destroy();
    }

    // ====================================================================
    //  Connection
    // ====================================================================

    /**
     * @param {string} address
     * @param {number} [port=60000]
     * @param {number} [timeoutMs=10000]
     * @param {?{enabled?:boolean, intervalMs?:number, maxRetries?:number}} [reconnect]
     */
    connect(address, port = 60000, timeoutMs = 10000, reconnect = null) {
        const h = this._begin();
        try {
            let cfgPtr = null;
            if (reconnect) {
                cfgPtr = {
                    enabled: reconnect.enabled ? 1 : 0,
                    intervalMs: reconnect.intervalMs || 3000,
                    maxRetries: reconnect.maxRetries || 0,
                };
            }
            const rc = fns.Client_connect(h, address, port,
                timeoutMs, cfgPtr);
            checkCall(rc, 'connect(' + address + ':' + port + ')');
        } finally {
            this._end();
        }
    }

    connectAsync(address, port = 60000, timeoutMs = 10000, reconnect = null) {
        let cfgPtr = null;
        if (reconnect) {
            cfgPtr = {
                enabled: reconnect.enabled ? 1 : 0,
                intervalMs: reconnect.intervalMs || 3000,
                maxRetries: reconnect.maxRetries || 0,
            };
        }
        const h = this._begin();
        return new Promise((resolve, reject) => {
            try {
                fns.Client_connect.async(h, address, port, timeoutMs,
                    cfgPtr, (err, rc) => {
                        this._end();
                        if (err) {
                            reject(err);
                            return;
                        }
                        try {
                            checkCall(rc, 'connect(' + address + ':' + port + ')');
                            resolve();
                        } catch (e) { reject(e); }
                    });
            } catch (e) {
                this._end();
                reject(e);
            }
        });
    }

    disconnect() {
        const h = this._tryBegin();
        if (!h) return;
        try {
            fns.Client_disconnect(h);
        } finally {
            this._end();
        }
    }

    isConnected() {
        const h = this._tryBegin();
        if (!h) return false;
        try {
            return Boolean(fns.Client_isConnected(h));
        } finally {
            this._end();
        }
    }

    // ====================================================================
    //  Authentication
    // ====================================================================

    authenticate(password, timeoutMs = 10000) {
        const h = this._begin();
        try {
            const rc = fns.Client_authenticate(h, password, timeoutMs);
            checkCall(rc, 'authenticate');
        } finally {
            this._end();
        }
    }

    authenticateAsync(password, timeoutMs) {
        return new Promise((resolve, reject) => {
            try {
                this.authenticate(password, timeoutMs);
                resolve();
            } catch (e) { reject(e); }
        });
    }

    // ====================================================================
    //  Subscription
    // ====================================================================

    subscribeCaptures(config, timeoutMs = 10000) {
        const h = this._begin();
        try {
            const c = Object.assign({
                includeTrigger: 1,
                includeSnapshot: 1,
                includeMetadata: 1,
                embedComments: 1,
                embedExif: 1,
                embedSignature: 0,
                triggerQuality: -1,
                snapshotQuality: -1,
            }, _normaliseCaptureConfig(config));
            const rc = fns.Client_subscribeCaptures(h, c, timeoutMs);
            checkCall(rc, 'subscribeCaptures');
        } finally {
            this._end();
        }
    }

    subscribeCapturesAsync(config, timeoutMs = 10000) {
        const c = Object.assign({
            includeTrigger: 1,
            includeSnapshot: 1,
            includeMetadata: 1,
            embedComments: 1,
            embedExif: 1,
            embedSignature: 0,
            triggerQuality: -1,
            snapshotQuality: -1,
        }, _normaliseCaptureConfig(config));
        const h = this._begin();
        return new Promise((resolve, reject) => {
            try {
                fns.Client_subscribeCaptures.async(h, c, timeoutMs,
                    (err, rc) => {
                        this._end();
                        if (err) {
                            reject(err);
                            return;
                        }
                        try {
                            checkCall(rc, 'subscribeCaptures');
                            resolve();
                        } catch (e) { reject(e); }
                    });
            } catch (e) {
                this._end();
                reject(e);
            }
        });
    }

    subscribe(events, timeoutMs = 10000) {
        const h = this._begin();
        try {
            const e = Object.assign({
                pipeline: 0, triggerMetadata: 0, triggerImage: 0,
                snapshotMetadata: 0, snapshotImage: 0,
                previewMetadata: 0, previewImage: 0,
                gpio: 0, serial1: 0, serial2: 0,
            }, _normaliseEventSubscription(events));
            const rc = fns.Client_subscribe(h, e, timeoutMs);
            checkCall(rc, 'subscribe');
        } finally {
            this._end();
        }
    }

    // ====================================================================
    //  Capture
    // ====================================================================

    captureSnapshot(timeoutMs = 15000) {
        const h = this._begin();
        try {
            const arrPtrOut = [null];
            const rc = fns.Client_captureSnapshot(h,
                { reserved: 0 }, timeoutMs, arrPtrOut);
            checkCall(rc, 'captureSnapshot');
            const arr = arrPtrOut[0];
            try {
                return _readResultArray(arr);
            } finally {
                if (arr) fns.CaptureResultArray_destroy(arr);
            }
        } finally {
            this._end();
        }
    }

    captureSnapshotAsync(timeoutMs) {
        return new Promise((resolve, reject) => {
            try { resolve(this.captureSnapshot(timeoutMs)); }
            catch (e) { reject(e); }
        });
    }

    getLastFrame(quality = 80, timeoutMs = 10000) {
        const h = this._begin();
        try {
            const out = [null];
            const rc = fns.Client_getLastFrame(h, quality,
                timeoutMs, out);
            checkCall(rc, 'getLastFrame');
            const arr = out[0];
            if (!arr) return Buffer.alloc(0);
            try {
                const size = Number(fns.ByteArray_size(arr));
                const data = fns.ByteArray_data(arr);
                return readBytes(data, size);
            } finally {
                fns.ByteArray_destroy(arr);
            }
        } finally {
            this._end();
        }
    }

    // ====================================================================
    //  Profiles
    // ====================================================================

    getActiveProfileId(timeoutMs = 10000) {
        const h = this._begin();
        try {
            const out = [0];
            const rc = fns.Client_getActiveProfileId(h, timeoutMs, out);
            checkCall(rc, 'getActiveProfileId');
            return out[0];
        } finally {
            this._end();
        }
    }

    setActiveProfile(profileId, timeoutMs = 10000) {
        const h = this._begin();
        try {
            const rc = fns.Client_setActiveProfile(h, profileId, timeoutMs);
            checkCall(rc, 'setActiveProfile');
        } finally {
            this._end();
        }
    }

    listProfiles(timeoutMs = 10000) {
        const h = this._begin();
        try {
            const out = [null];
            const rc = fns.Client_listProfiles(h, timeoutMs, out);
            checkCall(rc, 'listProfiles');
            const arr = out[0];
            try {
                const size = Number(fns.ProfileArray_size(arr));
                const profiles = [];
                for (let i = 0; i < size; i++) {
                    const info = {
                        id: 0, name: '', description: '', isActive: 0,
                    };
                    if (fns.ProfileArray_get(arr, i, info)) {
                        profiles.push({
                            id: info.id,
                            name: info.name || '',
                            description: info.description || '',
                            active: Boolean(info.isActive),
                        });
                    }
                }
                return profiles;
            } finally {
                if (arr) fns.ProfileArray_destroy(arr);
            }
        } finally {
            this._end();
        }
    }

    reboot(timeoutMs = 10000) {
        const h = this._begin();
        try {
            const rc = fns.Client_reboot(h, timeoutMs);
            checkCall(rc, 'reboot');
        } finally {
            this._end();
        }
    }

    // ====================================================================
    //  Callbacks (delivered on the JS thread, in order)
    // ====================================================================
    //
    // Each callback type gets one native trampoline, installed the first time
    // a handler is set and kept until the client is destroyed; the setters
    // below only swap the JS handler it calls. A native setter that replaces
    // a callback waits for the one in progress, which may be waiting for this
    // thread: installing a first one waits for nothing.

    onTriggerImage(callback) {
        this._setCaptureCallback('trigger', callback,
            (h, cb, ud) => fns.Client_onTriggerImage(h, cb, ud));
    }

    onSnapshotImage(callback) {
        this._setCaptureCallback('snapshot', callback,
            (h, cb, ud) => fns.Client_onSnapshotImage(h, cb, ud));
    }

    onDisconnect(callback) {
        this._setCallback('disconnect', callback, () => koffi.register((reason, _ud) => {
            const fn = this._handlers.disconnect;
            if (this._closed || !fn) return;
            try { fn(reason || ''); } catch (_) { /* swallow */ }
        }, koffi.pointer(callbacks.DisconnectCb)),
        (h, cb) => fns.Client_onDisconnect(h, cb, null));
    }

    onConnectionState(callback) {
        this._setCallback('connectionState', callback, () => koffi.register((state, reason, _ud) => {
            const fn = this._handlers.connectionState;
            if (this._closed || !fn) return;
            try { fn(state, reason || ''); } catch (_) {}
        }, koffi.pointer(callbacks.ConnStateCb)),
        (h, cb) => fns.Client_onConnectionState(h, cb, null));
    }

    onLog(callback) {
        this._setCallback('log', callback, () => koffi.register((level, message, _ud) => {
            const fn = this._handlers.log;
            if (this._closed || !fn) return;
            try { fn(level, message || ''); } catch (_) {}
        }, koffi.pointer(callbacks.LogCb)),
        (h, cb) => fns.Client_onLog(h, cb, null));
    }

    // Sets the JS handler of `key`, installing its trampoline the first time
    _setCallback(key, callback, register, install) {
        const h = this._tryBegin();
        if (!h) return;
        try {
            this._handlers[key] = callback || null;
            if (callback && !this._callbackRefs.has(key)) {
                const cb = register();
                this._callbackRefs.set(key, cb);
                install(h, cb);
            }
        } finally {
            this._end();
        }
    }

    _setCaptureCallback(key, userCallback, setter) {
        this._setCallback(key, userCallback, () => koffi.register((resultPtr, _ud) => {
            const fn = this._handlers[key];
            if (this._closed || !fn || !resultPtr) return;
            try {
                fn(_readSingleResult(resultPtr));
            } catch (_) { /* swallow */ }
        }, koffi.pointer(callbacks.CaptureCb)),
        (h, cb) => setter(h, cb, null));
    }
}

// --------------------------------------------------------------------------
//  Internal helpers
// --------------------------------------------------------------------------

function _normaliseCaptureConfig(c) {
    if (!c) return {};
    return {
        includeTrigger: c.includeTrigger ? 1 : 0,
        includeSnapshot: c.includeSnapshot ? 1 : 0,
        includeMetadata: c.includeMetadata ? 1 : 0,
        embedComments: c.embedComments ? 1 : 0,
        embedExif: c.embedExif ? 1 : 0,
        embedSignature: c.embedSignature ? 1 : 0,
        triggerQuality: c.triggerQuality == null ? -1 : c.triggerQuality,
        snapshotQuality: c.snapshotQuality == null ? -1 : c.snapshotQuality,
    };
}

function _normaliseEventSubscription(e) {
    if (!e) return {};
    return {
        pipeline: e.pipeline ? 1 : 0,
        triggerMetadata: e.triggerMetadata ? 1 : 0,
        triggerImage: e.triggerImage ? 1 : 0,
        snapshotMetadata: e.snapshotMetadata ? 1 : 0,
        snapshotImage: e.snapshotImage ? 1 : 0,
        previewMetadata: e.previewMetadata ? 1 : 0,
        previewImage: e.previewImage ? 1 : 0,
        gpio: e.gpio ? 1 : 0,
        serial1: e.serial1 ? 1 : 0,
        serial2: e.serial2 ? 1 : 0,
    };
}

function _readResultArray(arr) {
    if (!arr) return [];
    const size = Number(fns.CaptureResultArray_size(arr));
    const results = [];
    for (let i = 0; i < size; i++) {
        const p = fns.CaptureResultArray_get(arr, i);
        if (p) results.push(_readSingleResult(p));
    }
    return results;
}

function _readSingleResult(p) {
    const info = fns.CaptureResult_getInfo(p);

    const plateCount = Number(fns.CaptureResult_getPlateCount(p));
    const plates = [];
    for (let i = 0; i < plateCount; i++) {
        const s = fns.CaptureResult_getPlate(p, i);
        if (s) plates.push(s);
    }

    const sizeOut = [0];
    const dataPtr = fns.CaptureResult_getJpeg(p, sizeOut);
    const jpeg = readBytes(dataPtr, Number(sizeOut[0]));

    return {
        info: {
            requestId: Number(info.requestId),
            frameCount: Number(info.frameCount),
            multiExpIndex: info.multiExpIndex,
            multiExpLength: info.multiExpLength,
            shutter: info.shutter,
            gain: info.gain,
            width: info.width,
            height: info.height,
            timestamp: Object.assign({}, info.timestamp),
            plates,
        },
        plates,
        jpeg,
    };
}

module.exports = {
    ItscamClient,
    ConnectionState,
    LogLevel,
};
