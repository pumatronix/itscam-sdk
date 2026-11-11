/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

/**
 * State of the whole equipment.  A part that can't be read in time is null;
 * read the parts with {@link #get(String)} ({@code resources},
 * {@code services}, {@code camera}, {@code connections}, ...).
 */
public class SystemTelemetry extends RestObject {
    public SystemTelemetry() { super(); }
    public SystemTelemetry(String json) { super(json); }
    public SystemTelemetry(JsonObject json) { super(json); }

    /** Format version; fields are only added within a version. */
    public Integer getVersion() { return getInteger("version"); }

    /** When it was read, ISO 8601 UTC. */
    public String getTimestamp() { return getString("timestamp"); }

    /** Most severe issue: ok, warning or critical; null if absent. */
    public String getHealthStatus() {
        JsonObject health = getObject("health");
        JsonElement status = health == null ? null : health.get("status");
        return status == null || status.isJsonNull() ? null : status.getAsString();
    }
}
