/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.internal.Async;
import com.pumatronix.itscam.internal.ItscamLibrary;
import com.pumatronix.itscam.internal.NativeLibrary;
import com.pumatronix.itscam.resttypes.AnalyticsConfig;
import com.pumatronix.itscam.resttypes.AutoFocus;
import com.pumatronix.itscam.resttypes.ClassifierConfig;
import com.pumatronix.itscam.resttypes.FtpConfig;
import com.pumatronix.itscam.resttypes.ImageSignConfig;
import com.pumatronix.itscam.resttypes.IoBasic;
import com.pumatronix.itscam.resttypes.IoConfig;
import com.pumatronix.itscam.resttypes.ItscamproConfig;
import com.pumatronix.itscam.resttypes.ItscamproStatus;
import com.pumatronix.itscam.resttypes.LanesConfig;
import com.pumatronix.itscam.resttypes.Licenses;
import com.pumatronix.itscam.resttypes.LinceConfig;
import com.pumatronix.itscam.resttypes.LinceStatus;
import com.pumatronix.itscam.resttypes.Misc;
import com.pumatronix.itscam.resttypes.MiscVolatile;
import com.pumatronix.itscam.resttypes.OcrConfig;
import com.pumatronix.itscam.resttypes.ProfileConfig;
import com.pumatronix.itscam.resttypes.ProfileTransitioner;
import com.pumatronix.itscam.resttypes.ProtocolsConfig;
import com.pumatronix.itscam.resttypes.RestApiClientConfig;
import com.pumatronix.itscam.resttypes.RestApiClientStatus;
import com.pumatronix.itscam.resttypes.RestObject;
import com.pumatronix.itscam.resttypes.StreamConfig;
import com.pumatronix.itscam.resttypes.VehicleIndicatorConfig;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.PointerByReference;

import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.Future;

/** REST/JSON client for the ITSCAM webapp on port 80/443. */
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

    public String login(String username, String password, int timeoutMs) {
        requireOpen();
        PointerByReference out = new PointerByReference();
        int rc = lib.ITSCAM_RestClient_login(handle, username, password,
                timeoutMs, out);
        String body = takeString(out.getValue());
        ItscamException.throwIfFailed(rc, "login");
        return body;
    }

    public Future<String> loginAsync(final String username, final String password,
                                     final int timeoutMs) {
        return Async.submit(new Callable<String>() {
            @Override
            public String call() {
                return login(username, password, timeoutMs);
            }
        });
    }

    public void setAuthToken(String token) {
        requireOpen();
        lib.ITSCAM_RestClient_setAuthToken(handle, token);
    }

    public void clearAuthToken() {
        requireOpen();
        lib.ITSCAM_RestClient_clearAuthToken(handle);
    }

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

    public String patchJson(String path, String partialJson, int timeoutMs) {
        return httpPut(path, partialJson, timeoutMs);
    }

    public String patchJson(String path, RestObject patch, int timeoutMs) {
        return httpPut(path, require(patch, "patch").toJsonString(), timeoutMs);
    }

    public Future<String> getAsync(final String path, final int timeoutMs) {
        return Async.submit(new Callable<String>() {
            @Override
            public String call() {
                return httpGet(path, timeoutMs);
            }
        });
    }

    public Future<String> putAsync(final String path, final String body,
                                   final int timeoutMs) {
        return Async.submit(new Callable<String>() {
            @Override
            public String call() {
                return httpPut(path, body, timeoutMs);
            }
        });
    }

    public Future<String> postAsync(final String path, final String body,
                                    final int timeoutMs) {
        return Async.submit(new Callable<String>() {
            @Override
            public String call() {
                return httpPost(path, body, timeoutMs);
            }
        });
    }

    public Future<String> deleteAsync(final String path, final int timeoutMs) {
        return Async.submit(new Callable<String>() {
            @Override
            public String call() {
                return httpDelete(path, timeoutMs);
            }
        });
    }

    public String getProfilesJson(int timeoutMs) { return httpGet("/api/image/profiles", timeoutMs); }
    public String getProfileJson(int profileId, int timeoutMs) { return httpGet("/api/image/profiles?id=" + profileId, timeoutMs); }
    public String createProfileJson(String jsonProfile, int timeoutMs) { return httpPost("/api/image/profiles", jsonProfile, timeoutMs); }
    public String updateProfileJson(int profileId, String jsonProfile, int timeoutMs) { return httpPut("/api/image/profiles/" + profileId, jsonProfile, timeoutMs); }
    public String updateProfilesJson(String jsonProfiles, int timeoutMs) { return httpPut("/api/image/profiles", jsonProfiles, timeoutMs); }
    public String getVolatileInfoJson(int timeoutMs) { return httpGet("/api/equipment/misc/readonly/volatile", timeoutMs); }
    public String getGeneralConfigJson(int timeoutMs) { return httpGet("/api/equipment/misc", timeoutMs); }
    public String setGeneralConfigJson(String json, int timeoutMs) { return httpPut("/api/equipment/misc", json, timeoutMs); }
    public String getOcrConfigJson(int timeoutMs) { return httpGet("/api/equipment/ocr", timeoutMs); }
    public String setOcrConfigJson(String json, int timeoutMs) { return httpPut("/api/equipment/ocr", json, timeoutMs); }
    public String getAnalyticsConfigJson(int timeoutMs) { return httpGet("/api/equipment/analytics", timeoutMs); }
    public String setAnalyticsConfigJson(String json, int timeoutMs) { return httpPut("/api/equipment/analytics", json, timeoutMs); }
    public String getClassifierConfigJson(int timeoutMs) { return httpGet("/api/equipment/classifier", timeoutMs); }
    public String setClassifierConfigJson(String json, int timeoutMs) { return httpPut("/api/equipment/classifier", json, timeoutMs); }
    public String getLanesConfigJson(int timeoutMs) { return httpGet("/api/equipment/lanes", timeoutMs); }
    public String setLanesConfigJson(String json, int timeoutMs) { return httpPut("/api/equipment/lanes", json, timeoutMs); }
    public String getItscamproConfigJson(int timeoutMs) { return httpGet("/api/equipment/servers/itscampro", timeoutMs); }
    public String setItscamproConfigJson(String json, int timeoutMs) { return httpPut("/api/equipment/servers/itscampro", json, timeoutMs); }
    public String getItscamproStatusJson(int timeoutMs) { return httpGet("/api/equipment/servers/itscampro/status", timeoutMs); }

    public List<ProfileConfig> getProfiles(int timeoutMs) {
        return ProfileConfig.listFromJson(getProfilesJson(timeoutMs));
    }

    public List<ProfileConfig> getProfile(int profileId, int timeoutMs) {
        return ProfileConfig.listFromJson(getProfileJson(profileId, timeoutMs));
    }

    public ProfileConfig getProfileByName(String name, int timeoutMs) {
        if (name == null) throw new IllegalArgumentException("name is null");
        List<ProfileConfig> profiles = getProfiles(timeoutMs);
        for (ProfileConfig profile : profiles) {
            if (name.equals(profile.getName())) return profile;
        }
        throw new ItscamInvalidParameterException("profile not found: \"" + name + "\"");
    }

    public ProfileConfig createProfile(ProfileConfig profile, int timeoutMs) {
        return new ProfileConfig(httpPost("/api/image/profiles",
                require(profile, "profile").toJsonString(), timeoutMs));
    }

    public ProfileConfig updateProfileById(int profileId, ProfileConfig profile,
                                           int timeoutMs) {
        return new ProfileConfig(httpPut("/api/image/profiles/" + profileId,
                require(profile, "profile").toJsonString(), timeoutMs));
    }

    public ProfileConfig updateProfileByName(String name, ProfileConfig profile,
                                             int timeoutMs) {
        ProfileConfig found = getProfileByName(name, timeoutMs);
        Long id = found.getId();
        if (id == null) {
            throw new ItscamInvalidParameterException("profile has no id: \"" + name + "\"");
        }
        return updateProfileById(id.intValue(), profile, timeoutMs);
    }

    public ProfileConfig updateProfiles(List<ProfileConfig> profiles, int timeoutMs) {
        return new ProfileConfig(httpPut("/api/image/profiles",
                RestObject.listToJson(profiles), timeoutMs));
    }

    public String deleteProfile(int profileId, int timeoutMs) {
        return httpDelete("/api/image/profiles?id=" + profileId, timeoutMs);
    }

    public MiscVolatile getVolatileInfo(int timeoutMs) {
        return new MiscVolatile(getVolatileInfoJson(timeoutMs));
    }

    public Misc getMisc(int timeoutMs) {
        return new Misc(httpGet("/api/equipment/misc", timeoutMs));
    }

    public Misc setMisc(Misc config, int timeoutMs) {
        return new Misc(httpPut("/api/equipment/misc",
                require(config, "config").toJsonString(), timeoutMs));
    }

    public Misc getGeneralConfig(int timeoutMs) { return getMisc(timeoutMs); }
    public Misc setGeneralConfig(Misc config, int timeoutMs) { return setMisc(config, timeoutMs); }

    public OcrConfig getOcrConfig(int timeoutMs) { return new OcrConfig(getOcrConfigJson(timeoutMs)); }
    public OcrConfig setOcrConfig(OcrConfig config, int timeoutMs) {
        return new OcrConfig(httpPut("/api/equipment/ocr", require(config, "config").toJsonString(), timeoutMs));
    }

    public AnalyticsConfig getAnalyticsConfig(int timeoutMs) { return new AnalyticsConfig(getAnalyticsConfigJson(timeoutMs)); }
    public AnalyticsConfig setAnalyticsConfig(AnalyticsConfig config, int timeoutMs) {
        return new AnalyticsConfig(httpPut("/api/equipment/analytics", require(config, "config").toJsonString(), timeoutMs));
    }

    public ClassifierConfig getClassifierConfig(int timeoutMs) { return new ClassifierConfig(getClassifierConfigJson(timeoutMs)); }
    public ClassifierConfig setClassifierConfig(ClassifierConfig config, int timeoutMs) {
        return new ClassifierConfig(httpPut("/api/equipment/classifier", require(config, "config").toJsonString(), timeoutMs));
    }

    public AutoFocus getAutoFocus(int timeoutMs) { return new AutoFocus(httpGet("/api/equipment/autofocus", timeoutMs)); }
    public AutoFocus setAutoFocus(AutoFocus config, int timeoutMs) {
        return new AutoFocus(httpPut("/api/equipment/autofocus", require(config, "config").toJsonString(), timeoutMs));
    }

    public StreamConfig getStreamConfig(int timeoutMs) { return new StreamConfig(httpGet("/api/video/streams", timeoutMs)); }
    public StreamConfig setStreamConfig(StreamConfig config, int timeoutMs) {
        return new StreamConfig(httpPut("/api/video/streams", require(config, "config").toJsonString(), timeoutMs));
    }

    public LanesConfig getLanesConfig(int timeoutMs) { return new LanesConfig(getLanesConfigJson(timeoutMs)); }
    public LanesConfig setLanesConfig(LanesConfig config, int timeoutMs) {
        return new LanesConfig(httpPut("/api/equipment/lanes", require(config, "config").toJsonString(), timeoutMs));
    }

    public ItscamproConfig getItscamproConfig(int timeoutMs) { return new ItscamproConfig(getItscamproConfigJson(timeoutMs)); }
    public ItscamproConfig setItscamproConfig(ItscamproConfig config, int timeoutMs) {
        return new ItscamproConfig(httpPut("/api/equipment/servers/itscampro", require(config, "config").toJsonString(), timeoutMs));
    }

    public ItscamproStatus getItscamproStatus(int timeoutMs) { return new ItscamproStatus(getItscamproStatusJson(timeoutMs)); }
    public ImageSignConfig getImageSignConfig(int timeoutMs) { return new ImageSignConfig(httpGet("/api/equipment/imageSign", timeoutMs)); }

    public FtpConfig getFtpConfig(int timeoutMs) { return new FtpConfig(httpGet("/api/equipment/servers/ftp", timeoutMs)); }
    public FtpConfig setFtpConfig(FtpConfig config, int timeoutMs) {
        return new FtpConfig(httpPut("/api/equipment/servers/ftp", require(config, "config").toJsonString(), timeoutMs));
    }

    public LinceConfig getLinceConfig(int timeoutMs) { return new LinceConfig(httpGet("/api/equipment/servers/lince", timeoutMs)); }
    public LinceConfig setLinceConfig(LinceConfig config, int timeoutMs) {
        return new LinceConfig(httpPut("/api/equipment/servers/lince", require(config, "config").toJsonString(), timeoutMs));
    }
    public LinceStatus getLinceStatus(int timeoutMs) { return new LinceStatus(httpGet("/api/equipment/servers/lince/status", timeoutMs)); }

    public VehicleIndicatorConfig getVehicleIndicatorConfig(int timeoutMs) { return new VehicleIndicatorConfig(httpGet("/api/equipment/vehicleIndicator", timeoutMs)); }
    public VehicleIndicatorConfig setVehicleIndicatorConfig(VehicleIndicatorConfig config, int timeoutMs) {
        return new VehicleIndicatorConfig(httpPut("/api/equipment/vehicleIndicator", require(config, "config").toJsonString(), timeoutMs));
    }

    public ProtocolsConfig getProtocolsConfig(int timeoutMs) { return new ProtocolsConfig(httpGet("/api/equipment/servers/protocols", timeoutMs)); }
    public ProtocolsConfig setProtocolsConfig(ProtocolsConfig config, int timeoutMs) {
        return new ProtocolsConfig(httpPut("/api/equipment/servers/protocols", require(config, "config").toJsonString(), timeoutMs));
    }

    public ProfileTransitioner getProfileTransitioner(int timeoutMs) { return new ProfileTransitioner(httpGet("/api/equipment/transitioner", timeoutMs)); }
    public ProfileTransitioner setProfileTransitioner(ProfileTransitioner config, int timeoutMs) {
        return new ProfileTransitioner(httpPut("/api/equipment/transitioner", require(config, "config").toJsonString(), timeoutMs));
    }

    public List<IoConfig> getIoPorts(int timeoutMs) { return IoConfig.listFromJson(httpGet("/api/equipment/ioPorts", timeoutMs)); }
    public List<IoConfig> setIoPorts(List<IoConfig> ports, int timeoutMs) {
        return IoConfig.listFromJson(httpPut("/api/equipment/ioPorts", RestObject.listToJson(ports), timeoutMs));
    }
    public IoConfig getIoPort(int portId, int timeoutMs) { return new IoConfig(httpGet("/api/equipment/ioPorts/" + portId, timeoutMs)); }
    public IoConfig setIoPort(int portId, IoConfig port, int timeoutMs) {
        return new IoConfig(httpPut("/api/equipment/ioPorts/" + portId, require(port, "port").toJsonString(), timeoutMs));
    }

    public List<IoBasic> getIoBasic(int timeoutMs) { return IoBasic.listFromJson(httpGet("/api/equipment/ioBasic", timeoutMs)); }
    public List<IoBasic> setIoBasic(List<IoBasic> ports, int timeoutMs) {
        return IoBasic.listFromJson(httpPut("/api/equipment/ioBasic", RestObject.listToJson(ports), timeoutMs));
    }

    public RestApiClientConfig getRestApiClientConfig(int serverId, int timeoutMs) {
        return new RestApiClientConfig(httpGet("/api/equipment/servers/restapiclient/" + serverId + "/config", timeoutMs));
    }
    public RestApiClientConfig setRestApiClientConfig(int serverId, RestApiClientConfig config, int timeoutMs) {
        return new RestApiClientConfig(httpPut("/api/equipment/servers/restapiclient/" + serverId + "/config", require(config, "config").toJsonString(), timeoutMs));
    }
    public RestApiClientStatus getRestApiClientStatus(int serverId, int timeoutMs) {
        return new RestApiClientStatus(httpGet("/api/equipment/servers/restapiclient/" + serverId + "/status", timeoutMs));
    }

    public Licenses getLicenses(int timeoutMs) { return new Licenses(httpGet("/api/system/licenses", timeoutMs)); }

    private <T extends RestObject> T require(T value, String name) {
        if (value == null) throw new IllegalArgumentException(name + " is null");
        return value;
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