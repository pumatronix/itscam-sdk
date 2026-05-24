/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import java.util.LinkedHashMap;
import java.util.Map;

/** Parameters for {@link ItscamCgiClient#getSnapshot(SnapshotCgiRequest, int)}. */
public final class SnapshotCgiRequest {

    private int[] shutters = new int[0];
    private int[] gains = new int[0];
    private int   quality = -1;     // -1 = camera default
    private boolean mosaic;
    private String format = "";
    private int   scenario = -1;    // -1 = unchanged
    private String crop = "";
    private String textOverlay = "";
    private final Map<String, String> userMetadata = new LinkedHashMap<>();

    public SnapshotCgiRequest setShutters(int... v) {
        this.shutters = v == null ? new int[0] : v.clone(); return this;
    }
    public SnapshotCgiRequest setGains(int... v) {
        this.gains = v == null ? new int[0] : v.clone(); return this;
    }
    public SnapshotCgiRequest setQuality(int v) { quality = v; return this; }
    public SnapshotCgiRequest setMosaic(boolean v) { mosaic = v; return this; }
    public SnapshotCgiRequest setFormat(String v) { format = v == null ? "" : v; return this; }
    public SnapshotCgiRequest setScenario(int v) { scenario = v; return this; }
    public SnapshotCgiRequest setCrop(String v) { crop = v == null ? "" : v; return this; }
    public SnapshotCgiRequest setTextOverlay(String v) {
        textOverlay = v == null ? "" : v; return this;
    }
    public SnapshotCgiRequest addUserMetadata(String key, String value) {
        if (key != null && value != null) userMetadata.put(key, value);
        return this;
    }

    public int[] shutters() { return shutters; }
    public int[] gains() { return gains; }
    public int quality() { return quality; }
    public boolean mosaic() { return mosaic; }
    public String format() { return format; }
    public int scenario() { return scenario; }
    public String crop() { return crop; }
    public String textOverlay() { return textOverlay; }
    public Map<String, String> userMetadata() { return userMetadata; }
}
