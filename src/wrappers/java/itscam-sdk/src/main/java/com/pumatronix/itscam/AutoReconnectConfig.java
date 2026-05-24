/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Auto-reconnect policy for {@link ItscamClient}. */
public final class AutoReconnectConfig {

    private boolean enabled;
    private int     intervalMs = 3000;
    private int     maxRetries;          // 0 = unlimited

    public AutoReconnectConfig setEnabled(boolean v) {
        this.enabled = v; return this;
    }
    public AutoReconnectConfig setIntervalMs(int v) {
        this.intervalMs = v; return this;
    }
    public AutoReconnectConfig setMaxRetries(int v) {
        this.maxRetries = v; return this;
    }

    public boolean enabled() { return enabled; }
    public int     intervalMs() { return intervalMs; }
    public int     maxRetries() { return maxRetries; }
}
