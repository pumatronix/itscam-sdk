/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** ITSCAM PRO server status. */
public class ItscamproStatus extends RestObject {
    public ItscamproStatus() { super(); }
    public ItscamproStatus(String json) { super(json); }
    public ItscamproStatus(JsonObject json) { super(json); }
}