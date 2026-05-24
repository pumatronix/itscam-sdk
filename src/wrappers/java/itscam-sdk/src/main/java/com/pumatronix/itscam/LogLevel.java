/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Log level reported to the optional log handler. */
public enum LogLevel {
    INFO, ERROR, UNKNOWN;

    public static LogLevel fromInt(int value) {
        switch (value) {
            case 0: return INFO;
            case 1: return ERROR;
            default: return UNKNOWN;
        }
    }
}
