/*
 *  Smoke.java -- Java wrapper smoke test, against tests/standin/server.js.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Same checks as the other languages (tests/README.md).  Java 7 source, as
 *  the wrapper.  Run it on a JVM whose default charset isn't UTF-8 too
 *  (-Dfile.encoding=US-ASCII): text from the camera must still decode.
 *  Environment: ITSCAM_TEST_HOST (127.0.0.1), ITSCAM_TEST_PORT (18080);
 *  -Ditscam.sdk.library=<path to libitscam_sdk.so>.
 */

import com.pumatronix.itscam.CgiImage;
import com.pumatronix.itscam.ErrorCode;
import com.pumatronix.itscam.ItscamCgiClient;
import com.pumatronix.itscam.ItscamClient;
import com.pumatronix.itscam.ItscamConsumer;
import com.pumatronix.itscam.ItscamException;
import com.pumatronix.itscam.ItscamInvalidParameterException;
import com.pumatronix.itscam.ItscamRestClient;
import com.pumatronix.itscam.Sdk;
import com.pumatronix.itscam.TelemetryUpdate;
import com.pumatronix.itscam.jpeg.JpegMetadata;
import com.pumatronix.itscam.jpeg.PlateRecognition;
import com.pumatronix.itscam.resttypes.ProfileConfig;
import com.pumatronix.itscam.resttypes.SystemTelemetry;

import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public final class Smoke {
    private static int failures;

    private static void check(boolean ok, String name, String detail) {
        System.out.println((ok ? "PASS " : "FAIL ") + name
                + (detail == null || detail.isEmpty() ? "" : ": " + detail));
        if (!ok) failures++;
    }

    private static String env(String name, String fallback) {
        String v = System.getenv(name);
        return v == null || v.isEmpty() ? fallback : v;
    }

    public static void main(String[] args) throws InterruptedException {
        String host = env("ITSCAM_TEST_HOST", "127.0.0.1");
        int port = Integer.parseInt(env("ITSCAM_TEST_PORT", "18080"));
        System.out.println("Java " + System.getProperty("java.version") + " smoke ("
                + System.getProperty("file.encoding") + ") against " + host + ":" + port);

        // ITSCAM_TEST_EXPECT_VERSION: the library under test, so that a stale
        // copy found first fails instead of passing
        String version = Sdk.getNativeLibraryVersion();
        String expected = env("ITSCAM_TEST_EXPECT_VERSION", "");
        boolean current = expected.isEmpty() || expected.equals(version);
        check(version != null && !version.isEmpty() && current, "version",
                current ? version : version + ", expected " + expected);

        ItscamRestClient rest = new ItscamRestClient();
        try {
            rest.setBaseUrl(host, port, "http");
            try {
                rest.login("admin", "1234", 10000);
                check(true, "rest login", "");
            } catch (ItscamException e) {
                check(false, "rest login", e.getMessage());
            }

            try {
                List<ProfileConfig> profiles = rest.getProfiles(10000);
                boolean ok = profiles.size() == 1
                        && Long.valueOf(3).equals(profiles.get(0).getId())
                        && "Câmera São João".equals(profiles.get(0).getName());
                // Escaped, so the report itself doesn't depend on the charset
                check(ok, "rest typed get", profiles.isEmpty() ? "no profile"
                        : "id " + profiles.get(0).getId() + ", name " + escape(profiles.get(0).getName()));
            } catch (ItscamException e) {
                check(false, "rest typed get", e.getMessage());
            }

            try {
                rest.httpGet("/api/does-not-exist", 10000);
                check(false, "rest 404 maps to ItscamInvalidParameterException", "succeeded");
            } catch (ItscamException e) {
                check(e instanceof ItscamInvalidParameterException
                                && e.getCode() == ErrorCode.INVALID_PARAMETER,
                        "rest 404 maps to ItscamInvalidParameterException",
                        e.getClass().getSimpleName() + ": " + e.getMessage());
            }
        } finally {
            rest.close();
        }

        ItscamCgiClient cgi = new ItscamCgiClient();
        try {
            cgi.setBaseUrl(host, port, "http");
            try {
                List<CgiImage> images = cgi.getSnapshot(15000);
                List<PlateRecognition> plates = JpegMetadata.extractPlateRecognitions(
                        JpegMetadata.parseTags(JpegMetadata.extractComment(images.get(0).data())));
                check(images.size() == 1 && plates.size() == 1 && "ABC1D23".equals(plates.get(0).plate()),
                        "cgi snapshot + jpeg plate", plates.isEmpty() ? "no plate" : plates.get(0).plate());
            } catch (ItscamException e) {
                check(false, "cgi snapshot + jpeg plate", e.getMessage());
            }
        } finally {
            cgi.close();
        }

        ItscamRestClient telemetryRest = new ItscamRestClient();
        try {
            telemetryRest.setBaseUrl(host, port, "http");
            telemetryRest.login("admin", "1234", 10000);
            try {
                SystemTelemetry telemetry = telemetryRest.getSystemTelemetry(10000);
                check("warning".equals(telemetry.getHealthStatus())
                                && String.valueOf(telemetry.get("health")).contains("storageFull"),
                        "telemetry read", "health " + telemetry.getHealthStatus());
            } catch (ItscamException e) {
                check(false, "telemetry read", e.getMessage());
            }

            // The stand-in pushes every 200 ms
            final AtomicInteger received = new AtomicInteger();
            try {
                telemetryRest.startTelemetryStream(new ItscamConsumer<TelemetryUpdate>() {
                    @Override
                    public void accept(TelemetryUpdate update) {
                        received.incrementAndGet();
                    }
                }, 0);
                for (int i = 0; i < 100 && received.get() < 2; i++) Thread.sleep(50);
                telemetryRest.stopTelemetryStream();
                TelemetryUpdate latest = telemetryRest.latestTelemetry();
                check(received.get() >= 2 && latest != null && latest.sequence() >= 2,
                        "telemetry stream", received.get() + " documents");
            } catch (ItscamException e) {
                check(false, "telemetry stream", e.getMessage());
            }
        } finally {
            telemetryRest.close();
        }

        // The stand-in's certificate is self-signed: nobody trusts it
        int tlsPort = Integer.parseInt(env("ITSCAM_TEST_TLS_PORT", "18443"));
        ItscamRestClient untrusting = new ItscamRestClient();
        try {
            untrusting.setBaseUrl(host, tlsPort, "https");
            untrusting.login("admin", "1234", 10000);
            check(false, "https refuses an untrusted certificate", "accepted");
        } catch (ItscamException e) {
            check(true, "https refuses an untrusted certificate", e.getMessage());
        } finally {
            untrusting.close();
        }
        ItscamRestClient trusting = new ItscamRestClient();
        try {
            trusting.setBaseUrl(host, tlsPort, "https");
            trusting.setCaCertFile(env("ITSCAM_TEST_CA", "tests/standin/tls/cert.pem"));
            trusting.login("admin", "1234", 10000);
            check(trusting.getProfiles(10000).size() == 1, "https with the certificate trusted", "");
        } catch (ItscamException e) {
            check(false, "https with the certificate trusted", e.getMessage());
        } finally {
            trusting.close();
        }

        // Nothing listens on port 1: fails fast and leaves no thread behind
        ItscamClient client = new ItscamClient();
        try {
            client.connect(host, 1, 2000);
            check(false, "binary connect to a closed port fails", "connected");
        } catch (ItscamException e) {
            check(true, "binary connect to a closed port fails", e.getMessage());
        } finally {
            client.close();
        }

        System.out.println(failures > 0 ? "Java smoke: FAILED" : "Java smoke: OK");
        System.exit(failures > 0 ? 1 : 0);
    }

    private static String escape(String s) {
        if (s == null) return "null";
        StringBuilder b = new StringBuilder("'");
        for (char c : s.toCharArray()) {
            if (c < 0x80) b.append(c);
            else b.append(String.format("\\u%04x", (int) c));
        }
        return b.append('\'').toString();
    }
}
