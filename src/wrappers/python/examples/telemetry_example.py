#!/usr/bin/env python3
"""
ITSCAM SDK - system telemetry example (Python)

Reads the state of the whole camera once, then follows the documents the
camera pushes for a while.

Usage::

    python3 telemetry_example.py <host> <user> <password> [--seconds N] [--https]

Copyright (c) 2026 Pumatronix
"""
from __future__ import annotations

import argparse
import sys
import time

from itscam import ItscamRestClient, SystemTelemetry, TelemetryUpdate


def print_health(telemetry: SystemTelemetry) -> None:
    health = telemetry.health
    if health is None:
        print("health: unavailable")
        return
    print("health:", health.status)
    for issue in health.issues or []:
        print(f"  [{issue.severity}] {issue.id}: {issue.message}")


def on_update(update: TelemetryUpdate) -> None:
    # Runs on the SDK's callback pool: do not block here.
    print(f"\nupdate #{update.sequence}")
    print_health(update.telemetry())


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("host")
    p.add_argument("user")
    p.add_argument("password")
    p.add_argument("--seconds", type=int, default=30,
                   help="how long to follow the stream")
    p.add_argument("--https", action="store_true",
                   help="use HTTPS instead of HTTP")
    p.add_argument("--insecure", action="store_true",
                   help="skip TLS server certificate verification")
    args = p.parse_args()

    with ItscamRestClient() as rest:
        rest.set_base_url(args.host, 443 if args.https else 80,
                          "https" if args.https else "http")
        if args.https and args.insecure:
            rest.set_verify_server_certificate(False)
        rest.login(args.user, args.password)

        # Polling: one read of the whole camera
        print_health(rest.get_system_telemetry())

        # Stream: one document at once, then one every 5 s
        rest.start_telemetry_stream(on_update)
        time.sleep(args.seconds)
        rest.stop_telemetry_stream()

        latest = rest.latest_telemetry()
        if latest is not None:
            print(f"\nlast document: #{latest.sequence}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
