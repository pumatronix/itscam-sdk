/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

import java.util.List;

/** I/O port configuration. */
public class IoConfig extends RestObject {
    public IoConfig() { super(); }
    public IoConfig(String json) { super(json); }
    public IoConfig(JsonObject json) { super(json); }

    public static List<IoConfig> listFromJson(String json) {
        return RestObject.listFromJson(json, new Factory<IoConfig>() {
            @Override
            public IoConfig create(JsonObject object) {
                return new IoConfig(object);
            }
        });
    }
}