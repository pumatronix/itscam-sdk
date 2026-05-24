/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * CGI snapshot example.  Mirrors cgi_snapshot_example.py /
 * cgi_snapshot_example.go.
 *
 * CGI auth is opt-in -- pass --user and --password only when the
 * camera has configCgi.blockAPI=true.
 *
 * Usage:
 *   java -cp ... com.pumatronix.itscam.examples.CgiSnapshotExample \
 *       <host> [--https] [--insecure] [--user U --password P]
 */
package com.pumatronix.itscam.examples;

import com.pumatronix.itscam.CgiImage;
import com.pumatronix.itscam.ItscamCgiClient;
import com.pumatronix.itscam.SnapshotCgiRequest;

import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public final class CgiSnapshotExample {

    public static void main(String[] args) throws InterruptedException, IOException {
        if (args.length < 1) {
            System.err.println(
                    "Usage: CgiSnapshotExample <host> "
                  + "[--https] [--insecure] [--user U --password P]");
            System.exit(1);
        }
        String host = args[0];
        boolean useHttps = false;
        boolean insecure = false;
        String user = null;
        String pass = null;
        for (int i = 1; i < args.length; i++) {
            switch (args[i]) {
                case "--https":   useHttps = true; break;
                case "--insecure": insecure = true; break;
                case "--user":     user = args[++i]; break;
                case "--password": pass = args[++i]; break;
                default:
                    System.err.println("Unknown flag: " + args[i]);
                    System.exit(1);
            }
        }

        String scheme = useHttps ? "https" : "http";
        int    port   = useHttps ? 443 : 80;

        try (ItscamCgiClient cgi = new ItscamCgiClient()) {
            cgi.setBaseUrl(host, port, scheme);
            if (useHttps && insecure) {
                cgi.setVerifyServerCertificate(false);
            }
            if (user != null && pass != null) {
                cgi.login(user, pass, 10000);
                System.out.println("Logged in as " + user);
            } else {
                System.out.println(
                        "No credentials supplied; talking to the camera "
                      + "without CGI authentication.");
            }

            System.out.println("Fetching lastframe.cgi ...");
            CgiImage last = cgi.getLastFrame(10000);
            last.save(Paths.get("lastframe.jpg"));
            System.out.printf("  -> lastframe.jpg (%s, %d bytes)%n",
                    last.mimeType(), last.data().length);

            System.out.println("Triggering snapshot.cgi (Q=80, mosaic off) ...");
            List<CgiImage> images = cgi.getSnapshot(
                    new SnapshotCgiRequest().setQuality(80), 15000);
            System.out.printf("  -> received %d image(s)%n", images.size());
            for (int i = 0; i < images.size(); i++) {
                CgiImage img = images.get(i);
                Path out = Paths.get("snapshot-" + i + ".jpg");
                img.save(out);
                System.out.printf("     %s: %s, %d bytes%n",
                        out, img.mimeType(), img.data().length);
            }

            System.out.println("Streaming MJPEG for 5 seconds ...");
            AtomicInteger frameCount = new AtomicInteger();
            cgi.startMjpegStream(frame -> {
                int n = frameCount.incrementAndGet();
                if (n == 1) {
                    try {
                        java.nio.file.Files.write(
                                Paths.get("mjpeg-first.jpg"),
                                frame.data());
                    } catch (IOException ignored) {}
                }
            }, 10000);
            Thread.sleep(5000);
            cgi.stopMjpegStream();
            System.out.printf("  -> received %d MJPEG frame(s)%n",
                    frameCount.get());
        }
    }

    private CgiSnapshotExample() {}
}
