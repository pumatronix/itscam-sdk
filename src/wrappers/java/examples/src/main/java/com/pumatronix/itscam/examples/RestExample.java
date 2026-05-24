/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * REST API example.  Mirrors rest_example.py / rest_example.go.
 *
 * Usage:
 *   java -cp ... com.pumatronix.itscam.examples.RestExample \
 *       <host> <user> <password> [--https] [--insecure]
 */
package com.pumatronix.itscam.examples;

import com.pumatronix.itscam.ItscamRestClient;

import java.util.Arrays;
import java.util.List;

public final class RestExample {

    public static void main(String[] args) {
        if (args.length < 3) {
            System.err.println("Usage: RestExample <host> <user> <pass> [--https] [--insecure]");
            System.exit(1);
        }

        String host = args[0];
        String user = args[1];
        String pass = args[2];
        List<String> flags = Arrays.asList(args).subList(3, args.length);
        boolean useHttps = flags.contains("--https");
        boolean insecure = flags.contains("--insecure");

        String scheme = useHttps ? "https" : "http";
        int    port   = useHttps ? 443 : 80;

        try (ItscamRestClient rest = new ItscamRestClient()) {
            rest.setBaseUrl(host, port, scheme);
            if (useHttps && insecure) {
                rest.setVerifyServerCertificate(false);
            }

            String loginResp = rest.login(user, pass, 10000);
            System.out.println("login OK: " + truncate(loginResp));

            String volatileInfo = rest.httpGet(
                    "/api/equipment/misc/readonly/volatile", 10000);
            System.out.println();
            System.out.println("volatile info: " + truncate(volatileInfo));

            String profiles = rest.httpGet("/api/image/profiles", 10000);
            System.out.println();
            System.out.println("profiles: " + truncate(profiles));
        }
    }

    private static String truncate(String s) {
        if (s == null) return "";
        return s.length() > 1024 ? s.substring(0, 1024) + " ...(truncated)" : s;
    }

    private RestExample() {}
}
