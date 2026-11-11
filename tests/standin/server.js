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
 *
 *  /api/statusws?format=telemetry is a WebSocket pushing the system telemetry
 *  document every PUSH_MS (default 200 ms; the camera pushes every 5 s).
 */

"use strict";

const crypto = require("crypto");
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
    // GET /api/system/telemetry, and every document the stream pushes
    telemetry: () => ({
        version: 1,
        timestamp: new Date().toISOString(),
        device: { model: "ITSCAM 450 LM", family: "itscam450", firmwareVersion: "v1.7.11" },
        health: {
            status: "warning",
            issues: [{ id: "storageFull", severity: "warning", message: "Disk almost full" }],
        },
        camera: { reachable: true, pipelineReady: true, profile: { id: 3, name: "Câmera São João" } },
        services: {},
        processing: null,
        flashTest: null,
    }),
};
const PUSH_MS = Number(process.env.PUSH_MS || 200);

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
        if (route === "GET /api/system/telemetry") {
            return sendJson(res, 200, FIXTURES.telemetry());
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

// --- WebSocket (RFC 6455, the parts the SDK uses) ---------------------------

function wsFrame(opcode, payload) {
    const len = payload.length;
    const header = len < 126 ? Buffer.from([0x80 | opcode, len])
        : len < 65536 ? Buffer.from([0x80 | opcode, 126, len >> 8, len & 0xff])
            : Buffer.concat([Buffer.from([0x80 | opcode, 127]),
                (() => { const b = Buffer.alloc(8); b.writeBigUInt64BE(BigInt(len)); return b; })()]);
    return Buffer.concat([header, payload]);
}

// Client frames are masked; answers pings and closes, ignores the rest
function wsOnData(socket, state, chunk) {
    state.buf = Buffer.concat([state.buf, chunk]);
    for (;;) {
        const b = state.buf;
        if (b.length < 2) return;
        const opcode = b[0] & 0x0f;
        let len = b[1] & 0x7f, off = 2;
        if (len === 126) { if (b.length < 4) return; len = b.readUInt16BE(2); off = 4; }
        else if (len === 127) { if (b.length < 10) return; len = Number(b.readBigUInt64BE(2)); off = 10; }
        const masked = (b[1] & 0x80) !== 0;
        if (b.length < off + (masked ? 4 : 0) + len) return;
        const mask = masked ? b.subarray(off, off + 4) : null;
        off += masked ? 4 : 0;
        const payload = Buffer.from(b.subarray(off, off + len));
        if (mask) for (let i = 0; i < payload.length; i++) payload[i] ^= mask[i % 4];
        state.buf = b.subarray(off + len);
        if (opcode === 0x9) socket.write(wsFrame(0xa, payload));
        else if (opcode === 0x8) { socket.end(wsFrame(0x8, payload.subarray(0, 2))); return; }
    }
}

function upgrade(req, socket) {
    const url = new URL(req.url, "http://standin");
    const key = req.headers["sec-websocket-key"];
    const reject = (status, text) => {
        console.log(`GET ${req.url} (websocket) -> ${status}`);
        socket.end(`HTTP/1.1 ${status} ${text}\r\nContent-Length: 0\r\n\r\n`);
    };
    if (url.pathname !== "/api/statusws" || url.searchParams.get("format") !== "telemetry" || !key) {
        return reject(404, "Not Found");
    }
    if (!authorized(req)) return reject(401, "Unauthorized");
    const accept = crypto.createHash("sha1")
        .update(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").digest("base64");
    socket.write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n" +
        `Connection: Upgrade\r\nSec-WebSocket-Accept: ${accept}\r\n\r\n`);
    console.log(`GET ${req.url} (websocket) -> 101`);
    const state = { buf: Buffer.alloc(0) };
    const push = () => socket.write(wsFrame(0x1, Buffer.from(JSON.stringify(FIXTURES.telemetry()), "utf8")));
    push();
    const timer = setInterval(push, PUSH_MS);
    socket.on("data", (chunk) => wsOnData(socket, state, chunk));
    socket.on("close", () => clearInterval(timer));
    socket.on("error", () => clearInterval(timer));
}

server.on("upgrade", upgrade);

const port = Number(process.env.PORT || 18080);
server.listen(port, process.env.HOST || "0.0.0.0", () => {
    console.log(`stand-in listening on ${port}`);
});
