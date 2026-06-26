/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.internal.ItscamLibrary;
import com.pumatronix.itscam.internal.Async;
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
    private Pointer handle;

    /** Strong references for native callbacks (prevent GC while in use). */
    private final Map<String, Object> callbacks = new ConcurrentHashMap<>();

    public ItscamClient() {
        this.lib = NativeLibrary.get();
        this.handle = lib.ITSCAM_Client_create();
        if (handle == null || handle.equals(Pointer.NULL)) {
            throw new ItscamException(ErrorCode.ALLOCATION_FAILED,
                    "ITSCAM_Client_create returned NULL");
        }
    }

    @Override
    public void close() {
        if (handle != null) {
            lib.ITSCAM_Client_destroy(handle);
            handle = null;
            callbacks.clear();
        }
    }

    private void requireOpen() {
        if (handle == null) {
            throw new IllegalStateException(
                    "ItscamClient has been closed");
        }
    }

    // ====================================================================
    //  Connection
    // ====================================================================

    public void connect(String address, int port, int timeoutMs) {
        connect(address, port, timeoutMs, null);
    }

    public void connect(String address, int port, int timeoutMs,
                        AutoReconnectConfig reconnect) {
        requireOpen();
        ItscamLibrary.ITSCAMAutoReconnectConfig cfg = null;
        if (reconnect != null) {
            cfg = new ItscamLibrary.ITSCAMAutoReconnectConfig();
            cfg.enabled = reconnect.enabled() ? 1 : 0;
            cfg.intervalMs = reconnect.intervalMs();
            cfg.maxRetries = reconnect.maxRetries();
        }
        int rc = lib.ITSCAM_Client_connect(handle, address, (short) port,
                timeoutMs, cfg);
        ItscamException.throwIfFailed(rc, "connect(" + address + ":" + port + ")");
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
        requireOpen();
        lib.ITSCAM_Client_disconnect(handle);
    }

    public boolean isConnected() {
        return handle != null
                && lib.ITSCAM_Client_isConnected(handle) != 0;
    }

    // ====================================================================
    //  Authentication
    // ====================================================================

    public void authenticate(String password, int timeoutMs) {
        requireOpen();
        int rc = lib.ITSCAM_Client_authenticate(handle, password, timeoutMs);
        ItscamException.throwIfFailed(rc, "authenticate");
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
        requireOpen();
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
        int rc = lib.ITSCAM_Client_subscribe(handle, s, timeoutMs);
        ItscamException.throwIfFailed(rc, "subscribe");
    }

    public void subscribeCaptures(int timeoutMs) {
        subscribeCaptures(new CaptureSubscriptionConfig(), timeoutMs);
    }

    public void subscribeCaptures(CaptureSubscriptionConfig config,
                                  int timeoutMs) {
        requireOpen();
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
        int rc = lib.ITSCAM_Client_subscribeCaptures(handle, c, timeoutMs);
        ItscamException.throwIfFailed(rc, "subscribeCaptures");
    }

    // ====================================================================
    //  Capture
    // ====================================================================

    public List<CaptureResult> captureSnapshot(int timeoutMs) {
        requireOpen();
        ItscamLibrary.ITSCAMSnapshotRequest req =
                new ItscamLibrary.ITSCAMSnapshotRequest();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_Client_captureSnapshot(handle, req, timeoutMs, out);
        ItscamException.throwIfFailed(rc, "captureSnapshot");

        Pointer arr = out.getValue();
        try {
            return convertResultArray(arr);
        } finally {
            if (arr != null) lib.ITSCAM_CaptureResultArray_destroy(arr);
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
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_Client_getLastFrame(handle, quality, timeoutMs, out);
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
    }

    // ====================================================================
    //  Profiles
    // ====================================================================

    public int getActiveProfileId(int timeoutMs) {
        requireOpen();
        IntByReference out = new IntByReference();
        int rc = lib.ITSCAM_Client_getActiveProfileId(handle, timeoutMs, out);
        ItscamException.throwIfFailed(rc, "getActiveProfileId");
        return out.getValue();
    }

    public void setActiveProfile(int profileId, int timeoutMs) {
        requireOpen();
        int rc = lib.ITSCAM_Client_setActiveProfile(handle, profileId, timeoutMs);
        ItscamException.throwIfFailed(rc, "setActiveProfile");
    }

    public List<ProfileInfo> listProfiles(int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_Client_listProfiles(handle, timeoutMs, out);
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
    }

    // ====================================================================
    //  System
    // ====================================================================

    public void reboot(int timeoutMs) {
        requireOpen();
        int rc = lib.ITSCAM_Client_reboot(handle, timeoutMs);
        ItscamException.throwIfFailed(rc, "reboot");
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
        if (handle == null) return;
        if (callback == null) {
            callbacks.remove("disconnect");
            lib.ITSCAM_Client_onDisconnect(handle, null, null);
            return;
        }
        ItscamLibrary.DisconnectCallback cb = new ItscamLibrary.DisconnectCallback() {
            @Override
            public void invoke(String reason, Pointer userData) {
                try { callback.accept(reason == null ? "" : reason); }
                catch (Throwable ignored) { /* never let exceptions cross FFI */ }
            }
        };
        callbacks.put("disconnect", cb);
        lib.ITSCAM_Client_onDisconnect(handle, cb, null);
    }

    public void onConnectionState(
            final ItscamBiConsumer<ConnectionState, String> callback) {
        if (handle == null) return;
        if (callback == null) {
            callbacks.remove("connectionState");
            lib.ITSCAM_Client_onConnectionState(handle, null, null);
            return;
        }
        ItscamLibrary.ConnectionStateCallback cb = new ItscamLibrary.ConnectionStateCallback() {
            @Override
            public void invoke(int state, String reason, Pointer userData) {
                try {
                    callback.accept(ConnectionState.fromInt(state),
                            reason == null ? "" : reason);
                } catch (Throwable ignored) {}
            }
        };
        callbacks.put("connectionState", cb);
        lib.ITSCAM_Client_onConnectionState(handle, cb, null);
    }

    public void onLog(final ItscamBiConsumer<LogLevel, String> callback) {
        if (handle == null) return;
        if (callback == null) {
            callbacks.remove("log");
            lib.ITSCAM_Client_onLog(handle, null, null);
            return;
        }
        ItscamLibrary.LogCallback cb = new ItscamLibrary.LogCallback() {
            @Override
            public void invoke(int level, String message, Pointer userData) {
                try {
                    callback.accept(LogLevel.fromInt(level),
                            message == null ? "" : message);
                } catch (Throwable ignored) {}
            }
        };
        callbacks.put("log", cb);
        lib.ITSCAM_Client_onLog(handle, cb, null);
    }

    private interface CaptureSetter {
        void apply(Pointer h, ItscamLibrary.CaptureCallback cb, Pointer userData);
    }

    private void setCaptureCallback(String key,
                                    final ItscamConsumer<CaptureResult> userCallback,
                                    CaptureSetter setter) {
        if (handle == null) return;
        if (userCallback == null) {
            callbacks.remove(key);
            setter.apply(handle, null, null);
            return;
        }
        ItscamLibrary.CaptureCallback cb = new ItscamLibrary.CaptureCallback() {
            @Override
            public void invoke(Pointer resultPtr, Pointer userData) {
                try {
                    CaptureResult r = convertResult(resultPtr);
                    userCallback.accept(r);
                } catch (Throwable ignored) {}
            }
        };
        callbacks.put(key, cb);
        setter.apply(handle, cb, null);
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
