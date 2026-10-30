/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.internal.ItscamLibrary;
import com.pumatronix.itscam.internal.NativeLibrary;
import com.pumatronix.itscam.internal.SdkVersion;

/** Global utilities exposed by the native SDK. */
public final class Sdk {

    private Sdk() {}

    /**
     * Return the version string of the loaded {@code libitscam_sdk}
     * shared library (semver, e.g. {@code "0.0.1"}).
     */
    public static String getNativeLibraryVersion() {
        ItscamLibrary lib = NativeLibrary.get();
        String v = lib.ITSCAM_getVersion();
        return v == null ? "" : v;
    }

    /**
     * Maven group / artifact version of this Java wrapper.
     *
     * <p>This is sourced from {@link SdkVersion}, which is regenerated
     * from {@code tools/version/gen-version.sh} together with the
     * native library version.
     */
    public static String getWrapperVersion() {
        return SdkVersion.VERSION;
    }

    /** Full version descriptor including git SHA and build date. */
    public static String getWrapperVersionFull() {
        return SdkVersion.VERSION_FULL;
    }

    public static Timestamp getSystemLocalTime() {
        ItscamLibrary.ITSCAMTimestamp ts =
                NativeLibrary.get().ITSCAM_getSystemLocalTime();
        return new Timestamp(ts.year, ts.month, ts.day,
                ts.hour, ts.minute, ts.second,
                ts.millisecond, ts.timezoneOffset);
    }

    public static Timestamp getSystemUtcTime() {
        ItscamLibrary.ITSCAMTimestamp ts =
                NativeLibrary.get().ITSCAM_getSystemUtcTime();
        return new Timestamp(ts.year, ts.month, ts.day,
                ts.hour, ts.minute, ts.second,
                ts.millisecond, ts.timezoneOffset);
    }

    public static long getEpochTime() {
        return NativeLibrary.get().ITSCAM_getEpochTime();
    }

    public static long getEpochTimeMs() {
        return NativeLibrary.get().ITSCAM_getEpochTimeMs();
    }

    public static String getLastError() {
        String e = NativeLibrary.get().ITSCAM_getLastError();
        return e == null ? "" : e;
    }

    /**
     * Set how many threads run client callbacks.  Callbacks of every
     * client run on a pool of SDK threads shared by all clients, in order
     * and one at a time per client.
     *
     * @param count maximum number of threads; 0 restores the default (the number of CPU cores, at most 4)
     */
    public static void setCallbackThreads(int count) {
        if (count < 0) {
            throw new IllegalArgumentException("count must not be negative");
        }
        NativeLibrary.get().ITSCAM_setCallbackThreads(count);
    }
}
