#!/usr/bin/env node
/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Binary client capture example.  Mirrors capture_example.py /
 * capture_example.go / BinaryCaptureExample/Program.cs.
 *
 * Usage:
 *   node capture-example.js <camera_ip> [password]
 */
'use strict';

const path = require('path');
const { setTimeout: sleep } = require('timers/promises');
const { ItscamClient, getNativeLibraryVersion } =
    require(path.join(__dirname, '..', 'src'));

async function main() {
    if (process.argv.length < 3) {
        console.error('Usage: capture-example.js <camera_ip> [password]');
        process.exit(1);
    }
    const host = process.argv[2];
    const password = process.argv[3] || null;

    console.log('ITSCAM SDK', getNativeLibraryVersion());

    let snapshotCount = 0;
    const camera = new ItscamClient();
    try {
        console.log(`Connecting to ${host}:60000 ...`);
        camera.connect(host, 60000, 10000, { enabled: true });

        if (password) {
            camera.authenticate(password, 10000);
            console.log('Authenticated.');
        }

        camera.onSnapshotImage((result) => {
            snapshotCount++;
            console.log(
                `  snapshot callback #${snapshotCount}: ${result.jpeg.length} bytes`);
        });

        camera.subscribeCaptures(undefined, 10000);

        const frames = camera.captureSnapshot(15000);
        console.log(`captureSnapshot returned ${frames.length} frame(s)`);
        frames.forEach((r, i) => {
            console.log(`  frame ${i + 1}: ${r.jpeg.length} bytes, plates=${JSON.stringify(r.plates)}`);
        });

        console.log('Waiting briefly for trigger frames ...');
        await sleep(2000);
    } finally {
        camera.close();
    }

    console.log(`Done (${snapshotCount} snapshot callback(s)).`);
}

main().catch((err) => {
    console.error(err);
    process.exit(1);
});
