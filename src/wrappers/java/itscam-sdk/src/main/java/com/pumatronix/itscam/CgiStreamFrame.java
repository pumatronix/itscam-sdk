/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Single MJPEG frame delivered to {@link ItscamCgiClient#startMjpegStream}. */
public final class CgiStreamFrame {

    private final long sequence;
    private final String mimeType;
    private final byte[] data;

    public CgiStreamFrame(long sequence, String mimeType, byte[] data) {
        this.sequence = sequence;
        this.mimeType = mimeType == null ? "" : mimeType;
        this.data = data == null ? new byte[0] : data;
    }

    public long sequence() { return sequence; }
    public String mimeType() { return mimeType; }
    public byte[] data() { return data; }
}
