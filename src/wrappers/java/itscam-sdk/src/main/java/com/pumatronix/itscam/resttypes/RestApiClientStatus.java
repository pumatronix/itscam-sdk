/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** REST API client/webhook server status. */
public class RestApiClientStatus extends RestObject {
    public RestApiClientStatus() { super(); }
    public RestApiClientStatus(String json) { super(json); }
    public RestApiClientStatus(JsonObject json) { super(json); }
}