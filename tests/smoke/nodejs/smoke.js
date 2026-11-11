#!/usr/bin/env node
/*
 *  smoke.js -- Node.js wrapper smoke test, against tests/standin/server.js.
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Same checks as the other languages (tests/README.md).  Runs on the
 *  oldest supported Node.js (16).  Environment: ITSCAM_TEST_HOST
 *  (127.0.0.1), ITSCAM_TEST_PORT (18080); the wrapper finds libitscam_sdk
 *  through ITSCAM_SDK_LIBRARY or LD_LIBRARY_PATH.
 */

"use strict";

const path = require("path");
const itscam = require(path.join(__dirname, "../../../src/wrappers/nodejs"));

let failures = 0;

function check(ok, name, detail) {
    console.log(`${ok ? "PASS" : "FAIL"} ${name}${detail ? ": " + detail : ""}`);
    if (!ok) failures++;
}

function main() {
    const host = process.env.ITSCAM_TEST_HOST || "127.0.0.1";
    const port = Number(process.env.ITSCAM_TEST_PORT || 18080);
    console.log(`Node.js ${process.version} smoke against ${host}:${port}`);

    const version = itscam.getNativeLibraryVersion();
    check(!!version, "version", version);

    const rest = new itscam.ItscamRestClient();
    try {
        rest.setBaseUrl(host, port, "http");
        try {
            rest.login("admin", "1234");
            check(true, "rest login");
        } catch (e) {
            check(false, "rest login", e.message);
        }

        try {
            const profiles = rest.getProfiles();
            const ok = profiles.length === 1 && profiles[0].id === 3 &&
                profiles[0].name === "Câmera São João";
            check(ok, "rest typed get", profiles.length
                ? `id ${profiles[0].id}, name '${profiles[0].name}'` : "no profile");
        } catch (e) {
            check(false, "rest typed get", e.message);
        }

        try {
            rest.get("/api/does-not-exist");
            check(false, "rest 404 maps to ItscamInvalidParameterError", "succeeded");
        } catch (e) {
            check(e instanceof itscam.ItscamInvalidParameterError,
                "rest 404 maps to ItscamInvalidParameterError", `${e.name}: ${e.message}`);
        }
    } finally {
        rest.close();
    }

    const cgi = new itscam.ItscamCgiClient();
    try {
        cgi.setBaseUrl(host, port, "http");
        try {
            const images = cgi.getSnapshot({});
            const plates = itscam.extractPlateRecognitions(
                itscam.parseJpegCommentTags(itscam.extractJpegComment(images[0].data)));
            check(images.length === 1 && plates.length === 1 && plates[0].plate === "ABC1D23",
                "cgi snapshot + jpeg plate", plates.length ? plates[0].plate : "no plate");
        } catch (e) {
            check(false, "cgi snapshot + jpeg plate", e.message);
        }
    } finally {
        cgi.close();
    }

    // Nothing listens on port 1: fails fast and leaves no thread behind
    const client = new itscam.ItscamClient();
    try {
        client.connect(host, 1, 2000);
        check(false, "binary connect to a closed port fails", "connected");
    } catch (e) {
        check(true, "binary connect to a closed port fails", e.message);
    } finally {
        client.close();
    }

    console.log(failures ? "Node.js smoke: FAILED" : "Node.js smoke: OK");
    return failures ? 1 : 0;
}

process.exitCode = main();
