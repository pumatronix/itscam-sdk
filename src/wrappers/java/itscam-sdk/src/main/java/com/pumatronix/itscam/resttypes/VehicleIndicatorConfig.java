/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** Vehicle indicator configuration. */
public class VehicleIndicatorConfig extends RestObject {
    public VehicleIndicatorConfig() { super(); }
    public VehicleIndicatorConfig(String json) { super(json); }
    public VehicleIndicatorConfig(JsonObject json) { super(json); }
}