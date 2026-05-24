/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/**
 * SDK error codes returned by the native layer.  Mirrors
 * {@code ITSCAM_ErrorCode} in {@code itscam_sdk_c.h}.
 */
public enum ErrorCode {
    OK(0),
    CONNECTION_FAILED(1),
    TIMEOUT(2),
    NOT_AUTHENTICATED(3),
    INVALID_PARAMETER(4),
    SERVER_ERROR(5),
    DISCONNECTED(6),
    UNKNOWN(7),
    NULL_HANDLE(8),
    ALLOCATION_FAILED(9);

    private final int value;

    ErrorCode(int value) { this.value = value; }

    public int value() { return value; }

    public static ErrorCode fromInt(int value) {
        for (ErrorCode c : values()) {
            if (c.value == value) return c;
        }
        return UNKNOWN;
    }
}
