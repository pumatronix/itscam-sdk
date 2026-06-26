/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Single-argument callback interface compatible with JDK 7. */
public interface ItscamConsumer<T> {
    void accept(T value);
}