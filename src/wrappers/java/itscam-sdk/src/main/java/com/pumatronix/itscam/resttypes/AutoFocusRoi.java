/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Auto-focus region of interest. */
public class AutoFocusRoi extends RestObject {
    public AutoFocusRoi() { super(); }
    public AutoFocusRoi(String json) { super(json); }
    public AutoFocusRoi(JsonObject json) { super(json); }

    public Integer getCenterX() { return getInteger("centerX"); }
    public AutoFocusRoi setCenterX(Integer centerX) { setInteger("centerX", centerX); return this; }

    public Integer getCenterY() { return getInteger("centerY"); }
    public AutoFocusRoi setCenterY(Integer centerY) { setInteger("centerY", centerY); return this; }

    public Integer getWidth() { return getInteger("width"); }
    public AutoFocusRoi setWidth(Integer width) { setInteger("width", width); return this; }

    public Integer getHeight() { return getInteger("height"); }
    public AutoFocusRoi setHeight(Integer height) { setInteger("height", height); return this; }
}