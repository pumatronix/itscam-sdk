/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonPrimitive;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/** Base class for generated REST configuration wrappers. */
public class RestObject {
    private static final Gson GSON = new GsonBuilder().create();

    protected final JsonObject json;

    public interface Factory<T> {
        T create(JsonObject json);
    }

    public RestObject() {
        this.json = new JsonObject();
    }

    public RestObject(String json) {
        this(parseObject(json));
    }

    public RestObject(JsonObject json) {
        this.json = json == null ? new JsonObject() : copyObject(json);
    }

    public JsonObject toJsonObject() {
        return copyObject(json);
    }

    public String toJsonString() {
        return GSON.toJson(json);
    }

    public boolean has(String name) {
        return json.has(name) && !json.get(name).isJsonNull();
    }

    public RestObject remove(String name) {
        json.remove(name);
        return this;
    }

    public JsonElement get(String name) {
        return json.get(name);
    }

    public RestObject set(String name, JsonElement value) {
        if (value == null) json.remove(name);
        else json.add(name, value);
        return this;
    }

    public String getString(String name) {
        JsonElement value = json.get(name);
        return value == null || value.isJsonNull() ? null : value.getAsString();
    }

    public RestObject setString(String name, String value) {
        if (value == null) json.remove(name);
        else json.addProperty(name, value);
        return this;
    }

    public Integer getInteger(String name) {
        JsonElement value = json.get(name);
        return value == null || value.isJsonNull() ? null : Integer.valueOf(value.getAsInt());
    }

    public RestObject setInteger(String name, Integer value) {
        if (value == null) json.remove(name);
        else json.addProperty(name, value);
        return this;
    }

    public Long getLong(String name) {
        JsonElement value = json.get(name);
        return value == null || value.isJsonNull() ? null : Long.valueOf(value.getAsLong());
    }

    public RestObject setLong(String name, Long value) {
        if (value == null) json.remove(name);
        else json.addProperty(name, value);
        return this;
    }

    public Double getDouble(String name) {
        JsonElement value = json.get(name);
        return value == null || value.isJsonNull() ? null : Double.valueOf(value.getAsDouble());
    }

    public RestObject setDouble(String name, Double value) {
        if (value == null) json.remove(name);
        else json.addProperty(name, value);
        return this;
    }

    public Boolean getBoolean(String name) {
        JsonElement value = json.get(name);
        return value == null || value.isJsonNull() ? null : Boolean.valueOf(value.getAsBoolean());
    }

    public RestObject setBoolean(String name, Boolean value) {
        if (value == null) json.remove(name);
        else json.addProperty(name, value);
        return this;
    }

    protected JsonObject getObject(String name) {
        JsonElement value = json.get(name);
        return value == null || !value.isJsonObject() ? null : value.getAsJsonObject();
    }

    protected RestObject setObject(String name, RestObject value) {
        if (value == null) json.remove(name);
        else json.add(name, value.toJsonObject());
        return this;
    }

    protected JsonArray getArray(String name) {
        JsonElement value = json.get(name);
        return value == null || !value.isJsonArray() ? null : value.getAsJsonArray();
    }

    public static JsonObject parseObject(String body) {
        if (body == null || body.length() == 0) {
            return new JsonObject();
        }
        JsonElement parsed = new JsonParser().parse(body);
        if (!parsed.isJsonObject()) {
            throw new IllegalArgumentException("Expected JSON object");
        }
        return parsed.getAsJsonObject();
    }

    public static JsonArray parseArray(String body) {
        if (body == null || body.length() == 0) {
            return new JsonArray();
        }
        JsonElement parsed = new JsonParser().parse(body);
        if (!parsed.isJsonArray()) {
            throw new IllegalArgumentException("Expected JSON array");
        }
        return parsed.getAsJsonArray();
    }

    public static <T> List<T> listFromJson(String body, Factory<T> factory) {
        JsonArray array = parseArray(body);
        List<T> out = new ArrayList<T>(array.size());
        for (JsonElement element : array) {
            if (!element.isJsonObject()) {
                throw new IllegalArgumentException("Expected JSON object in array");
            }
            out.add(factory.create(element.getAsJsonObject()));
        }
        return out;
    }

    public static String listToJson(List<? extends RestObject> values) {
        JsonArray array = new JsonArray();
        if (values != null) {
            for (RestObject value : values) {
                if (value != null) array.add(value.toJsonObject());
            }
        }
        return GSON.toJson(array);
    }

    private static JsonObject copyObject(JsonObject source) {
        JsonObject copy = new JsonObject();
        for (Map.Entry<String, JsonElement> entry : source.entrySet()) {
            copy.add(entry.getKey(), copyElement(entry.getValue()));
        }
        return copy;
    }

    private static JsonElement copyElement(JsonElement source) {
        if (source == null || source.isJsonNull()) return source;
        if (source.isJsonObject()) return copyObject(source.getAsJsonObject());
        if (source.isJsonArray()) {
            JsonArray copy = new JsonArray();
            for (JsonElement element : source.getAsJsonArray()) {
                copy.add(copyElement(element));
            }
            return copy;
        }
        JsonPrimitive primitive = source.getAsJsonPrimitive();
        if (primitive.isBoolean()) return new JsonPrimitive(Boolean.valueOf(primitive.getAsBoolean()));
        if (primitive.isNumber()) return new JsonPrimitive(primitive.getAsNumber());
        return new JsonPrimitive(primitive.getAsString());
    }
}