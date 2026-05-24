/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.internal.ItscamLibrary;
import com.pumatronix.itscam.internal.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.PointerByReference;

import java.util.concurrent.CompletableFuture;

/**
 * REST/JSON client for the ITSCAM webapp on port 80/443.  Use this
 * client for equipment configuration: profiles, OCR, classifier, lanes,
 * analytics, ITSCAM PRO server hooks, networking, etc.
 *
 * <h2>Authentication</h2>
 *
 * <p><strong>REST always requires authentication.</strong>  Call
 * {@link #login(String, String, int)} (or
 * {@link #setAuthToken(String)}) before any other method.
 *
 * <h2>Two surfaces, one client</h2>
 *
 * <ul>
 *   <li><b>Generic verbs</b> &mdash; {@link #httpGet}, {@link #httpPut},
 *       {@link #httpPost}, {@link #httpDelete} return the raw JSON
 *       response as a {@link String}.  Use these for partial updates
 *       (preferred) and for endpoints not yet covered by a typed
 *       helper.</li>
 *   <li><b>Typed convenience helpers</b> &mdash; {@link #getProfiles},
 *       {@link #setOcrConfig}, {@link #setItscamproConfig}, etc. wrap
 *       the typed C-API endpoints.  They return raw JSON; use a JSON
 *       library (Jackson, Gson, ...) of your choice to deserialise
 *       into POJOs.</li>
 * </ul>
 *
 * <h2>Partial updates</h2>
 *
 * <p>The ITSCAM daemon merges {@code PUT} bodies, so prefer
 * {@link #patchJson(String, String, int)} (which is just an alias for
 * {@code httpPut}) and send only the fields you intend to change.
 * Round-tripping a full {@code ProfileConfig} document via
 * {@code PUT /api/image/profiles/{id}} returns HTTP 500.
 */
public final class ItscamRestClient implements AutoCloseable {

    private final ItscamLibrary lib;
    private Pointer handle;

    public ItscamRestClient() {
        this.lib = NativeLibrary.get();
        this.handle = lib.ITSCAM_RestClient_create();
        if (handle == null || handle.equals(Pointer.NULL)) {
            throw new ItscamException(ErrorCode.ALLOCATION_FAILED,
                    "ITSCAM_RestClient_create returned NULL");
        }
    }

    @Override
    public void close() {
        if (handle != null) {
            lib.ITSCAM_RestClient_destroy(handle);
            handle = null;
        }
    }

    private void requireOpen() {
        if (handle == null) {
            throw new IllegalStateException("ItscamRestClient closed");
        }
    }

    // ====================================================================
    //  Configuration
    // ====================================================================

    public void setBaseUrl(String host, int port, String scheme) {
        requireOpen();
        int rc = lib.ITSCAM_RestClient_setBaseUrl(handle, host,
                (short) port, scheme == null ? "http" : scheme);
        ItscamException.throwIfFailed(rc, "setBaseUrl(" + host + ":" + port + ")");
    }

    public void setApiPrefix(String prefix) {
        requireOpen();
        lib.ITSCAM_RestClient_setApiPrefix(handle, prefix);
    }

    public void setCaCertFile(String pemPath) {
        requireOpen();
        lib.ITSCAM_RestClient_setCaCertFile(handle, pemPath);
    }

    public void setCaCertData(String pem) {
        requireOpen();
        lib.ITSCAM_RestClient_setCaCertData(handle, pem);
    }

    public void setVerifyServerCertificate(boolean verify) {
        requireOpen();
        lib.ITSCAM_RestClient_setVerifyServerCertificate(handle, verify ? 1 : 0);
    }

    public void setClientCertificate(String certPem, String keyPem) {
        requireOpen();
        lib.ITSCAM_RestClient_setClientCertificate(handle, certPem, keyPem);
    }

    // ====================================================================
    //  Authentication
    // ====================================================================

    public String login(String username, String password, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_login(handle, username, password,
                timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "login");
        return body;
    }

    public CompletableFuture<String> loginAsync(String username, String password,
                                                int timeoutMs) {
        return CompletableFuture.supplyAsync(
                () -> login(username, password, timeoutMs));
    }

    public void setAuthToken(String token) {
        requireOpen();
        lib.ITSCAM_RestClient_setAuthToken(handle, token);
    }

    public void clearAuthToken() {
        requireOpen();
        lib.ITSCAM_RestClient_clearAuthToken(handle);
    }

    // ====================================================================
    //  Generic HTTP verbs
    // ====================================================================

    public String httpGet(String path, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_httpGet(handle, path, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "GET " + path);
        return body;
    }

    public String httpPut(String path, String jsonBody, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_httpPut(handle, path,
                jsonBody == null ? "" : jsonBody, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "PUT " + path);
        return body;
    }

    public String httpPost(String path, String jsonBody, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_httpPost(handle, path,
                jsonBody == null ? "" : jsonBody, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "POST " + path);
        return body;
    }

    public String httpDelete(String path, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_httpDelete(handle, path, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "DELETE " + path);
        return body;
    }

    /**
     * Send a partial JSON body via HTTP PUT.  Alias of {@link #httpPut}.
     * Use this for ITSCAM daemon endpoints that merge incoming bodies
     * (notably {@code /api/image/profiles/{id}} which rejects full
     * documents with HTTP 500).
     */
    public String patchJson(String path, String partialJson, int timeoutMs) {
        return httpPut(path, partialJson, timeoutMs);
    }

    public CompletableFuture<String> getAsync(String path, int timeoutMs) {
        return CompletableFuture.supplyAsync(() -> httpGet(path, timeoutMs));
    }

    public CompletableFuture<String> putAsync(String path, String body,
                                              int timeoutMs) {
        return CompletableFuture.supplyAsync(() -> httpPut(path, body, timeoutMs));
    }

    public CompletableFuture<String> postAsync(String path, String body,
                                               int timeoutMs) {
        return CompletableFuture.supplyAsync(() -> httpPost(path, body, timeoutMs));
    }

    public CompletableFuture<String> deleteAsync(String path, int timeoutMs) {
        return CompletableFuture.supplyAsync(() -> httpDelete(path, timeoutMs));
    }

    // ====================================================================
    //  Typed convenience helpers
    //
    //  These wrap the ITSCAM_RestClient_get/setXxx C entry points and
    //  return the raw JSON string.  Plug your favourite JSON library
    //  (Jackson, Gson, JSON-B) on top to deserialise into POJOs.
    // ====================================================================

    public String getProfiles(int timeoutMs) {
        return invokeJson("getProfiles", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getProfiles(h, t, o));
    }

    public String getProfile(int profileId, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_getProfile(handle, profileId,
                timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "getProfile(" + profileId + ")");
        return body;
    }

    public String createProfile(String jsonProfile, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_createProfile(handle, jsonProfile,
                timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "createProfile");
        return body;
    }

    public String updateProfile(String jsonProfile, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_updateProfile(handle, jsonProfile,
                timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "updateProfile");
        return body;
    }

    public String deleteProfile(int profileId, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_deleteProfile(handle, profileId,
                timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "deleteProfile(" + profileId + ")");
        return body;
    }

    public String getVolatileInfo(int timeoutMs) {
        return invokeJson("getVolatileInfo", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getVolatileInfo(h, t, o));
    }

    public String getGeneralConfig(int timeoutMs) {
        return invokeJson("getGeneralConfig", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getGeneralConfig(h, t, o));
    }

    public String setGeneralConfig(String json, int timeoutMs) {
        return invokeJsonWithBody("setGeneralConfig", json, timeoutMs,
                (h, j, t, o) -> lib.ITSCAM_RestClient_setGeneralConfig(h, j, t, o));
    }

    public String getOcrConfig(int timeoutMs) {
        return invokeJson("getOcrConfig", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getOcrConfig(h, t, o));
    }

    public String setOcrConfig(String json, int timeoutMs) {
        return invokeJsonWithBody("setOcrConfig", json, timeoutMs,
                (h, j, t, o) -> lib.ITSCAM_RestClient_setOcrConfig(h, j, t, o));
    }

    public String getAnalyticsConfig(int timeoutMs) {
        return invokeJson("getAnalyticsConfig", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getAnalyticsConfig(h, t, o));
    }

    public String setAnalyticsConfig(String json, int timeoutMs) {
        return invokeJsonWithBody("setAnalyticsConfig", json, timeoutMs,
                (h, j, t, o) -> lib.ITSCAM_RestClient_setAnalyticsConfig(h, j, t, o));
    }

    public String getClassifierConfig(int timeoutMs) {
        return invokeJson("getClassifierConfig", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getClassifierConfig(h, t, o));
    }

    public String setClassifierConfig(String json, int timeoutMs) {
        return invokeJsonWithBody("setClassifierConfig", json, timeoutMs,
                (h, j, t, o) -> lib.ITSCAM_RestClient_setClassifierConfig(h, j, t, o));
    }

    public String getLanesConfig(int timeoutMs) {
        return invokeJson("getLanesConfig", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getLanesConfig(h, t, o));
    }

    public String setLanesConfig(String json, int timeoutMs) {
        return invokeJsonWithBody("setLanesConfig", json, timeoutMs,
                (h, j, t, o) -> lib.ITSCAM_RestClient_setLanesConfig(h, j, t, o));
    }

    public String getItscamproConfig(int timeoutMs) {
        return invokeJson("getItscamproConfig", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getItscamproConfig(h, t, o));
    }

    public String setItscamproConfig(String json, int timeoutMs) {
        return invokeJsonWithBody("setItscamproConfig", json, timeoutMs,
                (h, j, t, o) -> lib.ITSCAM_RestClient_setItscamproConfig(h, j, t, o));
    }

    public String getItscamproStatus(int timeoutMs) {
        return invokeJson("getItscamproStatus", timeoutMs,
                (h, t, o) -> lib.ITSCAM_RestClient_getItscamproStatus(h, t, o));
    }

    // ====================================================================
    //  Helpers
    // ====================================================================

    @FunctionalInterface
    private interface JsonGet {
        int call(Pointer h, int timeoutMs, PointerByReference out);
    }

    @FunctionalInterface
    private interface JsonPut {
        int call(Pointer h, String body, int timeoutMs,
                 PointerByReference out);
    }

    private String invokeJson(String label, int timeoutMs, JsonGet fn) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = fn.call(handle, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, label);
        return body;
    }

    private String invokeJsonWithBody(String label, String json,
                                      int timeoutMs, JsonPut fn) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = fn.call(handle, json == null ? "" : json, timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, label);
        return body;
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
