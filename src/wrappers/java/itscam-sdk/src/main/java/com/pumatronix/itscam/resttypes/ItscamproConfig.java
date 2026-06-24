/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** ITSCAM PRO server configuration. */
public class ItscamproConfig extends RestObject {
    public ItscamproConfig() { super(); }
    public ItscamproConfig(String json) { super(json); }
    public ItscamproConfig(JsonObject json) { super(json); }
}