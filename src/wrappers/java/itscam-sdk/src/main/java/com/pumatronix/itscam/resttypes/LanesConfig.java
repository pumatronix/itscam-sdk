/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Lane configuration. */
public class LanesConfig extends RestObject {
    public LanesConfig() { super(); }
    public LanesConfig(String json) { super(json); }
    public LanesConfig(JsonObject json) { super(json); }
}