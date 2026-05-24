/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * NativeLibrary -- locate libitscam_sdk on disk (or unpack the embedded
 * binary from the JAR) and expose a single shared JNA Library handle.
 */
package com.pumatronix.itscam.internal;

import com.sun.jna.Native;
import com.sun.jna.Platform;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

/**
 * Loads the ITSCAM native shared library and exposes the JNA bindings.
 *
 * <p>Resolution order:
 *
 * <ol>
 *   <li>System property {@code itscam.sdk.library} (absolute path).</li>
 *   <li>System path / {@code java.library.path} via JNA's default loader.</li>
 *   <li>Library bundled in the JAR at
 *       {@code META-INF/native/<os>-<arch>/<libname>}, extracted to a
 *       per-JVM temp directory.</li>
 * </ol>
 */
public final class NativeLibrary {

    private static final Object LOCK = new Object();
    private static volatile ItscamLibrary INSTANCE;

    private NativeLibrary() {}

    /** Return the shared JNA proxy, loading the library on first access. */
    public static ItscamLibrary get() {
        ItscamLibrary local = INSTANCE;
        if (local != null) {
            return local;
        }
        synchronized (LOCK) {
            if (INSTANCE == null) {
                INSTANCE = load();
            }
            return INSTANCE;
        }
    }

    private static ItscamLibrary load() {
        String override = System.getProperty("itscam.sdk.library");
        if (override != null && !override.isEmpty()) {
            return Native.load(override, ItscamLibrary.class);
        }
        try {
            return Native.load("itscam_sdk", ItscamLibrary.class);
        } catch (UnsatisfiedLinkError firstAttempt) {
            Path extracted = extractFromJar();
            if (extracted == null) {
                throw firstAttempt;
            }
            return Native.load(extracted.toAbsolutePath().toString(),
                    ItscamLibrary.class);
        }
    }

    private static Path extractFromJar() {
        String os;
        if (Platform.isWindows()) {
            os = "windows";
        } else if (Platform.isMac()) {
            os = "darwin";
        } else {
            os = "linux";
        }
        String arch = mapArch(System.getProperty("os.arch", ""));
        String libName = libName();
        String resource = "/META-INF/native/" + os + "-" + arch + "/"
                + libName;

        try (InputStream in = NativeLibrary.class
                .getResourceAsStream(resource)) {
            if (in == null) {
                return null;
            }
            Path dir = Files.createTempDirectory("itscam-sdk-");
            dir.toFile().deleteOnExit();
            Path out = dir.resolve(libName);
            Files.copy(in, out, StandardCopyOption.REPLACE_EXISTING);
            out.toFile().deleteOnExit();
            return out;
        } catch (IOException e) {
            throw new RuntimeException(
                    "Failed to extract " + resource + " from JAR", e);
        }
    }

    private static String mapArch(String arch) {
        String a = arch.toLowerCase();
        if (a.equals("amd64") || a.equals("x86_64")) return "x86_64";
        if (a.equals("aarch64") || a.equals("arm64")) return "aarch64";
        if (a.startsWith("arm")) return "arm";
        if (a.equals("x86") || a.equals("i386") || a.equals("i686")) return "x86";
        return a;
    }

    private static String libName() {
        if (Platform.isWindows()) return "itscam_sdk.dll";
        if (Platform.isMac()) return "libitscam_sdk.dylib";
        return "libitscam_sdk.so";
    }
}
