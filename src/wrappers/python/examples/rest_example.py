#!/usr/bin/env python3
"""
ITSCAM SDK - REST API example (Python)

Logs into a camera over HTTP (or HTTPS with --https) and fetches a few
config endpoints to demonstrate the wrapper.

Usage::

    python3 rest_example.py <host> <user> <password> [--https]

Copyright (c) 2026 Pumatronix
"""
from __future__ import annotations

import argparse
import json
import sys

from itscam import ItscamRestClient


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("host")
    p.add_argument("user")
    p.add_argument("password")
    p.add_argument("--https", action="store_true",
                   help="use HTTPS instead of HTTP")
    p.add_argument("--insecure", action="store_true",
                   help="skip TLS server certificate verification")
    args = p.parse_args()

    scheme = "https" if args.https else "http"
    port   = 443 if args.https else 80

    with ItscamRestClient() as rest:
        rest.set_base_url(args.host, port, scheme)
        if args.https and args.insecure:
            rest.set_verify_server_certificate(False)
        login_resp = rest.login(args.user, args.password)
        print("login OK:", json.dumps(login_resp, indent=2))

        info = rest.get("/api/equipment/misc/readonly/volatile")
        print("\nvolatile info:", json.dumps(info, indent=2))

        profiles = rest.get("/api/image/profiles")
        print("\nprofiles:", json.dumps(profiles, indent=2))

    return 0


if __name__ == "__main__":
    sys.exit(main())
