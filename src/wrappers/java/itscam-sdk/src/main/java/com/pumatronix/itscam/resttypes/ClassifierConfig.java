/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Vehicle classifier configuration. */
public class ClassifierConfig extends RestObject {
    public ClassifierConfig() { super(); }
    public ClassifierConfig(String json) { super(json); }
    public ClassifierConfig(JsonObject json) { super(json); }
}