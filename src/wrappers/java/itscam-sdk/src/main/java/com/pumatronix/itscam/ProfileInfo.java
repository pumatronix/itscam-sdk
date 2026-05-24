/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Camera image profile entry returned by {@link ItscamClient#listProfiles}. */
public final class ProfileInfo {

    private final int    id;
    private final String name;
    private final String description;
    private final boolean active;

    public ProfileInfo(int id, String name, String description,
                       boolean active) {
        this.id = id;
        this.name = name == null ? "" : name;
        this.description = description == null ? "" : description;
        this.active = active;
    }

    public int    id() { return id; }
    public String name() { return name; }
    public String description() { return description; }
    public boolean active() { return active; }
}
