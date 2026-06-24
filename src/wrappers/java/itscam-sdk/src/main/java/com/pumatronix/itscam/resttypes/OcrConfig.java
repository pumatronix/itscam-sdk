/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** OCR configuration. */
public class OcrConfig extends RestObject {
    public OcrConfig() { super(); }
    public OcrConfig(String json) { super(json); }
    public OcrConfig(JsonObject json) { super(json); }
}