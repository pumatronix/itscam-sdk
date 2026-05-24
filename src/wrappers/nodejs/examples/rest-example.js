#!/usr/bin/env node
/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * REST API example.  Mirrors rest_example.py / rest_example.go.
 *
 * Usage:
 *   node rest-example.js <host> <user> <password> [--https] [--insecure]
 */
'use strict';

const path = require('path');
const { ItscamRestClient } = require(path.join(__dirname, '..', 'src'));

function main() {
    const args = process.argv.slice(2);
    if (args.length < 3) {
        console.error(
            'Usage: rest-example.js <host> <user> <password> [--https] [--insecure]');
        process.exit(1);
    }
    const [host, user, password, ...flags] = args;
    const useHttps = flags.includes('--https');
    const insecure = flags.includes('--insecure');
    const scheme = useHttps ? 'https' : 'http';
    const port   = useHttps ? 443 : 80;

    const rest = new ItscamRestClient();
    try {
        rest.setBaseUrl(host, port, scheme);
        if (useHttps && insecure) {
            rest.setVerifyServerCertificate(false);
        }

        const loginResp = rest.login(user, password, 10000);
        console.log('login OK:', JSON.stringify(loginResp, null, 2));

        const volatileInfo = rest.get(
            '/api/equipment/misc/readonly/volatile', 10000);
        console.log('\nvolatile info:', JSON.stringify(volatileInfo, null, 2));

        const profiles = rest.get('/api/image/profiles', 10000);
        console.log('\nprofiles:', JSON.stringify(profiles, null, 2));
    } finally {
        rest.close();
    }
}

main();
