/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** License status/configuration. */
public class Licenses extends RestObject {
    public Licenses() { super(); }
    public Licenses(String json) { super(json); }
    public Licenses(JsonObject json) { super(json); }
}