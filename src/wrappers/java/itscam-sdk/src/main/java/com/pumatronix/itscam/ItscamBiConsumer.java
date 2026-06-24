/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Two-argument callback interface compatible with JDK 7. */
public interface ItscamBiConsumer<T, U> {
    void accept(T first, U second);
}