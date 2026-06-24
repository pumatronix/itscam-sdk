/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Output protocol configuration. */
public class ProtocolsConfig extends RestObject {
    public ProtocolsConfig() { super(); }
    public ProtocolsConfig(String json) { super(json); }
    public ProtocolsConfig(JsonObject json) { super(json); }
}