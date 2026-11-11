#!/usr/bin/env node
/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * System telemetry example.  Reads the state of the whole camera once, then
 * follows the documents the camera pushes for a while.
 *
 * Usage:
 *   node telemetry-example.js <host> <user> <password>
 *       [--seconds N] [--https] [--insecure]
 */
'use strict';

const path = require('path');
const { ItscamRestClient } = require(path.join(__dirname, '..', 'src'));

function printHealth(telemetry) {
    const health = telemetry.health;
    if (!health) {
        console.log('health: unavailable');
        return;
    }
    console.log('health:', health.status);
    for (const issue of health.issues || []) {
        console.log(`  [${issue.severity}] ${issue.id}: ${issue.message}`);
    }
}

async function main() {
    const args = process.argv.slice(2);
    if (args.length < 3) {
        console.error('Usage: telemetry-example.js <host> <user> <password> '
            + '[--seconds N] [--https] [--insecure]');
        process.exit(1);
    }
    const [host, user, password, ...flags] = args;
    const useHttps = flags.includes('--https');
    const insecure = flags.includes('--insecure');
    const secondsAt = flags.indexOf('--seconds');
    const seconds = secondsAt >= 0 ? Number(flags[secondsAt + 1]) : 30;

    const rest = new ItscamRestClient();
    try {
        rest.setBaseUrl(host, useHttps ? 443 : 80, useHttps ? 'https' : 'http');
        if (useHttps && insecure) {
            rest.setVerifyServerCertificate(false);
        }
        rest.login(user, password, 10000);

        // Polling: one read of the whole camera
        printHealth(rest.getSystemTelemetry(10000));

        // Stream: one document at once, then one every 5 s.  The callback
        // runs on the JS thread once the event loop is free.
        rest.startTelemetryStream((update) => {
            console.log(`\nupdate #${update.sequence}`);
            printHealth(update.document);
        });
        await new Promise((resolve) => setTimeout(resolve, seconds * 1000));
        await rest.stopTelemetryStream();

        const latest = rest.latestTelemetry();
        if (latest) {
            console.log(`\nlast document: #${latest.sequence}`);
        }
    } finally {
        rest.close();
    }
}

main().catch((error) => {
    console.error(error);
    process.exit(1);
});
