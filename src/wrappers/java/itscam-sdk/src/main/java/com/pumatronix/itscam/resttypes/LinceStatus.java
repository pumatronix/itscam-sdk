/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** LINCE server status. */
public class LinceStatus extends RestObject {
    public LinceStatus() { super(); }
    public LinceStatus(String json) { super(json); }
    public LinceStatus(JsonObject json) { super(json); }
}