/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Image signing configuration. */
public class ImageSignConfig extends RestObject {
    public ImageSignConfig() { super(); }
    public ImageSignConfig(String json) { super(json); }
    public ImageSignConfig(JsonObject json) { super(json); }
}