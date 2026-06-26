/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Miscellaneous equipment configuration. */
public class Misc extends RestObject {
    public Misc() { super(); }
    public Misc(String json) { super(json); }
    public Misc(JsonObject json) { super(json); }
}