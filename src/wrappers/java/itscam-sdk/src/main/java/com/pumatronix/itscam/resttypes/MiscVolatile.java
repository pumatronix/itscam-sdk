/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Current miscellaneous read-only configuration values. */
public class MiscVolatile extends RestObject {
    public MiscVolatile() { super(); }
    public MiscVolatile(String json) { super(json); }
    public MiscVolatile(JsonObject json) { super(json); }

    public LensConfig getLens() {
        JsonObject value = getObject("lens");
        return value == null ? null : new LensConfig(value);
    }
}