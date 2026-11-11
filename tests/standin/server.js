#!/usr/bin/env node
/*
 *  server.js -- a stand-in for the camera's web API, for the smoke tests.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Serves the few endpoints tests/smoke uses, with fixed answers the tests
 *  check exactly (see FIXTURES).  No dependencies: Node.js 16 or newer.
 *
 *    PORT=18080 node tests/standin/server.js
 *
 *  REST endpoints need the token POST /api/auth returns, as on the camera;
 *  CGI endpoints don't (configCgi.blockAPI defaults to false).  GET /health
 *  answers when the server is ready.  Every request is logged to stdout.
 */

"use strict";

const http = require("http");

// What the smoke tests expect, in every language
const FIXTURES = {
    username: "admin",
    password: "1234",
    token: "smoke-token",
    // A non-ASCII name catches a wrapper that doesn't decode UTF-8, and a
    // typed field after an optional one catches a struct layout mismatch
    profiles: [{ id: 3, name: "Câmera São João", active: true }],
    plate: "ABC1D23",
};

// The smallest JPEG the SDK's comment parser accepts: SOI, COM, EOI
function snapshotJpeg() {
    const comment = Buffer.from(
        `Placa=${FIXTURES.plate};CoordPlaca=10x20,100x30;CorPlaca=0;OCRCountryCode=76`,
        "latin1");
    const length = Buffer.alloc(2);
    length.writeUInt16BE(comment.length + 2);
    return Buffer.concat([
        Buffer.from([0xff, 0xd8, 0xff, 0xfe]), length, comment,
        Buffer.from([0xff, 0xd9]),
    ]);
}

function sendJson(res, status, body) {
    const data = Buffer.from(JSON.stringify(body), "utf8");
    res.writeHead(status, {
        "Content-Type": "application/json; charset=utf-8",
        "Content-Length": data.length,
    });
    res.end(data);
}

function readBody(req) {
    return new Promise((resolve) => {
        const chunks = [];
        req.on("data", (c) => chunks.push(c));
        req.on("end", () => resolve(Buffer.concat(chunks).toString("utf8")));
    });
}

function authorized(req) {
    return req.headers.authorization === `Bearer ${FIXTURES.token}`;
}

async function handle(req, res) {
    const url = new URL(req.url, "http://standin");
    const route = `${req.method} ${url.pathname}`;

    if (route === "GET /health") return sendJson(res, 200, { ok: true });

    if (route === "POST /api/auth") {
        let body = {};
        try { body = JSON.parse(await readBody(req)); } catch (e) { /* 401 */ }
        const creds = body.params || body;
        if (creds.username === FIXTURES.username &&
            creds.password === FIXTURES.password) {
            return sendJson(res, 200, { token: FIXTURES.token });
        }
        return sendJson(res, 401, { message: "invalid credentials" });
    }

    if (route === "GET /api/snapshot.cgi") {
        const jpeg = snapshotJpeg();
        res.writeHead(200, { "Content-Type": "image/jpeg", "Content-Length": jpeg.length });
        return res.end(jpeg);
    }

    if (url.pathname.startsWith("/api/")) {
        if (!authorized(req)) return sendJson(res, 401, { message: "Unauthorized" });
        if (route === "GET /api/image/profiles") {
            return sendJson(res, 200, FIXTURES.profiles);
        }
    }
    return sendJson(res, 404, { message: `no route for ${route}` });
}

const server = http.createServer((req, res) => {
    const started = Date.now();
    res.on("finish", () => {
        console.log(`${req.method} ${req.url} -> ${res.statusCode} (${Date.now() - started} ms)`);
    });
    handle(req, res).catch((e) => {
        console.error(e);
        if (!res.headersSent) sendJson(res, 500, { message: String(e) });
    });
});

const port = Number(process.env.PORT || 18080);
server.listen(port, process.env.HOST || "0.0.0.0", () => {
    console.log(`stand-in listening on ${port}`);
});
