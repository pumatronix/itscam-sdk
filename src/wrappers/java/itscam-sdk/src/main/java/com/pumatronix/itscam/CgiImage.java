/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/** Single image returned by {@code lastframe.cgi} or {@code snapshot.cgi}. */
public final class CgiImage {

    private final String mimeType;
    private final byte[] data;

    public CgiImage(String mimeType, byte[] data) {
        this.mimeType = mimeType == null ? "" : mimeType;
        this.data = data == null ? new byte[0] : data;
    }

    public String mimeType() { return mimeType; }
    public byte[] data() { return data; }

    public void save(String path) throws IOException {
        Files.write(Paths.get(path), data);
    }

    public void save(Path path) throws IOException {
        Files.write(path, data);
    }
}
