/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.internal.ItscamLibrary;
import com.pumatronix.itscam.internal.Async;
import com.pumatronix.itscam.internal.NativeHandle;
import com.pumatronix.itscam.internal.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.PointerByReference;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Binary TCP client for the ITSCAM Cougar protocol on port 60000.  Use
 * this client for real-time triggers, GPIO/serial I/O, exposure-group
 * accumulation and very low-latency capture.  For equipment configuration
 * (profiles, OCR, classifier, etc.) use {@link ItscamRestClient} instead.
 *
 * <h2>Threading</h2>
 *
 * <p>Callbacks installed through {@code onTriggerImage}, {@code onSnapshotImage},
 * {@code onConnectionState}, {@code onDisconnect} and {@code onLog} are
 * delivered on the SDK's worker thread.  <strong>Never block</strong>
 * inside a callback &mdash; hand the payload to a queue, channel or
 * {@code ExecutorService} and return immediately.
 */
public final class ItscamClient implements AutoCloseable {

    private final ItscamLibrary lib;
    private final NativeHandle handle;

    /**
     * Strong references for native callbacks (prevent GC while in use).
     * Kept until the native client is destroyed.
     */
    private final Map<String, Object> callbacks = new ConcurrentHashMap<>();

    /** The user handler each native callback calls (see setHandler()). */
    private final Map<String, Object> handlers = new ConcurrentHashMap<>();

    public ItscamClient() {
        this.lib = NativeLibrary.get();
        Pointer h = lib.ITSCAM_Client_create();
        if (h == null || h.equals(Pointer.NULL)) {
            throw new ItscamException(ErrorCode.ALLOCATION_FAILED,
                    "ITSCAM_Client_create returned NULL");
        }
        this.handle = new NativeHandle(h, "ItscamClient has been closed",
                new NativeHandle.Destroyer() {
                    @Override
                    public void destroy(Pointer p) {
                        lib.ITSCAM_Client_destroy(p);
                        callbacks.clear();
                        handlers.clear();
                    }
                });
    }

    /**
     * Releases the client.  No callback starts once this is called.  Calls
     * still running on other threads end normally, and the native client is
     * released after the last one.
     */
    @Override
    public void close() {
        handle.close(null);
    }

    // ====================================================================
    //  Connection
    // ====================================================================

    public void connect(String address, int port, int timeoutMs) {
        connect(address, port, timeoutMs, null);
    }

    public void connect(String address, int port, int timeoutMs,
                        AutoReconnectConfig reconnect) {
        Pointer h = handle.acquire();
        try {
            ItscamLibrary.ITSCAMAutoReconnectConfig cfg = null;
            if (reconnect != null) {
                cfg = new ItscamLibrary.ITSCAMAutoReconnectConfig();
                cfg.enabled = reconnect.enabled() ? 1 : 0;
                cfg.intervalMs = reconnect.intervalMs();
                cfg.maxRetries = reconnect.maxRetries();
            }
            int rc = lib.ITSCAM_Client_connect(h, address, (short) port,
                    timeoutMs, cfg);
            ItscamException.throwIfFailed(rc, "connect(" + address + ":" + port + ")");
        } finally {
            handle.release();
        }
    }

    public Future<Void> connectAsync(final String address, final int port,
                                     final int timeoutMs,
                                     final AutoReconnectConfig reconnect) {
        return Async.run(new Runnable() {
            @Override
            public void run() {
                connect(address, port, timeoutMs, reconnect);
            }
        });
    }

    public void disconnect() {
        Pointer h = handle.acquire();
        try {
            lib.ITSCAM_Client_disconnect(h);
        } finally {
            handle.release();
        }
    }

    public boolean isConnected() {
        Pointer h = handle.tryAcquire();
        if (h == null) return false;
        try {
            return lib.ITSCAM_Client_isConnected(h) != 0;
        } finally {
            handle.release();
        }
    }

    // ====================================================================
    //  Authentication
    // ====================================================================

    public void authenticate(String password, int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            int rc = lib.ITSCAM_Client_authenticate(h, password, timeoutMs);
            ItscamException.throwIfFailed(rc, "authenticate");
        } finally {
            handle.release();
        }
    }

    public Future<Void> authenticateAsync(final String password,
                                          final int timeoutMs) {
        return Async.run(new Runnable() {
            @Override
            public void run() {
                authenticate(password, timeoutMs);
            }
        });
    }

    // ====================================================================
    //  Subscription
    // ====================================================================

    public void subscribe(EventSubscription events, int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            ItscamLibrary.ITSCAMEventSubscription s =
                    new ItscamLibrary.ITSCAMEventSubscription();
            s.pipeline = events.pipeline() ? 1 : 0;
            s.triggerMetadata = events.triggerMetadata() ? 1 : 0;
            s.triggerImage = events.triggerImage() ? 1 : 0;
            s.snapshotMetadata = events.snapshotMetadata() ? 1 : 0;
            s.snapshotImage = events.snapshotImage() ? 1 : 0;
            s.previewMetadata = events.previewMetadata() ? 1 : 0;
            s.previewImage = events.previewImage() ? 1 : 0;
            s.gpio = events.gpio() ? 1 : 0;
            s.serial1 = events.serial1() ? 1 : 0;
            s.serial2 = events.serial2() ? 1 : 0;
            int rc = lib.ITSCAM_Client_subscribe(h, s, timeoutMs);
            ItscamException.throwIfFailed(rc, "subscribe");
        } finally {
            handle.release();
        }
    }

    public void subscribeCaptures(int timeoutMs) {
        subscribeCaptures(new CaptureSubscriptionConfig(), timeoutMs);
    }

    public void subscribeCaptures(CaptureSubscriptionConfig config,
                                  int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            ItscamLibrary.ITSCAMCaptureSubscriptionConfig c =
                    new ItscamLibrary.ITSCAMCaptureSubscriptionConfig();
            c.includeTrigger = config.includeTrigger() ? 1 : 0;
            c.includeSnapshot = config.includeSnapshot() ? 1 : 0;
            c.includeMetadata = config.includeMetadata() ? 1 : 0;
            c.embedComments = config.embedComments() ? 1 : 0;
            c.embedExif = config.embedExif() ? 1 : 0;
            c.embedSignature = config.embedSignature() ? 1 : 0;
            c.triggerQuality = config.triggerQuality();
            c.snapshotQuality = config.snapshotQuality();
            int rc = lib.ITSCAM_Client_subscribeCaptures(h, c, timeoutMs);
            ItscamException.throwIfFailed(rc, "subscribeCaptures");
        } finally {
            handle.release();
        }
    }

    // ====================================================================
    //  Capture
    // ====================================================================

    public List<CaptureResult> captureSnapshot(int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            ItscamLibrary.ITSCAMSnapshotRequest req =
                    new ItscamLibrary.ITSCAMSnapshotRequest();
            PointerByReference out = new PointerByReference();
            int rc = lib.ITSCAM_Client_captureSnapshot(h, req, timeoutMs, out);
            ItscamException.throwIfFailed(rc, "captureSnapshot");

            Pointer arr = out.getValue();
            try {
                return convertResultArray(arr);
            } finally {
                if (arr != null) lib.ITSCAM_CaptureResultArray_destroy(arr);
            }
        } finally {
            handle.release();
        }
    }

    public Future<List<CaptureResult>> captureSnapshotAsync(
            final int timeoutMs) {
        return Async.submit(new Callable<List<CaptureResult>>() {
            @Override
            public List<CaptureResult> call() {
                return captureSnapshot(timeoutMs);
            }
        });
    }

    public byte[] getLastFrame(int quality, int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            PointerByReference out = new PointerByReference();
            int rc = lib.ITSCAM_Client_getLastFrame(h, quality, timeoutMs, out);
            ItscamException.throwIfFailed(rc, "getLastFrame");

            Pointer arr = out.getValue();
            if (arr == null) return new byte[0];
            try {
                long size = lib.ITSCAM_ByteArray_size(arr);
                Pointer data = lib.ITSCAM_ByteArray_data(arr);
                return data == null || size <= 0 ? new byte[0]
                        : data.getByteArray(0, (int) size);
            } finally {
                lib.ITSCAM_ByteArray_destroy(arr);
            }
        } finally {
            handle.release();
        }
    }

    // ====================================================================
    //  Profiles
    // ====================================================================

    public int getActiveProfileId(int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            IntByReference out = new IntByReference();
            int rc = lib.ITSCAM_Client_getActiveProfileId(h, timeoutMs, out);
            ItscamException.throwIfFailed(rc, "getActiveProfileId");
            return out.getValue();
        } finally {
            handle.release();
        }
    }

    public void setActiveProfile(int profileId, int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            int rc = lib.ITSCAM_Client_setActiveProfile(h, profileId, timeoutMs);
            ItscamException.throwIfFailed(rc, "setActiveProfile");
        } finally {
            handle.release();
        }
    }

    public List<ProfileInfo> listProfiles(int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            PointerByReference out = new PointerByReference();
            int rc = lib.ITSCAM_Client_listProfiles(h, timeoutMs, out);
            ItscamException.throwIfFailed(rc, "listProfiles");

            Pointer arr = out.getValue();
            try {
                List<ProfileInfo> profiles = new ArrayList<>();
                long size = lib.ITSCAM_ProfileArray_size(arr);
                for (long i = 0; i < size; i++) {
                    ItscamLibrary.ITSCAMProfileInfo info =
                            new ItscamLibrary.ITSCAMProfileInfo();
                    if (lib.ITSCAM_ProfileArray_get(arr, i, info) != 0) {
                        profiles.add(new ProfileInfo(info.id,
                                info.name, info.description,
                                info.isActive != 0));
                    }
                }
                return profiles;
            } finally {
                if (arr != null) lib.ITSCAM_ProfileArray_destroy(arr);
            }
        } finally {
            handle.release();
        }
    }

    // ====================================================================
    //  System
    // ====================================================================

    public void reboot(int timeoutMs) {
        Pointer h = handle.acquire();
        try {
            int rc = lib.ITSCAM_Client_reboot(h, timeoutMs);
            ItscamException.throwIfFailed(rc, "reboot");
        } finally {
            handle.release();
        }
    }

    // ====================================================================
    //  Callbacks
    // ====================================================================

    public void onTriggerImage(ItscamConsumer<CaptureResult> callback) {
        setCaptureCallback("trigger", callback, new CaptureSetter() {
            @Override
            public void apply(Pointer h, ItscamLibrary.CaptureCallback cb,
                              Pointer userData) {
                lib.ITSCAM_Client_onTriggerImage(h, cb, userData);
            }
        });
    }

    public void onSnapshotImage(ItscamConsumer<CaptureResult> callback) {
        setCaptureCallback("snapshot", callback, new CaptureSetter() {
            @Override
            public void apply(Pointer h, ItscamLibrary.CaptureCallback cb,
                              Pointer userData) {
                lib.ITSCAM_Client_onSnapshotImage(h, cb, userData);
            }
        });
    }

    public void onDisconnect(final ItscamConsumer<String> callback) {
        setHandler("disconnect", callback, new Trampoline() {
            @Override
            public Object create() {
                return new ItscamLibrary.DisconnectCallback() {
                    @Override
                    public void invoke(String reason, Pointer userData) {
                        @SuppressWarnings("unchecked")
                        ItscamConsumer<String> fn =
                                (ItscamConsumer<String>) handlers.get("disconnect");
                        if (fn == null || handle.isClosed()) return;
                        try { fn.accept(reason == null ? "" : reason); }
                        catch (Throwable ignored) { /* never let exceptions cross FFI */ }
                    }
                };
            }

            @Override
            public void install(Pointer h, Object cb) {
                lib.ITSCAM_Client_onDisconnect(h,
                        (ItscamLibrary.DisconnectCallback) cb, null);
            }
        });
    }

    public void onConnectionState(
            final ItscamBiConsumer<ConnectionState, String> callback) {
        setHandler("connectionState", callback, new Trampoline() {
            @Override
            public Object create() {
                return new ItscamLibrary.ConnectionStateCallback() {
                    @Override
                    public void invoke(int state, String reason, Pointer userData) {
                        @SuppressWarnings("unchecked")
                        ItscamBiConsumer<ConnectionState, String> fn =
                                (ItscamBiConsumer<ConnectionState, String>)
                                        handlers.get("connectionState");
                        if (fn == null || handle.isClosed()) return;
                        try {
                            fn.accept(ConnectionState.fromInt(state),
                                    reason == null ? "" : reason);
                        } catch (Throwable ignored) {}
                    }
                };
            }

            @Override
            public void install(Pointer h, Object cb) {
                lib.ITSCAM_Client_onConnectionState(h,
                        (ItscamLibrary.ConnectionStateCallback) cb, null);
            }
        });
    }

    public void onLog(final ItscamBiConsumer<LogLevel, String> callback) {
        setHandler("log", callback, new Trampoline() {
            @Override
            public Object create() {
                return new ItscamLibrary.LogCallback() {
                    @Override
                    public void invoke(int level, String message, Pointer userData) {
                        @SuppressWarnings("unchecked")
                        ItscamBiConsumer<LogLevel, String> fn =
                                (ItscamBiConsumer<LogLevel, String>) handlers.get("log");
                        if (fn == null || handle.isClosed()) return;
                        try {
                            fn.accept(LogLevel.fromInt(level),
                                    message == null ? "" : message);
                        } catch (Throwable ignored) {}
                    }
                };
            }

            @Override
            public void install(Pointer h, Object cb) {
                lib.ITSCAM_Client_onLog(h, (ItscamLibrary.LogCallback) cb, null);
            }
        });
    }

    private interface CaptureSetter {
        void apply(Pointer h, ItscamLibrary.CaptureCallback cb, Pointer userData);
    }

    private void setCaptureCallback(final String key,
                                    ItscamConsumer<CaptureResult> userCallback,
                                    final CaptureSetter setter) {
        setHandler(key, userCallback, new Trampoline() {
            @Override
            public Object create() {
                return new ItscamLibrary.CaptureCallback() {
                    @Override
                    public void invoke(Pointer resultPtr, Pointer userData) {
                        @SuppressWarnings("unchecked")
                        ItscamConsumer<CaptureResult> fn =
                                (ItscamConsumer<CaptureResult>) handlers.get(key);
                        if (fn == null || handle.isClosed()) return;
                        try {
                            fn.accept(convertResult(resultPtr));
                        } catch (Throwable ignored) {}
                    }
                };
            }

            @Override
            public void install(Pointer h, Object cb) {
                setter.apply(h, (ItscamLibrary.CaptureCallback) cb, null);
            }
        });
    }

    /** Creates and installs the native trampoline of one callback type. */
    private interface Trampoline {
        Object create();
        void install(Pointer h, Object trampoline);
    }

    /**
     * Sets the handler of {@code key}.  Each callback type has one native
     * trampoline, installed the first time and kept until the native client
     * is destroyed, which calls the current handler: installing another one
     * could free a trampoline that a callback thread is about to call.
     */
    private void setHandler(String key, Object handler, Trampoline trampoline) {
        Pointer h = handle.tryAcquire();
        if (h == null) return;
        try {
            if (handler == null) {
                handlers.remove(key);
            } else {
                handlers.put(key, handler);
            }
            Object cb;
            synchronized (callbacks) {
                cb = callbacks.get(key);
                if (cb == null) {
                    if (handler == null) return;
                    cb = trampoline.create();
                    callbacks.put(key, cb);
                    // The first callback of its type: the setter waits for nothing
                    trampoline.install(h, cb);
                    return;
                }
            }
            // Setting the same trampoline again waits for the callback in
            // progress: once this returns, the replaced handler is not running
            // (unless this is called from a callback of the client)
            trampoline.install(h, cb);
        } finally {
            handle.release();
        }
    }

    // ====================================================================
    //  Helpers
    // ====================================================================

    private List<CaptureResult> convertResultArray(Pointer arr) {
        if (arr == null) return new ArrayList<>();
        long size = lib.ITSCAM_CaptureResultArray_size(arr);
        List<CaptureResult> out = new ArrayList<>((int) size);
        for (long i = 0; i < size; i++) {
            Pointer p = lib.ITSCAM_CaptureResultArray_get(arr, i);
            if (p != null) out.add(convertResult(p));
        }
        return out;
    }

    private CaptureResult convertResult(Pointer p) {
        ItscamLibrary.ITSCAMFrameInfo info = lib.ITSCAM_CaptureResult_getInfo(p);
        Timestamp ts = new Timestamp(info.timestamp.year,
                info.timestamp.month, info.timestamp.day,
                info.timestamp.hour, info.timestamp.minute,
                info.timestamp.second, info.timestamp.millisecond,
                info.timestamp.timezoneOffset);

        long plateCount = lib.ITSCAM_CaptureResult_getPlateCount(p);
        List<String> plates = new ArrayList<>((int) plateCount);
        for (long i = 0; i < plateCount; i++) {
            String s = lib.ITSCAM_CaptureResult_getPlate(p, i);
            if (s != null) plates.add(s);
        }

        FrameInfo fi = new FrameInfo(info.requestId, info.frameCount,
                info.multiExpIndex, info.multiExpLength,
                info.shutter, info.gain,
                info.width, info.height, ts, plates);

        LongByReference sizeRef = new LongByReference();
        Pointer data = lib.ITSCAM_CaptureResult_getJpeg(p, sizeRef);
        long sz = sizeRef.getValue();
        byte[] jpeg = (data == null || sz <= 0)
                ? new byte[0] : data.getByteArray(0, (int) sz);
        return new CaptureResult(fi, jpeg);
    }
}
