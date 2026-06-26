/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Analytics configuration. */
public class AnalyticsConfig extends RestObject {
    public AnalyticsConfig() { super(); }
    public AnalyticsConfig(String json) { super(json); }
    public AnalyticsConfig(JsonObject json) { super(json); }
}