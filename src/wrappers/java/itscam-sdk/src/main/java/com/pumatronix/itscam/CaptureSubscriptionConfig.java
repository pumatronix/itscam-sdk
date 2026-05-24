/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/**
 * High-level capture subscription options.  Mirrors
 * {@code ITSCAM_CaptureSubscriptionConfig}; defaults match the SDK
 * recommended configuration.
 */
public final class CaptureSubscriptionConfig {

    private boolean includeTrigger = true;
    private boolean includeSnapshot = true;
    private boolean includeMetadata = true;
    private boolean embedComments = true;
    private boolean embedExif = true;
    private boolean embedSignature = false;
    private int     triggerQuality = -1;   // -1 = leave unchanged
    private int     snapshotQuality = -1;

    public CaptureSubscriptionConfig setIncludeTrigger(boolean v) {
        this.includeTrigger = v; return this;
    }
    public CaptureSubscriptionConfig setIncludeSnapshot(boolean v) {
        this.includeSnapshot = v; return this;
    }
    public CaptureSubscriptionConfig setIncludeMetadata(boolean v) {
        this.includeMetadata = v; return this;
    }
    public CaptureSubscriptionConfig setEmbedComments(boolean v) {
        this.embedComments = v; return this;
    }
    public CaptureSubscriptionConfig setEmbedExif(boolean v) {
        this.embedExif = v; return this;
    }
    public CaptureSubscriptionConfig setEmbedSignature(boolean v) {
        this.embedSignature = v; return this;
    }
    public CaptureSubscriptionConfig setTriggerQuality(int v) {
        this.triggerQuality = v; return this;
    }
    public CaptureSubscriptionConfig setSnapshotQuality(int v) {
        this.snapshotQuality = v; return this;
    }

    public boolean includeTrigger() { return includeTrigger; }
    public boolean includeSnapshot() { return includeSnapshot; }
    public boolean includeMetadata() { return includeMetadata; }
    public boolean embedComments() { return embedComments; }
    public boolean embedExif() { return embedExif; }
    public boolean embedSignature() { return embedSignature; }
    public int     triggerQuality() { return triggerQuality; }
    public int     snapshotQuality() { return snapshotQuality; }
}
