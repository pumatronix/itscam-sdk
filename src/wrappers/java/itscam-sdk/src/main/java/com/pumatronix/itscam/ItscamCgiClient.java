/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.internal.ItscamLibrary;
import com.pumatronix.itscam.internal.NativeLibrary;
import com.sun.jna.Memory;
import com.sun.jna.Pointer;
import com.sun.jna.StringArray;
import com.sun.jna.ptr.PointerByReference;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Consumer;

/**
 * Legacy CGI client: {@code lastframe.cgi}, {@code snapshot.cgi},
 * {@code mjpegvideo.cgi}, {@code trigger.cgi?force=1}, {@code reboot.cgi}.
 *
 * <h2>Authentication</h2>
 *
 * <p>CGI authentication is <strong>opt-in</strong>.  By default the
 * camera's {@code configCgi.blockAPI} flag is {@code false}, which means
 * the CGI endpoints accept anonymous requests.  Only call
 * {@link #login(String, String, int)} when the operator has explicitly
 * enabled CGI auth.
 *
 * <h2>MJPEG streaming</h2>
 *
 * <p>Frame callbacks installed via
 * {@link #startMjpegStream(Consumer, int)} are delivered on the SDK's
 * worker thread.  <strong>Never block</strong> in the callback &mdash;
 * push frames to a queue and return immediately.
 */
public final class ItscamCgiClient implements AutoCloseable {

    private final ItscamLibrary lib;
    private Pointer handle;
    private ItscamLibrary.CgiStreamCallback streamCallback;

    public ItscamCgiClient() {
        this.lib = NativeLibrary.get();
        this.handle = lib.ITSCAM_CgiClient_create();
        if (handle == null || handle.equals(Pointer.NULL)) {
            throw new ItscamException(ErrorCode.ALLOCATION_FAILED,
                    "ITSCAM_CgiClient_create returned NULL");
        }
    }

    @Override
    public void close() {
        if (handle != null) {
            stopMjpegStream();
            lib.ITSCAM_CgiClient_destroy(handle);
            handle = null;
        }
    }

    private void requireOpen() {
        if (handle == null) {
            throw new IllegalStateException("ItscamCgiClient closed");
        }
    }

    // ====================================================================
    //  Configuration
    // ====================================================================

    public void setBaseUrl(String host, int port, String scheme) {
        requireOpen();
        int rc = lib.ITSCAM_CgiClient_setBaseUrl(handle, host,
                (short) port, scheme == null ? "http" : scheme);
        ItscamException.throwIfFailed(rc, "setBaseUrl(" + host + ":" + port + ")");
    }

    public void setApiPrefix(String prefix) {
        requireOpen();
        lib.ITSCAM_CgiClient_setApiPrefix(handle, prefix);
    }

    public void setCaCertFile(String pemPath) {
        requireOpen();
        lib.ITSCAM_CgiClient_setCaCertFile(handle, pemPath);
    }

    public void setCaCertData(String pem) {
        requireOpen();
        lib.ITSCAM_CgiClient_setCaCertData(handle, pem);
    }

    public void setVerifyServerCertificate(boolean verify) {
        requireOpen();
        lib.ITSCAM_CgiClient_setVerifyServerCertificate(handle, verify ? 1 : 0);
    }

    public void setClientCertificate(String certPem, String keyPem) {
        requireOpen();
        lib.ITSCAM_CgiClient_setClientCertificate(handle, certPem, keyPem);
    }

    // ====================================================================
    //  Authentication (opt-in)
    // ====================================================================

    public void login(String username, String password, int timeoutMs) {
        requireOpen();
        int rc = lib.ITSCAM_CgiClient_login(handle, username, password, timeoutMs);
        ItscamException.throwIfFailed(rc, "login");
    }

    public CompletableFuture<Void> loginAsync(String username, String password,
                                              int timeoutMs) {
        return CompletableFuture.runAsync(() -> login(username, password, timeoutMs));
    }

    public void setAuthToken(String token) {
        requireOpen();
        lib.ITSCAM_CgiClient_setAuthToken(handle, token);
    }

    public void clearAuthToken() {
        requireOpen();
        lib.ITSCAM_CgiClient_clearAuthToken(handle);
    }

    public void setBasicAuth(String user, String password) {
        requireOpen();
        lib.ITSCAM_CgiClient_setBasicAuth(handle, user, password);
    }

    public void clearBasicAuth() {
        requireOpen();
        lib.ITSCAM_CgiClient_clearBasicAuth(handle);
    }

    // ====================================================================
    //  /api/lastframe.cgi
    // ====================================================================

    public CgiImage getLastFrame(int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_CgiClient_getLastFrame(handle, timeoutMs, out);
        ItscamException.throwIfFailed(rc, "getLastFrame");
        return consumeImage(out.getValue());
    }

    public CompletableFuture<CgiImage> getLastFrameAsync(int timeoutMs) {
        return CompletableFuture.supplyAsync(() -> getLastFrame(timeoutMs));
    }

    // ====================================================================
    //  /api/snapshot.cgi
    // ====================================================================

    public List<CgiImage> getSnapshot(int timeoutMs) {
        return getSnapshot(new SnapshotCgiRequest(), timeoutMs);
    }

    public List<CgiImage> getSnapshot(SnapshotCgiRequest request, int timeoutMs) {
        requireOpen();
        ItscamLibrary.ITSCAMCgiSnapshotRequest n =
                new ItscamLibrary.ITSCAMCgiSnapshotRequest();

        Memory shuttersBuf = null;
        if (request.shutters().length > 0) {
            shuttersBuf = new Memory(request.shutters().length * 4L);
            shuttersBuf.write(0, request.shutters(), 0,
                    request.shutters().length);
            n.shutters = shuttersBuf;
            n.shuttersLen = request.shutters().length;
        }
        Memory gainsBuf = null;
        if (request.gains().length > 0) {
            gainsBuf = new Memory(request.gains().length * 4L);
            gainsBuf.write(0, request.gains(), 0, request.gains().length);
            n.gains = gainsBuf;
            n.gainsLen = request.gains().length;
        }
        n.quality = request.quality();
        n.mosaic = request.mosaic() ? 1 : 0;
        n.format = request.format().isEmpty() ? null : request.format();
        n.scenario = request.scenario();
        n.crop = request.crop().isEmpty() ? null : request.crop();
        n.textOverlay = request.textOverlay().isEmpty() ? null
                : request.textOverlay();

        StringArray keysArr = null;
        StringArray valsArr = null;
        if (!request.userMetadata().isEmpty()) {
            String[] keys = new String[request.userMetadata().size() + 1];
            String[] vals = new String[request.userMetadata().size() + 1];
            int i = 0;
            for (Map.Entry<String, String> e : request.userMetadata().entrySet()) {
                keys[i] = e.getKey();
                vals[i] = e.getValue();
                i++;
            }
            keys[i] = null;
            vals[i] = null;
            keysArr = new StringArray(keys, "UTF-8");
            valsArr = new StringArray(vals, "UTF-8");
            n.userMetadataKeys = keysArr;
            n.userMetadataValues = valsArr;
        }

        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_CgiClient_getSnapshot(handle, n, timeoutMs, out);
        // Keep references alive until the call returns to ensure JNA's
        // GC doesn't reclaim the buffers mid-call.
        if (shuttersBuf != null) shuttersBuf.clear(0);
        if (gainsBuf != null) gainsBuf.clear(0);
        if (keysArr != null) keysArr.getPointer(0);
        if (valsArr != null) valsArr.getPointer(0);
        ItscamException.throwIfFailed(rc, "getSnapshot");

        Pointer arr = out.getValue();
        try {
            List<CgiImage> images = new ArrayList<>();
            long size = lib.ITSCAM_CgiImageArray_size(arr);
            for (long i = 0; i < size; i++) {
                Pointer p = lib.ITSCAM_CgiImageArray_get(arr, i);
                if (p != null) images.add(borrowImage(p));
            }
            return images;
        } finally {
            if (arr != null) lib.ITSCAM_CgiImageArray_destroy(arr);
        }
    }

    public CompletableFuture<List<CgiImage>> getSnapshotAsync(
            SnapshotCgiRequest request, int timeoutMs) {
        return CompletableFuture.supplyAsync(
                () -> getSnapshot(request, timeoutMs));
    }

    // ====================================================================
    //  /api/mjpegvideo.cgi
    // ====================================================================

    public synchronized void startMjpegStream(Consumer<CgiStreamFrame> onFrame,
                                              int timeoutMs) {
        requireOpen();
        if (streamCallback != null) {
            throw new IllegalStateException("MJPEG stream already running");
        }
        ItscamLibrary.CgiStreamCallback cb = (frame, ud) -> {
            if (frame == null) return;
            String mime = frame.mimeType == null ? "" : frame.mimeType;
            byte[] body = (frame.data == null || frame.dataLen <= 0)
                    ? new byte[0]
                    : frame.data.getByteArray(0, (int) frame.dataLen);
            try {
                onFrame.accept(new CgiStreamFrame(frame.sequence, mime, body));
            } catch (Throwable ignored) {
                /* never let exceptions cross into native code */
            }
        };
        streamCallback = cb;
        int rc = lib.ITSCAM_CgiClient_startMjpegStream(handle, cb, null,
                timeoutMs);
        if (rc != 0) {
            streamCallback = null;
            ItscamException.throwIfFailed(rc, "startMjpegStream");
        }
    }

    public synchronized void stopMjpegStream() {
        if (handle == null) return;
        lib.ITSCAM_CgiClient_stopMjpegStream(handle);
        streamCallback = null;
    }

    public boolean isMjpegStreamRunning() {
        return handle != null
                && lib.ITSCAM_CgiClient_isMjpegStreamRunning(handle) != 0;
    }

    // ====================================================================
    //  /api/trigger.cgi (force=1) / /api/reboot.cgi
    // ====================================================================

    public String forceTrigger(int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_CgiClient_forceTrigger(handle, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "forceTrigger");
        return body;
    }

    public String reboot(int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_CgiClient_reboot(handle, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "reboot");
        return body;
    }

    // ====================================================================
    //  Helpers
    // ====================================================================

    private CgiImage consumeImage(Pointer p) {
        if (p == null) return new CgiImage("", new byte[0]);
        try {
            return readImage(p);
        } finally {
            lib.ITSCAM_CgiImage_destroy(p);
        }
    }

    private CgiImage borrowImage(Pointer p) {
        return readImage(p);
    }

    private CgiImage readImage(Pointer p) {
        String mime = lib.ITSCAM_CgiImage_mimeType(p);
        long size = lib.ITSCAM_CgiImage_size(p);
        Pointer data = lib.ITSCAM_CgiImage_data(p);
        byte[] bytes = (data == null || size <= 0)
                ? new byte[0] : data.getByteArray(0, (int) size);
        return new CgiImage(mime == null ? "" : mime, bytes);
    }

    private String takeString(Pointer p) {
        if (p == null) return "";
        try {
            String s = lib.ITSCAM_String_data(p);
            return s == null ? "" : s;
        } finally {
            lib.ITSCAM_String_destroy(p);
        }
    }
}
