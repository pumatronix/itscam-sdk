/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.jpeg.JpegMetadata;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;

/** Result of a single capture (one exposure / one image). */
public final class CaptureResult {

    private final FrameInfo info;
    private final byte[]    jpeg;

    public CaptureResult(FrameInfo info, byte[] jpeg) {
        this.info = info;
        this.jpeg = jpeg == null ? new byte[0] : jpeg;
    }

    public FrameInfo info() { return info; }

    /** Raw JPEG bytes; copy if you need to retain across method calls. */
    public byte[] jpeg() { return jpeg; }

    public List<String> plates() { return info.plates(); }

    /** Write the JPEG bytes to {@code path}. */
    public void save(String path) throws IOException {
        Files.write(Paths.get(path), jpeg);
    }

    public void save(Path path) throws IOException {
        Files.write(path, jpeg);
    }

    /**
     * Lazy-parse the JPEG COM marker into structured plate / classifier
     * metadata.  Returns an empty {@link JpegMetadata} if no marker.
     */
    public JpegMetadata parseJpegMetadata() {
        return JpegMetadata.parse(jpeg);
    }
}
