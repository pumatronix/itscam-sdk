/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

import java.util.List;

/** I/O port metadata. */
public class IoBasic extends RestObject {
    public IoBasic() { super(); }
    public IoBasic(String json) { super(json); }
    public IoBasic(JsonObject json) { super(json); }

    public static List<IoBasic> listFromJson(String json) {
        return RestObject.listFromJson(json, new Factory<IoBasic>() {
            @Override
            public IoBasic create(JsonObject object) {
                return new IoBasic(object);
            }
        });
    }
}