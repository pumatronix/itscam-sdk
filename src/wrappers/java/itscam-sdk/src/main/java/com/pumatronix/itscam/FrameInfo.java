/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import java.util.Collections;
import java.util.List;

/** Metadata about a captured frame. */
public final class FrameInfo {

    private final long requestId;
    private final long frameCount;
    private final int  multiExpIndex;
    private final int  multiExpLength;
    private final int  shutter;
    private final float gain;
    private final int  width;
    private final int  height;
    private final Timestamp timestamp;
    private final List<String> plates;

    public FrameInfo(long requestId, long frameCount,
                     int multiExpIndex, int multiExpLength,
                     int shutter, float gain,
                     int width, int height,
                     Timestamp timestamp, List<String> plates) {
        this.requestId = requestId;
        this.frameCount = frameCount;
        this.multiExpIndex = multiExpIndex;
        this.multiExpLength = multiExpLength;
        this.shutter = shutter;
        this.gain = gain;
        this.width = width;
        this.height = height;
        this.timestamp = timestamp;
        this.plates = plates == null ? Collections.emptyList()
                                     : Collections.unmodifiableList(plates);
    }

    public long requestId() { return requestId; }
    public long frameCount() { return frameCount; }
    public int  multiExpIndex() { return multiExpIndex; }
    public int  multiExpLength() { return multiExpLength; }
    public int  shutter() { return shutter; }
    public float gain() { return gain; }
    public int  width() { return width; }
    public int  height() { return height; }
    public Timestamp timestamp() { return timestamp; }
    public List<String> plates() { return plates; }
}
