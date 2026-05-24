#!/usr/bin/env node
/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * CGI snapshot example.  Mirrors cgi_snapshot_example.py /
 * cgi_snapshot_example.go.
 *
 * CGI auth is opt-in -- pass --user/--password only when the camera
 * has configCgi.blockAPI = true.
 *
 * Usage:
 *   node cgi-snapshot-example.js <host> [--https] [--insecure] \
 *                                [--user U --password P]
 */
'use strict';

const fs = require('fs');
const path = require('path');
const { setTimeout: sleep } = require('timers/promises');
const { ItscamCgiClient } = require(path.join(__dirname, '..', 'src'));

function parseArgs() {
    const a = process.argv.slice(2);
    if (a.length < 1) {
        console.error('Usage: cgi-snapshot-example.js <host> [flags]');
        process.exit(1);
    }
    const opts = { host: a[0], useHttps: false, insecure: false };
    for (let i = 1; i < a.length; i++) {
        switch (a[i]) {
            case '--https': opts.useHttps = true; break;
            case '--insecure': opts.insecure = true; break;
            case '--user': opts.user = a[++i]; break;
            case '--password': opts.password = a[++i]; break;
            default:
                console.error('Unknown flag', a[i]);
                process.exit(1);
        }
    }
    return opts;
}

async function main() {
    const opts = parseArgs();
    const scheme = opts.useHttps ? 'https' : 'http';
    const port   = opts.useHttps ? 443 : 80;

    const cgi = new ItscamCgiClient();
    try {
        cgi.setBaseUrl(opts.host, port, scheme);
        if (opts.useHttps && opts.insecure) {
            cgi.setVerifyServerCertificate(false);
        }
        if (opts.user && opts.password) {
            cgi.login(opts.user, opts.password, 10000);
            console.log('Logged in as', opts.user);
        } else {
            console.log(
                'No credentials supplied; talking to the camera without '
              + 'CGI authentication.');
        }

        console.log('Fetching lastframe.cgi ...');
        const last = cgi.getLastFrame(10000);
        fs.writeFileSync('lastframe.jpg', last.data);
        console.log(`  -> lastframe.jpg (${last.mimeType}, ${last.data.length} bytes)`);

        console.log('Triggering snapshot.cgi (Q=80, mosaic off) ...');
        const images = cgi.getSnapshot({ quality: 80 }, 15000);
        console.log(`  -> received ${images.length} image(s)`);
        images.forEach((img, i) => {
            const name = `snapshot-${i}.jpg`;
            fs.writeFileSync(name, img.data);
            console.log(`     ${name}: ${img.mimeType}, ${img.data.length} bytes`);
        });

        console.log('Streaming MJPEG for 5 seconds ...');
        let frameCount = 0;
        cgi.startMjpegStream((frame) => {
            frameCount++;
            if (frameCount === 1) {
                fs.writeFileSync('mjpeg-first.jpg', frame.data);
            }
        }, 10000);
        await sleep(5000);
        cgi.stopMjpegStream();
        console.log(`  -> received ${frameCount} MJPEG frame(s)`);
    } finally {
        cgi.close();
    }
}

main().catch((err) => {
    console.error(err);
    process.exit(1);
});
