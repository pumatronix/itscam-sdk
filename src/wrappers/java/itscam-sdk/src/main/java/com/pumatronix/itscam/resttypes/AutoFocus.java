/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** AutoFocus configuration. */
public class AutoFocus extends RestObject {
    public AutoFocus() { super(); }
    public AutoFocus(String json) { super(json); }
    public AutoFocus(JsonObject json) { super(json); }

    public Boolean getRun() { return getBoolean("run"); }
    public AutoFocus setRun(Boolean run) { setBoolean("run", run); return this; }

    public AutoFocusRoi getRoi() {
        JsonObject value = getObject("roi");
        return value == null ? null : new AutoFocusRoi(value);
    }

    public AutoFocus setRoi(AutoFocusRoi roi) {
        setObject("roi", roi);
        return this;
    }

    public Integer getUpdateRate() { return getInteger("updateRate"); }
    public AutoFocus setUpdateRate(Integer updateRate) { setInteger("updateRate", updateRate); return this; }

    public Double getContrastThreshold() { return getDouble("contrastThreshold"); }
    public AutoFocus setContrastThreshold(Double value) { setDouble("contrastThreshold", value); return this; }

    public Integer getCoarseStep() { return getInteger("coarseStep"); }
    public AutoFocus setCoarseStep(Integer coarseStep) { setInteger("coarseStep", coarseStep); return this; }
}