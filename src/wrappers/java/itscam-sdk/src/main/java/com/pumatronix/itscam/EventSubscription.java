/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/**
 * Low-level event subscription matrix.  Mirrors
 * {@code ITSCAM_EventSubscription}.  Most callers prefer
 * {@link CaptureSubscriptionConfig}.
 */
public final class EventSubscription {

    private boolean pipeline;
    private boolean triggerMetadata;
    private boolean triggerImage;
    private boolean snapshotMetadata;
    private boolean snapshotImage;
    private boolean previewMetadata;
    private boolean previewImage;
    private boolean gpio;
    private boolean serial1;
    private boolean serial2;

    public EventSubscription setPipeline(boolean v) { pipeline = v; return this; }
    public EventSubscription setTriggerMetadata(boolean v) { triggerMetadata = v; return this; }
    public EventSubscription setTriggerImage(boolean v) { triggerImage = v; return this; }
    public EventSubscription setSnapshotMetadata(boolean v) { snapshotMetadata = v; return this; }
    public EventSubscription setSnapshotImage(boolean v) { snapshotImage = v; return this; }
    public EventSubscription setPreviewMetadata(boolean v) { previewMetadata = v; return this; }
    public EventSubscription setPreviewImage(boolean v) { previewImage = v; return this; }
    public EventSubscription setGpio(boolean v) { gpio = v; return this; }
    public EventSubscription setSerial1(boolean v) { serial1 = v; return this; }
    public EventSubscription setSerial2(boolean v) { serial2 = v; return this; }

    public boolean pipeline() { return pipeline; }
    public boolean triggerMetadata() { return triggerMetadata; }
    public boolean triggerImage() { return triggerImage; }
    public boolean snapshotMetadata() { return snapshotMetadata; }
    public boolean snapshotImage() { return snapshotImage; }
    public boolean previewMetadata() { return previewMetadata; }
    public boolean previewImage() { return previewImage; }
    public boolean gpio() { return gpio; }
    public boolean serial1() { return serial1; }
    public boolean serial2() { return serial2; }
}
