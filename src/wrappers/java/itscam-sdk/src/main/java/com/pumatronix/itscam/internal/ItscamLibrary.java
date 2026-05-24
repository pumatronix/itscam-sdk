/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * ItscamLibrary -- JNA Library interface for libitscam_sdk.{so,dll,dylib}.
 *
 * Mirrors src/core/c_api/*.h.  All strings are marshalled as UTF-8 by
 * JNA.  Pointer arguments are mapped to com.sun.jna.Pointer, struct
 * arguments and return values use JNA Structure subclasses.
 */
package com.pumatronix.itscam.internal;

import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

import java.util.Arrays;
import java.util.List;

public interface ItscamLibrary extends Library {

    // ====================================================================
    //  Structures (POD layout, must match the C headers byte-for-byte)
    // ====================================================================

    @Structure.FieldOrder({
            "year", "month", "day",
            "hour", "minute", "second",
            "millisecond", "timezoneOffset"
    })
    class ITSCAMTimestamp extends Structure {
        public short year;
        public short month;
        public short day;
        public short hour;
        public short minute;
        public short second;
        public short millisecond;
        public int timezoneOffset;

        public ITSCAMTimestamp() {}

        public ITSCAMTimestamp(Pointer p) { super(p); read(); }

        public static class ByValue extends ITSCAMTimestamp
                implements Structure.ByValue {}
    }

    @Structure.FieldOrder({
            "requestId", "frameCount",
            "multiExpIndex", "multiExpLength",
            "shutter", "gain",
            "width", "height", "timestamp"
    })
    class ITSCAMFrameInfo extends Structure {
        public long  requestId;
        public long  frameCount;
        public int   multiExpIndex;
        public int   multiExpLength;
        public int   shutter;
        public float gain;
        public int   width;
        public int   height;
        public ITSCAMTimestamp timestamp;

        public ITSCAMFrameInfo() {}

        public ITSCAMFrameInfo(Pointer p) { super(p); read(); }

        public static class ByValue extends ITSCAMFrameInfo
                implements Structure.ByValue {}
    }

    @Structure.FieldOrder({"enabled", "intervalMs", "maxRetries"})
    class ITSCAMAutoReconnectConfig extends Structure {
        public int enabled;
        public int intervalMs;
        public int maxRetries;
    }

    @Structure.FieldOrder({
            "pipeline", "triggerMetadata", "triggerImage",
            "snapshotMetadata", "snapshotImage",
            "previewMetadata", "previewImage",
            "gpio", "serial1", "serial2"
    })
    class ITSCAMEventSubscription extends Structure {
        public int pipeline;
        public int triggerMetadata;
        public int triggerImage;
        public int snapshotMetadata;
        public int snapshotImage;
        public int previewMetadata;
        public int previewImage;
        public int gpio;
        public int serial1;
        public int serial2;
    }

    @Structure.FieldOrder({
            "includeTrigger", "includeSnapshot", "includeMetadata",
            "embedComments", "embedExif", "embedSignature",
            "triggerQuality", "snapshotQuality"
    })
    class ITSCAMCaptureSubscriptionConfig extends Structure {
        public int includeTrigger;
        public int includeSnapshot;
        public int includeMetadata;
        public int embedComments;
        public int embedExif;
        public int embedSignature;
        public int triggerQuality;
        public int snapshotQuality;
    }

    @Structure.FieldOrder({"reserved"})
    class ITSCAMSnapshotRequest extends Structure {
        public int reserved;
    }

    @Structure.FieldOrder({"id", "name", "description", "isActive"})
    class ITSCAMProfileInfo extends Structure {
        public int     id;
        public String  name;
        public String  description;
        public int     isActive;

        public ITSCAMProfileInfo() {}
        public ITSCAMProfileInfo(Pointer p) { super(p); read(); }
    }

    @Structure.FieldOrder({
            "shutters", "shuttersLen",
            "gains", "gainsLen",
            "quality", "mosaic", "format",
            "scenario", "crop", "textOverlay",
            "userMetadataKeys", "userMetadataValues"
    })
    class ITSCAMCgiSnapshotRequest extends Structure {
        public Pointer shutters;
        public long    shuttersLen;
        public Pointer gains;
        public long    gainsLen;
        public int     quality;
        public int     mosaic;
        public String  format;
        public int     scenario;
        public String  crop;
        public String  textOverlay;
        public Pointer userMetadataKeys;
        public Pointer userMetadataValues;
    }

    @Structure.FieldOrder({"sequence", "mimeType", "data", "dataLen"})
    class ITSCAMCgiStreamFrame extends Structure {
        public long    sequence;
        public String  mimeType;
        public Pointer data;
        public long    dataLen;

        public ITSCAMCgiStreamFrame() {}
        public ITSCAMCgiStreamFrame(Pointer p) { super(p); read(); }
    }

    // ====================================================================
    //  Callbacks
    // ====================================================================

    interface CaptureCallback extends Callback {
        void invoke(Pointer result, Pointer userData);
    }

    interface DisconnectCallback extends Callback {
        void invoke(String reason, Pointer userData);
    }

    interface ConnectionStateCallback extends Callback {
        void invoke(int state, String reason, Pointer userData);
    }

    interface LogCallback extends Callback {
        void invoke(int level, String message, Pointer userData);
    }

    interface CgiStreamCallback extends Callback {
        void invoke(ITSCAMCgiStreamFrame frame, Pointer userData);
    }

    // ====================================================================
    //  Binary client (ITSCAM_Client)
    // ====================================================================

    Pointer ITSCAM_Client_create();
    void    ITSCAM_Client_destroy(Pointer client);

    int ITSCAM_Client_connect(Pointer client, String address, short port,
                              int timeoutMs,
                              ITSCAMAutoReconnectConfig reconnect);
    void ITSCAM_Client_disconnect(Pointer client);
    int  ITSCAM_Client_isConnected(Pointer client);

    int ITSCAM_Client_authenticate(Pointer client, String password,
                                   int timeoutMs);

    int ITSCAM_Client_subscribe(Pointer client,
                                ITSCAMEventSubscription events,
                                int timeoutMs);
    int ITSCAM_Client_subscribeCaptures(
            Pointer client, ITSCAMCaptureSubscriptionConfig config,
            int timeoutMs);

    int ITSCAM_Client_captureSnapshot(Pointer client,
                                      ITSCAMSnapshotRequest request,
                                      int timeoutMs,
                                      PointerByReference outResult);
    int ITSCAM_Client_getLastFrame(Pointer client, int quality,
                                   int timeoutMs,
                                   PointerByReference outJpeg);

    int ITSCAM_Client_getActiveProfileId(Pointer client, int timeoutMs,
                                         IntByReference outId);
    int ITSCAM_Client_setActiveProfile(Pointer client, int profileId,
                                       int timeoutMs);
    int ITSCAM_Client_listProfiles(Pointer client, int timeoutMs,
                                   PointerByReference outProfiles);

    int ITSCAM_Client_reboot(Pointer client, int timeoutMs);

    void ITSCAM_Client_onTriggerImage(Pointer client,
                                      CaptureCallback cb,
                                      Pointer userData);
    void ITSCAM_Client_onSnapshotImage(Pointer client,
                                       CaptureCallback cb,
                                       Pointer userData);
    void ITSCAM_Client_onDisconnect(Pointer client,
                                    DisconnectCallback cb,
                                    Pointer userData);
    void ITSCAM_Client_onConnectionState(Pointer client,
                                         ConnectionStateCallback cb,
                                         Pointer userData);
    void ITSCAM_Client_onLog(Pointer client, LogCallback cb,
                             Pointer userData);

    // CaptureResult accessors
    long    ITSCAM_CaptureResultArray_size(Pointer array);
    Pointer ITSCAM_CaptureResultArray_get(Pointer array, long index);
    void    ITSCAM_CaptureResultArray_destroy(Pointer array);
    ITSCAMFrameInfo.ByValue ITSCAM_CaptureResult_getInfo(Pointer result);
    Pointer ITSCAM_CaptureResult_getJpeg(Pointer result,
                                         com.sun.jna.ptr.LongByReference outSize);
    long    ITSCAM_CaptureResult_getPlateCount(Pointer result);
    String  ITSCAM_CaptureResult_getPlate(Pointer result, long index);

    // ByteArray accessors
    long    ITSCAM_ByteArray_size(Pointer array);
    Pointer ITSCAM_ByteArray_data(Pointer array);
    void    ITSCAM_ByteArray_destroy(Pointer array);

    // ProfileArray accessors
    long ITSCAM_ProfileArray_size(Pointer array);
    int  ITSCAM_ProfileArray_get(Pointer array, long index,
                                 ITSCAMProfileInfo outInfo);
    void ITSCAM_ProfileArray_destroy(Pointer array);

    // JPEG comment helper
    long ITSCAM_Jpeg_extractComment(byte[] jpegData, long jpegSize,
                                    byte[] outBuf, long bufSize);

    // System utilities
    ITSCAMTimestamp.ByValue ITSCAM_getSystemLocalTime();
    ITSCAMTimestamp.ByValue ITSCAM_getSystemUtcTime();
    long   ITSCAM_getEpochTime();
    long   ITSCAM_getEpochTimeMs();
    int    ITSCAM_storeFile(String path, byte[] data, long size,
                            int overwrite);
    int    ITSCAM_createFolder(String path, int recursive);
    int    ITSCAM_fileExists(String path);
    int    ITSCAM_folderExists(String path);
    String ITSCAM_getLastError();
    String ITSCAM_getVersion();

    // ====================================================================
    //  REST client (ITSCAM_RestClient)
    // ====================================================================

    Pointer ITSCAM_RestClient_create();
    void    ITSCAM_RestClient_destroy(Pointer client);

    int  ITSCAM_RestClient_setBaseUrl(Pointer client, String host,
                                      short port, String scheme);
    void ITSCAM_RestClient_setApiPrefix(Pointer client, String prefix);
    void ITSCAM_RestClient_setCaCertFile(Pointer client, String pemPath);
    void ITSCAM_RestClient_setCaCertData(Pointer client, String pem);
    void ITSCAM_RestClient_setVerifyServerCertificate(Pointer client,
                                                      int verify);
    void ITSCAM_RestClient_setClientCertificate(Pointer client,
                                                 String certPem,
                                                 String keyPem);

    int  ITSCAM_RestClient_login(Pointer client, String username,
                                 String password, int timeoutMs,
                                 PointerByReference outResponse);
    void ITSCAM_RestClient_setAuthToken(Pointer client, String token);
    void ITSCAM_RestClient_clearAuthToken(Pointer client);

    int ITSCAM_RestClient_httpGet(Pointer client, String path,
                                  int timeoutMs,
                                  PointerByReference outResponse);
    int ITSCAM_RestClient_httpPut(Pointer client, String path,
                                  String jsonBody, int timeoutMs,
                                  PointerByReference outResponse);
    int ITSCAM_RestClient_httpPost(Pointer client, String path,
                                   String jsonBody, int timeoutMs,
                                   PointerByReference outResponse);
    int ITSCAM_RestClient_httpDelete(Pointer client, String path,
                                     int timeoutMs,
                                     PointerByReference outResponse);

    int ITSCAM_RestClient_getProfiles(Pointer client, int timeoutMs,
                                      PointerByReference outResponse);
    int ITSCAM_RestClient_getProfile(Pointer client, int profileId,
                                     int timeoutMs,
                                     PointerByReference outResponse);
    int ITSCAM_RestClient_createProfile(Pointer client, String jsonProfile,
                                        int timeoutMs,
                                        PointerByReference outResponse);
    int ITSCAM_RestClient_updateProfile(Pointer client, String jsonProfile,
                                        int timeoutMs,
                                        PointerByReference outResponse);
    int ITSCAM_RestClient_deleteProfile(Pointer client, int profileId,
                                        int timeoutMs,
                                        PointerByReference outResponse);

    int ITSCAM_RestClient_getVolatileInfo(Pointer client, int timeoutMs,
                                          PointerByReference outResponse);
    int ITSCAM_RestClient_getGeneralConfig(Pointer client, int timeoutMs,
                                           PointerByReference outResponse);
    int ITSCAM_RestClient_setGeneralConfig(Pointer client, String json,
                                           int timeoutMs,
                                           PointerByReference outResponse);
    int ITSCAM_RestClient_getOcrConfig(Pointer client, int timeoutMs,
                                       PointerByReference outResponse);
    int ITSCAM_RestClient_setOcrConfig(Pointer client, String json,
                                       int timeoutMs,
                                       PointerByReference outResponse);
    int ITSCAM_RestClient_getAnalyticsConfig(Pointer client, int timeoutMs,
                                              PointerByReference outResponse);
    int ITSCAM_RestClient_setAnalyticsConfig(Pointer client, String json,
                                              int timeoutMs,
                                              PointerByReference outResponse);
    int ITSCAM_RestClient_getClassifierConfig(Pointer client, int timeoutMs,
                                               PointerByReference outResponse);
    int ITSCAM_RestClient_setClassifierConfig(Pointer client, String json,
                                               int timeoutMs,
                                               PointerByReference outResponse);
    int ITSCAM_RestClient_getLanesConfig(Pointer client, int timeoutMs,
                                          PointerByReference outResponse);
    int ITSCAM_RestClient_setLanesConfig(Pointer client, String json,
                                          int timeoutMs,
                                          PointerByReference outResponse);
    int ITSCAM_RestClient_getItscamproConfig(Pointer client, int timeoutMs,
                                              PointerByReference outResponse);
    int ITSCAM_RestClient_setItscamproConfig(Pointer client, String json,
                                              int timeoutMs,
                                              PointerByReference outResponse);
    int ITSCAM_RestClient_getItscamproStatus(Pointer client, int timeoutMs,
                                              PointerByReference outResponse);

    void ITSCAM_RestClient_onLog(Pointer client, LogCallback cb,
                                 Pointer userData);

    // ITSCAM_String accessors
    String ITSCAM_String_data(Pointer s);
    long   ITSCAM_String_size(Pointer s);
    void   ITSCAM_String_destroy(Pointer s);

    // ====================================================================
    //  CGI client (ITSCAM_CgiClient)
    // ====================================================================

    Pointer ITSCAM_CgiClient_create();
    void    ITSCAM_CgiClient_destroy(Pointer client);

    int  ITSCAM_CgiClient_setBaseUrl(Pointer client, String host,
                                     short port, String scheme);
    void ITSCAM_CgiClient_setApiPrefix(Pointer client, String prefix);
    void ITSCAM_CgiClient_setCaCertFile(Pointer client, String pemPath);
    void ITSCAM_CgiClient_setCaCertData(Pointer client, String pem);
    void ITSCAM_CgiClient_setVerifyServerCertificate(Pointer client,
                                                      int verify);
    void ITSCAM_CgiClient_setClientCertificate(Pointer client,
                                                String certPem,
                                                String keyPem);

    int  ITSCAM_CgiClient_login(Pointer client, String username,
                                String password, int timeoutMs);
    void ITSCAM_CgiClient_setAuthToken(Pointer client, String token);
    void ITSCAM_CgiClient_clearAuthToken(Pointer client);
    void ITSCAM_CgiClient_setBasicAuth(Pointer client, String user,
                                        String password);
    void ITSCAM_CgiClient_clearBasicAuth(Pointer client);

    int ITSCAM_CgiClient_getLastFrame(Pointer client, int timeoutMs,
                                      PointerByReference outImage);
    int ITSCAM_CgiClient_getSnapshot(Pointer client,
                                     ITSCAMCgiSnapshotRequest request,
                                     int timeoutMs,
                                     PointerByReference outImages);
    int ITSCAM_CgiClient_startMjpegStream(Pointer client,
                                          CgiStreamCallback cb,
                                          Pointer userData,
                                          int timeoutMs);
    void ITSCAM_CgiClient_stopMjpegStream(Pointer client);
    int  ITSCAM_CgiClient_isMjpegStreamRunning(Pointer client);

    int ITSCAM_CgiClient_forceTrigger(Pointer client, int timeoutMs,
                                       PointerByReference outResponse);
    int ITSCAM_CgiClient_reboot(Pointer client, int timeoutMs,
                                 PointerByReference outResponse);

    void ITSCAM_CgiClient_onLog(Pointer client, LogCallback cb,
                                Pointer userData);

    // CgiImage / CgiImageArray accessors
    String  ITSCAM_CgiImage_mimeType(Pointer img);
    Pointer ITSCAM_CgiImage_data(Pointer img);
    long    ITSCAM_CgiImage_size(Pointer img);
    void    ITSCAM_CgiImage_destroy(Pointer img);

    long    ITSCAM_CgiImageArray_size(Pointer arr);
    Pointer ITSCAM_CgiImageArray_get(Pointer arr, long index);
    void    ITSCAM_CgiImageArray_destroy(Pointer arr);
}
