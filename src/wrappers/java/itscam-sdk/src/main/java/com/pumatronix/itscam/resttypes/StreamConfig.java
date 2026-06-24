/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Video stream configuration. */
public class StreamConfig extends RestObject {
    public StreamConfig() { super(); }
    public StreamConfig(String json) { super(json); }
    public StreamConfig(JsonObject json) { super(json); }
}