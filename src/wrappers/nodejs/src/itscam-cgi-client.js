/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * CGI client (snapshot.cgi, lastframe.cgi, mjpegvideo.cgi, reboot.cgi).
 * Authentication is opt-in -- only call login() when the camera has
 * configCgi.blockAPI = true.
 */
'use strict';

const { koffi, fns, callbacks } = require('./native');
const { ItscamError, ErrorCode } = require('./errors');
const { takeString, readBytes, checkCall } = require('./utils');

class ItscamCgiClient {
    constructor() {
        this._handle = fns.Cgi_create();
        if (!this._handle) {
            throw new ItscamError(ErrorCode.ALLOCATION_FAILED,
                'ITSCAM_CgiClient_create returned NULL');
        }
        this._streamCb = null;
    }

    [Symbol.dispose]() { this.close(); }

    close() {
        if (this._handle) {
            this.stopMjpegStream();
            fns.Cgi_destroy(this._handle);
            this._handle = null;
        }
    }

    _requireOpen() {
        if (!this._handle) throw new Error('ItscamCgiClient closed');
    }

    // ====================================================================
    //  Configuration / TLS
    // ====================================================================

    setBaseUrl(host, port = 80, scheme = 'http') {
        this._requireOpen();
        const rc = fns.Cgi_setBaseUrl(this._handle, host, port, scheme);
        checkCall(rc, 'setBaseUrl(' + host + ':' + port + ')');
    }

    setApiPrefix(prefix) {
        this._requireOpen();
        fns.Cgi_setApiPrefix(this._handle, prefix || '');
    }

    setCaCertFile(pemPath) {
        this._requireOpen();
        fns.Cgi_setCaCertFile(this._handle, pemPath || '');
    }

    setCaCertData(pem) {
        this._requireOpen();
        fns.Cgi_setCaCertData(this._handle, pem || '');
    }

    setVerifyServerCertificate(verify) {
        this._requireOpen();
        fns.Cgi_setVerifyServerCertificate(this._handle, verify ? 1 : 0);
    }

    setClientCertificate(certPem, keyPem) {
        this._requireOpen();
        fns.Cgi_setClientCertificate(this._handle,
            certPem || '', keyPem || '');
    }

    // ====================================================================
    //  Authentication (opt-in)
    // ====================================================================

    login(username, password, timeoutMs = 10000) {
        this._requireOpen();
        const rc = fns.Cgi_login(this._handle, username, password, timeoutMs);
        checkCall(rc, 'login');
    }

    loginAsync(username, password, timeoutMs) {
        return new Promise((resolve, reject) => {
            try { this.login(username, password, timeoutMs); resolve(); }
            catch (e) { reject(e); }
        });
    }

    setAuthToken(token) {
        this._requireOpen();
        fns.Cgi_setAuthToken(this._handle, token || '');
    }

    clearAuthToken() {
        this._requireOpen();
        fns.Cgi_clearAuthToken(this._handle);
    }

    setBasicAuth(user, password) {
        this._requireOpen();
        fns.Cgi_setBasicAuth(this._handle, user, password);
    }

    clearBasicAuth() {
        this._requireOpen();
        fns.Cgi_clearBasicAuth(this._handle);
    }

    // ====================================================================
    //  /api/lastframe.cgi
    // ====================================================================

    getLastFrame(timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Cgi_getLastFrame(this._handle, timeoutMs, out);
        checkCall(rc, 'getLastFrame');
        return _consumeImage(out[0]);
    }

    getLastFrameAsync(timeoutMs) {
        return new Promise((resolve, reject) => {
            try { resolve(this.getLastFrame(timeoutMs)); }
            catch (e) { reject(e); }
        });
    }

    // ====================================================================
    //  /api/snapshot.cgi
    // ====================================================================

    /**
     * @param {object} [request]
     * @param {number[]} [request.shutters]
     * @param {number[]} [request.gains]
     * @param {number}   [request.quality=-1]   -1 = camera default
     * @param {boolean}  [request.mosaic=false]
     * @param {string}   [request.format=""]    "" or "png"
     * @param {number}   [request.scenario=-1]
     * @param {string}   [request.crop=""]      "x0,y0,x1,y1"
     * @param {string}   [request.textOverlay=""]
     * @param {object}   [request.userMetadata] string-string map
     * @param {number}   [timeoutMs=15000]
     */
    getSnapshot(request = {}, timeoutMs = 15000) {
        this._requireOpen();

        const shutters = Array.isArray(request.shutters) ? request.shutters : [];
        const gains = Array.isArray(request.gains) ? request.gains : [];
        const meta = request.userMetadata || {};
        const metaKeys = Object.keys(meta);
        const metaVals = metaKeys.map(k => meta[k]);

        const native = {
            shutters: shutters.length ? shutters : null,
            shuttersLen: shutters.length,
            gains: gains.length ? gains : null,
            gainsLen: gains.length,
            quality: request.quality == null ? -1 : request.quality,
            mosaic: request.mosaic ? 1 : 0,
            format: request.format || '',
            scenario: request.scenario == null ? -1 : request.scenario,
            crop: request.crop || '',
            textOverlay: request.textOverlay || '',
            userMetadataKeys: metaKeys.length
                ? [...metaKeys, null] : null,
            userMetadataValues: metaVals.length
                ? [...metaVals, null] : null,
        };

        const out = [null];
        const rc = fns.Cgi_getSnapshot(this._handle, native, timeoutMs, out);
        checkCall(rc, 'getSnapshot');
        const arr = out[0];
        try {
            const size = Number(fns.CgiImageArray_size(arr));
            const images = [];
            for (let i = 0; i < size; i++) {
                const p = fns.CgiImageArray_get(arr, i);
                if (p) images.push(_borrowImage(p));
            }
            return images;
        } finally {
            if (arr) fns.CgiImageArray_destroy(arr);
        }
    }

    getSnapshotAsync(request, timeoutMs) {
        return new Promise((resolve, reject) => {
            try { resolve(this.getSnapshot(request, timeoutMs)); }
            catch (e) { reject(e); }
        });
    }

    // ====================================================================
    //  /api/mjpegvideo.cgi
    // ====================================================================

    /**
     * Begin streaming MJPEG frames.  The callback is invoked from the
     * SDK worker thread for every received frame.  Push the frame to a
     * Node.js queue / event-emitter / stream and return immediately.
     */
    startMjpegStream(onFrame, timeoutMs = 10000) {
        this._requireOpen();
        if (this._streamCb) {
            throw new Error('MJPEG stream already running');
        }
        const cb = koffi.register((framePtr, _ud) => {
            if (!framePtr) return;
            try {
                const frame = koffi.decode(framePtr,
                    require('./native').structs.ITSCAM_CgiStreamFrame);
                const data = readBytes(frame.data, Number(frame.dataLen));
                onFrame({
                    sequence: Number(frame.sequence),
                    mimeType: frame.mimeType || '',
                    data,
                });
            } catch (_) { /* never let exceptions cross FFI */ }
        }, koffi.pointer(callbacks.StreamCb));

        this._streamCb = cb;
        const rc = fns.Cgi_startMjpegStream(this._handle, cb, null, timeoutMs);
        if (rc !== 0) {
            try { koffi.unregister(cb); } catch (_) {}
            this._streamCb = null;
            checkCall(rc, 'startMjpegStream');
        }
    }

    stopMjpegStream() {
        if (!this._handle) return;
        fns.Cgi_stopMjpegStream(this._handle);
        if (this._streamCb) {
            try { koffi.unregister(this._streamCb); } catch (_) {}
            this._streamCb = null;
        }
    }

    isMjpegStreamRunning() {
        return Boolean(this._handle
            && fns.Cgi_isMjpegStreamRunning(this._handle));
    }

    // ====================================================================
    //  /api/trigger.cgi (force) / /api/reboot.cgi
    // ====================================================================

    forceTrigger(timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Cgi_forceTrigger(this._handle, timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'forceTrigger');
        return body;
    }

    reboot(timeoutMs = 10000) {
        this._requireOpen();
        const out = [null];
        const rc = fns.Cgi_reboot(this._handle, timeoutMs, out);
        const body = takeString(out[0]);
        checkCall(rc, 'reboot');
        return body;
    }
}

function _consumeImage(p) {
    if (!p) return { mimeType: '', data: Buffer.alloc(0) };
    try {
        return _readImage(p);
    } finally {
        fns.CgiImage_destroy(p);
    }
}

function _borrowImage(p) {
    return _readImage(p);
}

function _readImage(p) {
    const mime = fns.CgiImage_mimeType(p);
    const size = Number(fns.CgiImage_size(p));
    const data = fns.CgiImage_data(p);
    return {
        mimeType: mime || '',
        data: readBytes(data, size),
    };
}

module.exports = { ItscamCgiClient };
