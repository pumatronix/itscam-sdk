/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** LINCE server configuration. */
public class LinceConfig extends RestObject {
    public LinceConfig() { super(); }
    public LinceConfig(String json) { super(json); }
    public LinceConfig(JsonObject json) { super(json); }
}