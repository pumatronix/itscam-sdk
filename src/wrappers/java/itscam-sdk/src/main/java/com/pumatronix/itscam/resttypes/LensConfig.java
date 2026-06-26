/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Camera lens configuration fields. */
public class LensConfig extends RestObject {
    public LensConfig() { super(); }
    public LensConfig(String json) { super(json); }
    public LensConfig(JsonObject json) { super(json); }

    public Integer getZoom() { return getInteger("zoom"); }
    public LensConfig setZoom(Integer zoom) { setInteger("zoom", zoom); return this; }

    public Integer getFocus() { return getInteger("focus"); }
    public LensConfig setFocus(Integer focus) { setInteger("focus", focus); return this; }

    public Boolean getExchanger() { return getBoolean("exchanger"); }
    public LensConfig setExchanger(Boolean exchanger) { setBoolean("exchanger", exchanger); return this; }

    public Boolean getZfMirrorProfile0() { return getBoolean("zfMirrorProfile0"); }
    public LensConfig setZfMirrorProfile0(Boolean value) { setBoolean("zfMirrorProfile0", value); return this; }
}