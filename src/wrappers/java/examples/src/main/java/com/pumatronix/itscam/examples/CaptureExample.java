/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Binary-client capture example.  Mirrors capture_example.py /
 * capture_example.go / BinaryCaptureExample/Program.cs.
 *
 * Usage:
 *   java -cp itscam-sdk-examples.jar:itscam-sdk.jar:jna.jar \
 *        com.pumatronix.itscam.examples.CaptureExample <camera_ip> [password]
 */
package com.pumatronix.itscam.examples;

import com.pumatronix.itscam.AutoReconnectConfig;
import com.pumatronix.itscam.CaptureResult;
import com.pumatronix.itscam.ItscamClient;
import com.pumatronix.itscam.Sdk;

import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public final class CaptureExample {

    public static void main(String[] args) throws InterruptedException {
        if (args.length < 1) {
            System.err.println("Usage: CaptureExample <camera_ip> [password]");
            System.exit(1);
        }
        String host = args[0];
        String password = args.length > 1 ? args[1] : null;

        System.out.printf("ITSCAM SDK %s%n", Sdk.getNativeLibraryVersion());

        AtomicInteger snapshotCount = new AtomicInteger();

        try (ItscamClient camera = new ItscamClient()) {
            System.out.printf("Connecting to %s:60000 ...%n", host);
            camera.connect(host, 60000, 10000,
                    new AutoReconnectConfig().setEnabled(true));

            if (password != null && !password.isEmpty()) {
                camera.authenticate(password, 10000);
                System.out.println("Authenticated.");
            }

            camera.onSnapshotImage(result -> {
                int n = snapshotCount.incrementAndGet();
                System.out.printf("  snapshot callback #%d: %d bytes%n",
                        n, result.jpeg().length);
            });

            camera.subscribeCaptures(10000);

            List<CaptureResult> frames = camera.captureSnapshot(15000);
            System.out.printf("captureSnapshot returned %d frame(s)%n",
                    frames.size());
            for (int i = 0; i < frames.size(); i++) {
                CaptureResult r = frames.get(i);
                System.out.printf("  frame %d: %d bytes, plates=%s%n",
                        i + 1, r.jpeg().length, r.plates());
            }

            System.out.println("Waiting briefly for trigger frames ...");
            Thread.sleep(2000);
        }

        System.out.printf("Done (%d snapshot callback(s)).%n",
                snapshotCount.get());
    }

    private CaptureExample() {}
}
