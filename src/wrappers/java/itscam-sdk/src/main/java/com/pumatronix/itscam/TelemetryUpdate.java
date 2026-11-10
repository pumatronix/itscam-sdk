/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.resttypes.SystemTelemetry;

/** One telemetry document pushed by the camera (see {@link ItscamRestClient#startTelemetryStream}). */
public final class TelemetryUpdate {

    private final long sequence;
    private final long receivedAtMs;
    private final String json;

    public TelemetryUpdate(long sequence, long receivedAtMs, String json) {
        this.sequence = sequence;
        this.receivedAtMs = receivedAtMs;
        this.json = json == null ? "" : json;
    }

    /** 1 for the stream's first document, then one more for each, across reconnections. */
    public long sequence() { return sequence; }

    /** When it arrived, in Unix milliseconds. */
    public long receivedAtMs() { return receivedAtMs; }

    /** The document, as {@code GET /api/system/telemetry} returns it. */
    public String json() { return json; }

    /** The document as a {@link SystemTelemetry}. */
    public SystemTelemetry telemetry() { return new SystemTelemetry(json); }
}
