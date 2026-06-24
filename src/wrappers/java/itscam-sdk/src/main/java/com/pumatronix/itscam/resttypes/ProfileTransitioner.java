/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Profile transitioner configuration. */
public class ProfileTransitioner extends RestObject {
    public ProfileTransitioner() { super(); }
    public ProfileTransitioner(String json) { super(json); }
    public ProfileTransitioner(JsonObject json) { super(json); }
}