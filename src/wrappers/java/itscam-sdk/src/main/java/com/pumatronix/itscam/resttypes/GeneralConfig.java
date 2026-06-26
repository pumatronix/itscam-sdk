/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** General equipment configuration. */
public class GeneralConfig extends RestObject {
    public GeneralConfig() { super(); }
    public GeneralConfig(String json) { super(json); }
    public GeneralConfig(JsonObject json) { super(json); }
}