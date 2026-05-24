/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Connection-state transitions reported to {@code onConnectionState}. */
public enum ConnectionState {
    CONNECTED, DISCONNECTED, RECONNECTING, RECONNECTED, UNKNOWN;

    public static ConnectionState fromInt(int value) {
        switch (value) {
            case 0: return CONNECTED;
            case 1: return DISCONNECTED;
            case 2: return RECONNECTING;
            case 3: return RECONNECTED;
            default: return UNKNOWN;
        }
    }
}
