/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * REST API example.  Mirrors rest_example.py / rest_example.go.
 *
 * Usage:
 *   java -cp ... com.pumatronix.itscam.examples.RestExample \
 *       <host> <user> <password> [--https] [--insecure] \
 *       [--profile-id N] [--zoom N] [--focus N] [--autofocus]
 */
package com.pumatronix.itscam.examples;

import com.pumatronix.itscam.ItscamRestClient;
import com.pumatronix.itscam.resttypes.AutoFocus;
import com.pumatronix.itscam.resttypes.LensConfig;
import com.pumatronix.itscam.resttypes.MiscVolatile;
import com.pumatronix.itscam.resttypes.ProfileConfig;

import java.util.Arrays;
import java.util.List;

public final class RestExample {

    public static void main(String[] args) {
        if (args.length < 3) {
            System.err.println("Usage: RestExample <host> <user> <pass> "
                    + "[--https] [--insecure] [--profile-id N] "
                    + "[--zoom N] [--focus N] [--autofocus]");
            System.exit(1);
        }

        String host = args[0];
        String user = args[1];
        String pass = args[2];
        List<String> flags = Arrays.asList(args).subList(3, args.length);
        boolean useHttps = flags.contains("--https");
        boolean insecure = flags.contains("--insecure");
        int profileId = intFlag(args, "--profile-id", 0);
        Integer zoom = optionalIntFlag(args, "--zoom");
        Integer focus = optionalIntFlag(args, "--focus");
        boolean runAutofocus = flags.contains("--autofocus");
        if ((zoom != null || focus != null) && !flags.contains("--profile-id")) {
            System.err.println("--profile-id is required with --zoom or --focus");
            System.exit(1);
        }

        String scheme = useHttps ? "https" : "http";
        int    port   = useHttps ? 443 : 80;

        try (ItscamRestClient rest = new ItscamRestClient()) {
            rest.setBaseUrl(host, port, scheme);
            if (useHttps && insecure) {
                rest.setVerifyServerCertificate(false);
            }

            String loginResp = rest.login(user, pass, 10000);
            System.out.println("login OK: " + truncate(loginResp));

            MiscVolatile volatileInfo = rest.getVolatileInfo(10000);
            System.out.println();
            printLens("volatile lens", volatileInfo.getLens());

            List<ProfileConfig> profiles = rest.getProfiles(10000);
            System.out.println();
            System.out.printf("profiles: %d profile(s)%n", profiles.size());

            if (zoom != null || focus != null) {
                LensConfig lens = new LensConfig();
                if (zoom != null) lens.setZoom(zoom);
                if (focus != null) lens.setFocus(focus);

                ProfileConfig patch = new ProfileConfig().setLens(lens);
                ProfileConfig updated = rest.updateProfileById(profileId,
                        patch, 10000);

                System.out.println();
                System.out.printf("updated profile %d lens: %s%n",
                        profileId, truncate(updated.toJsonString()));
            }

            AutoFocus current = rest.getAutoFocus(10000);
            System.out.println();
            System.out.println("autofocus: " + truncate(current.toJsonString()));

            if (runAutofocus) {
                AutoFocus request = new AutoFocus().setRun(Boolean.TRUE);
                AutoFocus updated = rest.setAutoFocus(request, 10000);
                System.out.println("autofocus run requested: "
                        + truncate(updated.toJsonString()));
            }
        }
    }

    private static Integer optionalIntFlag(String[] args, String name) {
        for (int i = 0; i < args.length - 1; i++) {
            if (name.equals(args[i])) {
                return Integer.valueOf(args[i + 1]);
            }
        }
        return null;
    }

    private static int intFlag(String[] args, String name, int defaultValue) {
        Integer value = optionalIntFlag(args, name);
        return value == null ? defaultValue : value.intValue();
    }

    private static void printLens(String label, LensConfig lens) {
        if (lens == null) {
            System.out.println(label + ": unavailable");
            return;
        }
        System.out.printf("%s: zoom=%s focus=%s%n",
                label, text(lens.getZoom()), text(lens.getFocus()));
    }

    private static String text(Object value) {
        return value == null ? "?" : String.valueOf(value);
    }

    private static String truncate(String s) {
        if (s == null) return "";
        return s.length() > 1024 ? s.substring(0, 1024) + " ...(truncated)" : s;
    }

    private RestExample() {}
}