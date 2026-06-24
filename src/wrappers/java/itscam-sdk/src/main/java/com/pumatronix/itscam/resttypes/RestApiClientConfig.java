/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** REST API client/webhook server configuration. */
public class RestApiClientConfig extends RestObject {
    public RestApiClientConfig() { super(); }
    public RestApiClientConfig(String json) { super(json); }
    public RestApiClientConfig(JsonObject json) { super(json); }
}