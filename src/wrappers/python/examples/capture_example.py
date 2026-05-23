#!/usr/bin/env python3
"""
ITSCAM SDK Python Example - Binary TCP client

Connects to the camera on port 60000, optionally authenticates, requests
a snapshot, and waits briefly for trigger frames.

Usage:
    python3 capture_example.py <camera_ip> [password]

Copyright (c) 2026 Pumatronix
"""
from __future__ import annotations

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from itscam import ItscamClient


def main() -> int:
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <camera_ip> [password]")
        return 1

    host = sys.argv[1]
    password = sys.argv[2] if len(sys.argv) > 2 else ""

    snapshot_count = 0

    def on_snapshot(result) -> None:
        nonlocal snapshot_count
        snapshot_count += 1
        print(
            f"  snapshot callback #{snapshot_count}: "
            f"{len(result.jpeg)} bytes"
        )

    print(f"Connecting to {host}:60000 ...")
    with ItscamClient(host, timeout_ms=10000) as client:
        if password:
            client.authenticate(password)
            print("Authenticated.")

        client.on_snapshot_image(on_snapshot)
        client.subscribe_captures()

        results = client.capture_snapshot()
        print(f"capture_snapshot returned {len(results)} frame(s)")
        for i, result in enumerate(results):
            print(f"  frame {i + 1}: {len(result.jpeg)} bytes")

        print("Waiting briefly for trigger frames ...")
        time.sleep(2)

    print(f"Done ({snapshot_count} snapshot callback(s)).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
