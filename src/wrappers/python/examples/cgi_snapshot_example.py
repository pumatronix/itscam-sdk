#!/usr/bin/env python3
"""
ITSCAM SDK - CGI snapshot example (Python)

Triggers a snapshot.cgi capture (multi-exposure aware), writes each
resulting image to disk, fetches a lastframe.cgi preview, then streams
5 seconds of MJPEG to demonstrate the callback API.

CGI endpoints are unauthenticated by default on the camera
(``configCgi.blockAPI = false``); pass ``--user`` and ``--password``
only when the camera has CGI auth turned on.

Usage::

    python3 cgi_snapshot_example.py <host> [--https] [--insecure] \\
        [--user U --password P]

Copyright (c) 2026 Pumatronix
"""
from __future__ import annotations

import argparse
import sys
import time

from itscam import ItscamCgiClient, SnapshotCgiRequest


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("host")
    p.add_argument("--user", default=None,
                   help="opt-in CGI auth username (omit when blockAPI=false)")
    p.add_argument("--password", default=None,
                   help="opt-in CGI auth password")
    p.add_argument("--https", action="store_true")
    p.add_argument("--insecure", action="store_true",
                   help="skip TLS server certificate verification")
    args = p.parse_args()

    scheme = "https" if args.https else "http"
    port   = 443 if args.https else 80

    with ItscamCgiClient() as cgi:
        cgi.set_base_url(args.host, port, scheme)
        if args.https and args.insecure:
            cgi.set_verify_server_certificate(False)

        if args.user and args.password:
            cgi.login(args.user, args.password)
            print(f"Logged in as {args.user!r}")
        else:
            print("No credentials supplied; talking to the camera without "
                  "CGI authentication.")

        print("Fetching lastframe.cgi...")
        last = cgi.get_last_frame()
        with open("lastframe.jpg", "wb") as f:
            f.write(last.data)
        print(f"  -> lastframe.jpg ({last.mime_type}, {len(last.data)} bytes)")

        print("Triggering snapshot.cgi (Q=80, mosaic off)...")
        images = cgi.get_snapshot(SnapshotCgiRequest(quality=80))
        print(f"  -> received {len(images)} image(s)")
        for i, img in enumerate(images):
            path = f"snapshot-{i}.jpg"
            with open(path, "wb") as f:
                f.write(img.data)
            print(f"     {path}: {img.mime_type}, {len(img.data)} bytes")

        print("Streaming MJPEG for 5 seconds...")
        frames = []
        def on_frame(frame):
            frames.append(frame.sequence)
            if len(frames) == 1:
                with open("mjpeg-first.jpg", "wb") as f:
                    f.write(frame.data)
        cgi.start_mjpeg_stream(on_frame)
        time.sleep(5)
        cgi.stop_mjpeg_stream()
        print(f"  -> received {len(frames)} MJPEG frame(s)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
