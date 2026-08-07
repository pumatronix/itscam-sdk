/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Apply the virtual loop preset configuration using the Java SDK typed REST
 * client.
 *
 * Usage:
 *   java -cp ... com.pumatronix.itscam.examples.ConfigVirtualLoopPreset \
 *       [base-url] [user] [password] [--insecure] [--timeout-ms N]
 */
package com.pumatronix.itscam.examples;

import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import com.pumatronix.itscam.ItscamRestClient;
import com.pumatronix.itscam.resttypes.AnalyticsConfig;
import com.pumatronix.itscam.resttypes.ClassifierConfig;
import com.pumatronix.itscam.resttypes.Misc;
import com.pumatronix.itscam.resttypes.OcrConfig;
import com.pumatronix.itscam.resttypes.ProfileConfig;
import com.pumatronix.itscam.resttypes.ProfileTransitioner;
import com.pumatronix.itscam.resttypes.StreamConfig;

import java.net.URI;
import java.util.Collections;

public final class ConfigVirtualLoopPreset {
    private static final String DEFAULT_BASE_URL = "http://192.168.254.254";
    private static final String DEFAULT_USER = "admin";
    private static final String DEFAULT_PASSWORD = "1234";
    private static final int DEFAULT_TIMEOUT_MS = 10000;

    public static void main(String[] args) {
        Args parsed = Args.parse(args);
        if (parsed.help) {
            usage();
            return;
        }

        try (ItscamRestClient rest = openRest(parsed.baseUrl, parsed.user,
                parsed.password, parsed.timeoutMs, parsed.insecure)) {
            applyConfig(rest, parsed.timeoutMs);
        }
    }

    private static void applyConfig(ItscamRestClient rest, int timeoutMs) {
        rest.updateProfiles(Collections.singletonList(dayProfile()), timeoutMs);
        System.out.println("applied profiles with updateProfiles");

        rest.setProfileTransitioner(transitioner(), timeoutMs);
        System.out.println("applied transitioner with setProfileTransitioner");

        rest.setMisc(misc(), timeoutMs);
        System.out.println("applied misc_image with setMisc");

        rest.setStreamConfig(streams(), timeoutMs);
        System.out.println("applied streams_mjpeg with setStreamConfig");

        rest.setOcrConfig(ocr(), timeoutMs);
        System.out.println("applied ocr with setOcrConfig");

        rest.setAnalyticsConfig(analytics(), timeoutMs);
        System.out.println("applied analytics with setAnalyticsConfig");

        rest.setClassifierConfig(classifier(), timeoutMs);
        System.out.println("applied classifier with setClassifierConfig");
    }

    private static ProfileConfig dayProfile() {
        ProfileConfig profile = new ProfileConfig();
        profile.setId(Long.valueOf(0L));
        profile.setName("Diurno");
        profile.setBoolean("active", Boolean.TRUE);
        profile.set("color", object(
                pair("blacklevel", number(Integer.valueOf(0))),
                pair("brightness", number(Integer.valueOf(0))),
                pair("contrast", number(Integer.valueOf(0))),
                pair("gamma", number(Integer.valueOf(12))),
                pair("saturation", number(Integer.valueOf(50)))));
        profile.set("exposure", object(
                pair("gain", object(
                        pair("automatic", bool(true)),
                        pair("fixedValue", number(Integer.valueOf(1))),
                        pair("maxValue", number(Integer.valueOf(2400))),
                        pair("minValue", number(Integer.valueOf(0))))),
                pair("highGainMode", bool(true)),
                pair("iris", object(
                        pair("automatic", bool(false)),
                        pair("fixedValue", number(Integer.valueOf(1000))))),
                pair("level", object(
                        pair("roi", object(pair("enabled", bool(false)))),
                        pair("targetValue", number(Double.valueOf(40.0))))),
                pair("shutter", object(
                        pair("automatic", bool(true)),
                        pair("fixedValue", number(Integer.valueOf(99))),
                        pair("maxValue", number(Integer.valueOf(30000))),
                        pair("minValue", number(Integer.valueOf(24)))))));
        profile.set("filter", object(
                pair("sharpnessLevel", number(Integer.valueOf(10))),
                pair("timedomainLevel", number(Integer.valueOf(10)))));
        profile.set("lens", object(pair("exchanger", bool(true))));
        profile.set("movFilter", object(pair("enabled", bool(false))));
        profile.set("multipleExposures", object(
                pair("enabled", bool(true)),
                pair("settings", array(object(
                        pair("flash", object(pair("power", array()))),
                        pair("gain", object(
                                pair("percentageOfCurrent", bool(true)),
                                pair("value", number(Double.valueOf(100.0))))),
                        pair("shutter", object(
                                pair("percentageOfCurrent", bool(true)),
                                pair("value", number(Double.valueOf(100.0))))))))));
        profile.set("scenario", object(
                pair("enable", bool(false)),
                pair("selected", number(Integer.valueOf(1)))));
        profile.set("transitions", object(
                pair("lower", object(
                        pair("endTime", string("00:00:00")),
                        pair("holdTime", number(Integer.valueOf(60000))),
                        pair("level", number(Double.valueOf(10.0))),
                        pair("profile", number(Integer.valueOf(34409))),
                        pair("startTime", string("00:00:00")))),
                pair("upper", object(
                        pair("endTime", string("00:00:00")),
                        pair("holdTime", number(Integer.valueOf(60000))),
                        pair("level", number(Double.valueOf(30.0))),
                        pair("profile", number(Integer.valueOf(0))),
                        pair("startTime", string("00:00:00"))))));
        profile.set("trigger", object(pair("enabled", bool(false))));
        return profile;
    }

    private static ProfileTransitioner transitioner() {
        ProfileTransitioner transitioner = new ProfileTransitioner();
        transitioner.setBoolean("automatic", Boolean.FALSE);
        return transitioner;
    }

    private static Misc misc() {
        Misc misc = new Misc();
        misc.setBoolean("cameraOrientation", Boolean.FALSE);
        misc.setInteger("jpegQuality", Integer.valueOf(70));
        return misc;
    }

    private static StreamConfig streams() {
        StreamConfig streams = new StreamConfig();
        streams.set("mjpeg", object(
                pair("main", object(
                        pair("enabled", bool(true)),
                        pair("framerate", number(Integer.valueOf(10))),
                        pair("quality", number(Integer.valueOf(70))),
                        pair("resolution", string("800x600")),
                        pair("useTriggerFrames", bool(false))))));
        return streams;
    }

    private static OcrConfig ocr() {
        OcrConfig ocr = new OcrConfig();
        ocr.set("ocr", object(pair("enabled", bool(false))));
        return ocr;
    }

    private static AnalyticsConfig analytics() {
        AnalyticsConfig analytics = new AnalyticsConfig();
        analytics.set("voting", object(pair("enabled", bool(false))));
        return analytics;
    }

    private static ClassifierConfig classifier() {
        ClassifierConfig classifier = new ClassifierConfig();
        classifier.set("classifier", object(pair("enabled", bool(false))));
        return classifier;
    }

    private static ItscamRestClient openRest(String baseUrl, String user,
                                             String password, int timeoutMs,
                                             boolean insecure) {
        Endpoint endpoint = Endpoint.parse(baseUrl);
        ItscamRestClient rest = new ItscamRestClient();
        boolean success = false;
        try {
            rest.setBaseUrl(endpoint.host, endpoint.port, endpoint.scheme);
            if ("https".equals(endpoint.scheme) && insecure) {
                rest.setVerifyServerCertificate(false);
            }
            rest.login(user, password, timeoutMs);
            success = true;
            return rest;
        } finally {
            if (!success) {
                rest.close();
            }
        }
    }

    private static JsonObject object(JsonPair... pairs) {
        JsonObject out = new JsonObject();
        for (int i = 0; i < pairs.length; i++) {
            JsonPair pair = pairs[i];
            out.add(pair.name, pair.value == null ? JsonNull.INSTANCE : pair.value);
        }
        return out;
    }

    private static JsonArray array(JsonElement... values) {
        JsonArray out = new JsonArray();
        for (int i = 0; i < values.length; i++) {
            out.add(values[i] == null ? JsonNull.INSTANCE : values[i]);
        }
        return out;
    }

    private static JsonPair pair(String name, JsonElement value) {
        return new JsonPair(name, value);
    }

    private static JsonElement bool(boolean value) {
        return new JsonPrimitive(Boolean.valueOf(value));
    }

    private static JsonElement number(Number value) {
        return value == null ? JsonNull.INSTANCE : new JsonPrimitive(value);
    }

    private static JsonElement string(String value) {
        return value == null ? JsonNull.INSTANCE : new JsonPrimitive(value);
    }

    private static void usage() {
        System.out.println("Usage: ConfigVirtualLoopPreset [base-url] [user] [password] "
                + "[--insecure] [--timeout-ms N]");
        System.out.println("  base-url       target camera URL (default: "
                + DEFAULT_BASE_URL + ")");
        System.out.println("  user           REST user (default: " + DEFAULT_USER + ")");
        System.out.println("  password       REST password (default: "
                + DEFAULT_PASSWORD + ")");
    }

    private static final class JsonPair {
        final String name;
        final JsonElement value;

        JsonPair(String name, JsonElement value) {
            this.name = name;
            this.value = value;
        }
    }

    private static final class Args {
        String baseUrl = DEFAULT_BASE_URL;
        String user = DEFAULT_USER;
        String password = DEFAULT_PASSWORD;
        int timeoutMs = DEFAULT_TIMEOUT_MS;
        boolean insecure;
        boolean help;

        static Args parse(String[] args) {
            Args parsed = new Args();
            int positional = 0;
            for (int i = 0; i < args.length; i++) {
                String arg = args[i];
                if ("--help".equals(arg) || "-h".equals(arg)) {
                    parsed.help = true;
                } else if ("--insecure".equals(arg)) {
                    parsed.insecure = true;
                } else if ("--timeout-ms".equals(arg)) {
                    parsed.timeoutMs = Integer.parseInt(requireValue(args, ++i, arg));
                } else if (arg.startsWith("--")) {
                    throw new IllegalArgumentException("unknown argument: " + arg);
                } else if (positional == 0) {
                    parsed.baseUrl = arg;
                    positional++;
                } else if (positional == 1) {
                    parsed.user = arg;
                    positional++;
                } else if (positional == 2) {
                    parsed.password = arg;
                    positional++;
                } else {
                    throw new IllegalArgumentException("too many positional arguments");
                }
            }
            return parsed;
        }

        private static String requireValue(String[] args, int index, String flag) {
            if (index >= args.length) {
                throw new IllegalArgumentException(flag + " requires a value");
            }
            return args[index];
        }
    }

    private static final class Endpoint {
        final String scheme;
        final String host;
        final int port;

        Endpoint(String scheme, String host, int port) {
            this.scheme = scheme;
            this.host = host;
            this.port = port;
        }

        static Endpoint parse(String raw) {
            String value = raw == null ? "" : raw.trim();
            if (value.length() == 0) {
                throw new IllegalArgumentException("empty URL");
            }
            if (value.indexOf("://") < 0) {
                value = "http://" + value;
            }
            URI uri = URI.create(value);
            String scheme = uri.getScheme() == null ? "http" : uri.getScheme().toLowerCase();
            String host = uri.getHost();
            if (host == null || host.length() == 0) {
                throw new IllegalArgumentException("URL has no host: " + raw);
            }
            int port = uri.getPort();
            if (port < 0) {
                port = "https".equals(scheme) ? 443 : 80;
            }
            return new Endpoint(scheme, host, port);
        }
    }

    private ConfigVirtualLoopPreset() {}
}