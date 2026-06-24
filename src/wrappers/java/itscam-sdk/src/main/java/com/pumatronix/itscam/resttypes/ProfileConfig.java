/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

import java.util.List;

/** Image profile configuration. */
public class ProfileConfig extends RestObject {
    public ProfileConfig() { super(); }
    public ProfileConfig(String json) { super(json); }
    public ProfileConfig(JsonObject json) { super(json); }

    public static List<ProfileConfig> listFromJson(String json) {
        return RestObject.listFromJson(json, new Factory<ProfileConfig>() {
            @Override
            public ProfileConfig create(JsonObject object) {
                return new ProfileConfig(object);
            }
        });
    }

    public Long getId() { return getLong("id"); }
    public ProfileConfig setId(Long id) { setLong("id", id); return this; }

    public String getName() { return getString("name"); }
    public ProfileConfig setName(String name) { setString("name", name); return this; }

    public String getDescription() { return getString("description"); }
    public ProfileConfig setDescription(String description) { setString("description", description); return this; }

    public LensConfig getLens() {
        JsonObject value = getObject("lens");
        return value == null ? null : new LensConfig(value);
    }

    public ProfileConfig setLens(LensConfig lens) {
        setObject("lens", lens);
        return this;
    }
}