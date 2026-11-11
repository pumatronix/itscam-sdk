#!/usr/bin/env python3
"""Python wrapper smoke test, against tests/standin/server.js.

Copyright (c) 2026 Pumatronix

Same checks as the other languages (tests/README.md).  Runs on the oldest
supported Python (3.7).  Environment: ITSCAM_TEST_HOST (127.0.0.1),
ITSCAM_TEST_PORT (18080); the wrapper finds libitscam_sdk through
ITSCAM_SDK_LIBRARY or LD_LIBRARY_PATH.
"""

import os
import sys

import itscam
from itscam import (ItscamCgiClient, ItscamClient, ItscamError,
                    ItscamRestClient, extract_plate_recognitions,
                    parse_jpeg_comment_tags)
from itscam.types import ErrorCode

failures = 0


def check(ok, name, detail=""):
    global failures
    print("%s %s%s%s" % ("PASS" if ok else "FAIL", name, ": " if detail else "", detail))
    if not ok:
        failures += 1


def main():
    host = os.environ.get("ITSCAM_TEST_HOST") or "127.0.0.1"
    port = int(os.environ.get("ITSCAM_TEST_PORT") or "18080")
    print("Python %s smoke against %s:%d" % (sys.version.split()[0], host, port))

    version = itscam.get_version()
    check(bool(version), "version", version)

    rest = ItscamRestClient()
    try:
        rest.set_base_url(host, port, "http")
        try:
            rest.login("admin", "1234")
            check(True, "rest login")
        except ItscamError as e:
            check(False, "rest login", str(e))

        try:
            profiles = rest.get_profiles()
            ok = (len(profiles) == 1 and profiles[0].id == 3 and
                  profiles[0].name == "Câmera São João")
            check(ok, "rest typed get",
                  "id %s, name %r" % (profiles[0].id, profiles[0].name) if profiles else "no profile")
        except ItscamError as e:
            check(False, "rest typed get", str(e))

        try:
            rest.get("/api/does-not-exist")
            check(False, "rest 404 maps to INVALID_PARAMETER", "succeeded")
        except ItscamError as e:
            check(e.code == ErrorCode.INVALID_PARAMETER,
                  "rest 404 maps to INVALID_PARAMETER", str(e))
    finally:
        rest.close()

    cgi = ItscamCgiClient()
    try:
        cgi.set_base_url(host, port, "http")
        try:
            images = cgi.get_snapshot()
            plates = extract_plate_recognitions(
                parse_jpeg_comment_tags(itscam.extract_jpeg_comment(images[0].data)))
            check(len(images) == 1 and len(plates) == 1 and plates[0].plate == "ABC1D23",
                  "cgi snapshot + jpeg plate",
                  plates[0].plate if plates else "no plate")
        except ItscamError as e:
            check(False, "cgi snapshot + jpeg plate", str(e))
    finally:
        cgi.close()

    # Nothing listens on port 1: fails fast and leaves no thread behind
    client = ItscamClient()
    try:
        client.connect(host, 1, 2000)
        check(False, "binary connect to a closed port fails", "connected")
    except ItscamError as e:
        check(True, "binary connect to a closed port fails", str(e))
    finally:
        client.close()

    print("Python smoke: FAILED" if failures else "Python smoke: OK")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
