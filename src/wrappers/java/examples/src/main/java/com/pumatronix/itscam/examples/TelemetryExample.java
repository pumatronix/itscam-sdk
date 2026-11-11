/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * System telemetry example.  Reads the state of the whole camera once, then
 * follows the documents the camera pushes for a while.
 *
 * Usage:
 *   java -cp ... com.pumatronix.itscam.examples.TelemetryExample \
 *       <host> <user> <password> [--seconds N] [--https] [--insecure]
 */
package com.pumatronix.itscam.examples;

import com.pumatronix.itscam.ItscamConsumer;
import com.pumatronix.itscam.ItscamRestClient;
import com.pumatronix.itscam.TelemetryUpdate;
import com.pumatronix.itscam.resttypes.SystemTelemetry;

import java.util.Arrays;
import java.util.List;

public final class TelemetryExample {

    public static void main(String[] args) throws InterruptedException {
        if (args.length < 3) {
            System.err.println("Usage: TelemetryExample <host> <user> <pass> "
                    + "[--seconds N] [--https] [--insecure]");
            System.exit(1);
        }

        List<String> flags = Arrays.asList(args).subList(3, args.length);
        boolean useHttps = flags.contains("--https");
        boolean insecure = flags.contains("--insecure");
        int secondsAt = flags.indexOf("--seconds");
        int seconds = secondsAt >= 0
                ? Integer.parseInt(flags.get(secondsAt + 1)) : 30;

        try (ItscamRestClient rest = new ItscamRestClient()) {
            rest.setBaseUrl(args[0], useHttps ? 443 : 80,
                    useHttps ? "https" : "http");
            if (useHttps && insecure) {
                rest.setVerifyServerCertificate(false);
            }
            rest.login(args[1], args[2], 10000);

            // Polling: one read of the whole camera
            printHealth(rest.getSystemTelemetry(10000));

            // Stream: one document at once, then one every 5 s.  The
            // callback runs on the SDK's callback pool: do not block in it.
            rest.startTelemetryStream(new ItscamConsumer<TelemetryUpdate>() {
                @Override
                public void accept(TelemetryUpdate update) {
                    System.out.printf("%nupdate #%d%n", update.sequence());
                    printHealth(update.telemetry());
                }
            }, 0);
            Thread.sleep(seconds * 1000L);
            rest.stopTelemetryStream();

            TelemetryUpdate latest = rest.latestTelemetry();
            if (latest != null) {
                System.out.printf("%nlast document: #%d%n", latest.sequence());
            }
        }
    }

    private static void printHealth(SystemTelemetry telemetry) {
        System.out.println("health: " + telemetry.getHealthStatus());
        System.out.println("  " + telemetry.get("health"));
    }

    private TelemetryExample() {}
}
