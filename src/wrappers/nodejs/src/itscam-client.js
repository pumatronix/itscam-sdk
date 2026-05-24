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
    }

    [Symbol.dispose]() { this.close(); }

    close() {
        if (this._handle) {
            fns.Client_destroy(this._handle);
            this._handle = null;
            for (const cb of this._callbackRefs.values()) {
                try { koffi.unregister(cb); } catch (_) { /* ignore */ }
            }
            this._callbackRefs.clear();
        }
    }

    _requireOpen() {
        if (!this._handle) {
            throw new Error('ItscamClient has been closed');
        }
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
        this._requireOpen();
        let cfgPtr = null;
        if (reconnect) {
            cfgPtr = {
                enabled: reconnect.enabled ? 1 : 0,
                intervalMs: reconnect.intervalMs || 3000,
                maxRetries: reconnect.maxRetries || 0,
            };
        }
        const rc = fns.Client_connect(this._handle, address, port,
            timeoutMs, cfgPtr);
        checkCall(rc, 'connect(' + address + ':' + port + ')');
    }

    connectAsync(address, port, timeoutMs, reconnect) {
        return new Promise((resolve, reject) => {
            try {
                this.connect(address, port, timeoutMs, reconnect);
                resolve();
            } catch (e) { reject(e); }
        });
    }

    disconnect() {
        if (this._handle) fns.Client_disconnect(this._handle);
    }

    isConnected() {
        return Boolean(this._handle && fns.Client_isConnected(this._handle));
    }

    // ====================================================================
    //  Authentication
    // ====================================================================

    authenticate(password, timeoutMs = 10000) {
        this._requireOpen();
        const rc = fns.Client_authenticate(this._handle, password, timeoutMs);
        checkCall(rc, 'authenticate');
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
        this._requireOpen();
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
        const rc = fns.Client_subscribeCaptures(this._handle, c, timeoutMs);
        checkCall(rc, 'subscribeCaptures');
    }

    subscribe(events, timeoutMs = 10000) {
        this._requireOpen();
        const e = Object.assign({
            pipeline: 0, triggerMetadata: 0, triggerImage: 0,
            snapshotMetadata: 0, snapshotImage: 0,
            previewMetadata: 0, previewImage: 0,
            gpio: 0, serial1: 0, serial2: 0,
        }, _normaliseEventSubscription(events));
        const rc = fns.Client_subscribe(this._handle, e, timeoutMs);
        checkCall(rc, 'subscribe');
    }

    // ====================================================================
    //  Capture
    // ====================================================================

    captureSnapshot(timeoutMs = 15000) {
        this._requireOpen();
        const arrPtrOut = [null];
        const rc = fns.Client_captureSnapshot(this._handle,
            { reserved: 0 }, timeoutMs, arrPtrOut);
        checkCall(rc, 'captureSnapshot');
        const arr = arrPtrOut[0];
        try {
            return _readResultArray(arr);
        } finally {
            if (arr) fns.CaptureResultArray_destroy(arr);
        }
    }

    captureSnapshotAsync(timeoutMs) {
        return new Promise((resolve, reject) => {
            try { resolve(this.captureSnapshot(timeoutMs)); }
            catch (e) { reject(e); }
        });
    }

    getLastFrame(quality = 80, timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Client_getLastFrame(this._handle, quality,
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
    }

    // ====================================================================
    //  Profiles
    // ====================================================================

    getActiveProfileId(timeoutMs = 10000) {
        this._requireOpen();
        const out = [0];
        const rc = fns.Client_getActiveProfileId(this._handle, timeoutMs, out);
        checkCall(rc, 'getActiveProfileId');
        return out[0];
    }

    setActiveProfile(profileId, timeoutMs = 10000) {
        this._requireOpen();
        const rc = fns.Client_setActiveProfile(this._handle, profileId, timeoutMs);
        checkCall(rc, 'setActiveProfile');
    }

    listProfiles(timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Client_listProfiles(this._handle, timeoutMs, out);
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
    }

    reboot(timeoutMs = 10000) {
        this._requireOpen();
        const rc = fns.Client_reboot(this._handle, timeoutMs);
        checkCall(rc, 'reboot');
    }

    // ====================================================================
    //  Callbacks (delivered on the SDK worker thread -- do not block!)
    // ====================================================================

    onTriggerImage(callback) {
        this._setCaptureCallback('trigger', callback,
            (h, cb, ud) => fns.Client_onTriggerImage(h, cb, ud));
    }

    onSnapshotImage(callback) {
        this._setCaptureCallback('snapshot', callback,
            (h, cb, ud) => fns.Client_onSnapshotImage(h, cb, ud));
    }

    onDisconnect(callback) {
        if (!this._handle) return;
        if (callback === null || callback === undefined) {
            this._unregister('disconnect');
            fns.Client_onDisconnect(this._handle, null, null);
            return;
        }
        const cb = koffi.register((reason, _ud) => {
            try { callback(reason || ''); } catch (_) { /* swallow */ }
        }, koffi.pointer(callbacks.DisconnectCb));
        this._registerCallback('disconnect', cb);
        fns.Client_onDisconnect(this._handle, cb, null);
    }

    onConnectionState(callback) {
        if (!this._handle) return;
        if (!callback) {
            this._unregister('connectionState');
            fns.Client_onConnectionState(this._handle, null, null);
            return;
        }
        const cb = koffi.register((state, reason, _ud) => {
            try { callback(state, reason || ''); } catch (_) {}
        }, koffi.pointer(callbacks.ConnStateCb));
        this._registerCallback('connectionState', cb);
        fns.Client_onConnectionState(this._handle, cb, null);
    }

    onLog(callback) {
        if (!this._handle) return;
        if (!callback) {
            this._unregister('log');
            fns.Client_onLog(this._handle, null, null);
            return;
        }
        const cb = koffi.register((level, message, _ud) => {
            try { callback(level, message || ''); } catch (_) {}
        }, koffi.pointer(callbacks.LogCb));
        this._registerCallback('log', cb);
        fns.Client_onLog(this._handle, cb, null);
    }

    _registerCallback(key, cb) {
        this._unregister(key);
        this._callbackRefs.set(key, cb);
    }

    _unregister(key) {
        const prev = this._callbackRefs.get(key);
        if (prev) {
            try { koffi.unregister(prev); } catch (_) {}
            this._callbackRefs.delete(key);
        }
    }

    _setCaptureCallback(key, userCallback, setter) {
        if (!this._handle) return;
        if (!userCallback) {
            this._unregister(key);
            setter(this._handle, null, null);
            return;
        }
        const cb = koffi.register((resultPtr, _ud) => {
            try {
                if (resultPtr) userCallback(_readSingleResult(resultPtr));
            } catch (_) { /* swallow */ }
        }, koffi.pointer(callbacks.CaptureCb));
        this._registerCallback(key, cb);
        setter(this._handle, cb, null);
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
