#!/usr/bin/env python3
"""
ITSCAM SDK - Snapshot-to-Freeflow Swap Example (Python)

Demonstrates the two main ITSCAM operational modes and how to switch
between them at runtime using the SDK:

  Mode 1 - SNAPSHOT:  On-demand image capture via snapshot.cgi (CGI client).
           The camera sits idle until the application requests an image.

  Mode 2 - FREEFLOW:  Continuous trigger with majority voting and automatic
           plate dispatch via the REST API Client (RAC) service.  The camera
           continuously captures, runs on-device OCR with voting, and POSTs
           results (plate + timestamp + JPEG base64) to an external HTTP
           endpoint.

When switching to freeflow, the example configures:
  - Day profile (Diurno) with single exposure + constant trigger
  - Night profile (Noturno) with 2 multi-exposure steps at different
    flash power + constant trigger
  - Analytics voting (majority voting) for plate accuracy
  - RAC server with a simple JSON body template

Usage::

    python3 snapshot_to_freeflow_example.py <host> <user> <password> [options]

Copyright (c) 2026 Pumatronix
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from itscam import (
    ItscamCgiClient,
    ItscamClient,
    ItscamRestClient,
    SnapshotCgiRequest,
)
from itscam import rest_types as rt


def snapshot_mode(host: str, *, https: bool = False,
                  insecure: bool = False) -> None:
    """Phase 1: on-demand snapshot captures via snapshot.cgi."""

    print()
    print("=" * 60)
    print("  PHASE 1 - SNAPSHOT MODE (on-demand via snapshot.cgi)")
    print("=" * 60)

    scheme = "https" if https else "http"
    port = 443 if https else 80

    with ItscamCgiClient() as cgi:
        cgi.set_base_url(host, port, scheme)
        if https and insecure:
            cgi.set_verify_server_certificate(False)

        # CGI auth is off by default; we don't call login() unless needed.
        print("Taking 3 on-demand snapshots via snapshot.cgi ...")

        for shot in range(1, 4):
            print(f"\n--- Snapshot {shot}/3 ---")

            images = cgi.get_snapshot(SnapshotCgiRequest(quality=85))
            print(f"  received {len(images)} image(s)")

            for i, img in enumerate(images):
                path = f"snapshot_phase1_{shot}_exp{i}.jpg"
                with open(path, "wb") as f:
                    f.write(img.data)
                print(f"  wrote {len(img.data)} bytes -> {path}")

            if shot < 3:
                time.sleep(1)

    print("\nSnapshot mode complete.")


def build_rac_config(rac_host: str, rac_port: int,
                     rac_path: str) -> rt.RESTAPIClientConfig:
    """Build the RAC configuration using the typed RESTAPIClientConfig."""
    # JSON body template -- the camera replaces {{...}} placeholders.
    body_template = (
        '{"plate":"{{plate}}",'
        '"timestamp":"{{utcYear}}-{{utcMonth}}-{{utcDay}}'
        'T{{utcHours}}:{{utcMinutes}}:{{utcSeconds}}.{{utcMilliseconds}}Z",'
        '"image":"{{image}}"}'
    )

    return rt.RESTAPIClientConfig(
        enabled=True,
        body=rt.Body(
            parts=[rt.Part(
                data=body_template,
                name="",
                type=rt.TypeEnum.JSON,
            )],
            variant=rt.Variant.SINGLEPART,
        ),
        headers=[rt.Header(name="Content-Type", value="application/json")],
        jpeg=rt.JPEG(quality=85, resolution=rt.Resolution(height=0, width=0)),
        method=rt.Method.POST,
        persistency=rt.Persistency(
            enabled=True,
            max_disk_usage=2147483648,   # 2 GiB
            max_file_age=604800,          # 7 days
            newest_first=False,
        ),
        retries=0,
        send_individual_requests=False,
        send_without_ocr=False,
        timeout=0,
        tls=rt.TLS(insecure=False, mtls_key=""),
        url=rt.URL(
            host=f"{rac_host}:{rac_port}",
            path=rac_path,
            query=[],
            scheme=rt.Scheme.HTTP,
        ),
    )


def build_night_profile(profile_id: int) -> rt.ProfileConfig:
    """Build a night profile with 2 multi-exposure steps at different
    flash power levels and continuous trigger."""

    # Step 1: flash at 80 % power
    step1 = rt.MultipleExposuresConfig(
        flash=rt.Flash(power=[rt.Power(out=1, percent=80)]),
        shutter=rt.Shutter(percentage_of_current=True, value=100.0),
        gain=rt.SettingGain(percentage_of_current=True, value=100.0),
    )

    # Step 2: flash at 40 % power
    step2 = rt.MultipleExposuresConfig(
        flash=rt.Flash(power=[rt.Power(out=1, percent=40)]),
        shutter=rt.Shutter(percentage_of_current=True, value=100.0),
        gain=rt.SettingGain(percentage_of_current=True, value=100.0),
    )

    return rt.ProfileConfig(
        id=profile_id,
        multiple_exposures=rt.MultipleExposures(
            enabled=True,
            settings=[step1, step2],
        ),
        trigger=rt.Trigger(
            enabled=True,
            event="constant",
            minimum_interval=150,   # ms between triggers
        ),
    )


def build_day_profile(profile_id: int) -> rt.ProfileConfig:
    """Build a day profile with single exposure and continuous trigger."""
    return rt.ProfileConfig(
        id=profile_id,
        multiple_exposures=rt.MultipleExposures(enabled=False),
        trigger=rt.Trigger(
            enabled=True,
            event="constant",
            minimum_interval=150,   # ms between triggers
        ),
    )


def build_voting_config() -> rt.AnalyticsConfig:
    """Build analytics config with majority voting enabled."""
    return rt.AnalyticsConfig(
        voting=rt.Voting(
            enabled=True,
            keep_best_only=True,
            max_diff_chars=2,
            same_plate_debounce=30,    # seconds
            use_classifier=True,
            forward_without_plate_if_tracker=False,
        ),
    )


def freeflow_mode(host: str, user: str, password: str, *,
                  https: bool = False, insecure: bool = False,
                  rac_host: str = "192.168.0.10", # <-- CHANGE THIS to your RAC destination host (e.g. a server on the LAN)
                  rac_port: int = 8080,
                  rac_path: str = "/api/captures",
                  profile_name: str = "Noturno",
                  day_profile_name: str = "Diurno",
                  duration_sec: int = 30) -> None:
    """Phase 2: configure freeflow, run for a duration, then restore."""

    print()
    print("=" * 60)
    print("  PHASE 2 - FREEFLOW MODE (continuous trigger + voting + RAC)")
    print("=" * 60)

    scheme = "https" if https else "http"
    port = 443 if https else 80

    # ---- REST client: configure equipment ----------------------------------

    with ItscamRestClient() as rest:
        rest.set_base_url(host, port, scheme)
        if https and insecure:
            rest.set_verify_server_certificate(False)

        print("\nAuthenticating with REST API...")
        rest.login(user, password)
        print(f"Logged in as {user!r}")

        # -- Find both profiles by name --

        print(f"\nLooking up night profile {profile_name!r}...")
        try:
            found_profile = rest.get_profile_by_name(profile_name)
        except Exception as e:
            print(f"ERROR: {e}")
            print("Available profiles:")
            for p in rest.get_profiles():
                print(f"  id={p.id}  name={p.name!r}")
            return
        profile_id = found_profile.id
        print(f"  found night profile id={profile_id}")

        print(f"\nLooking up day profile {day_profile_name!r}...")
        try:
            found_day_profile = rest.get_profile_by_name(day_profile_name)
        except Exception as e:
            print(f"ERROR: {e}")
            print("Available profiles:")
            for p in rest.get_profiles():
                print(f"  id={p.id}  name={p.name!r}")
            return
        day_profile_id = found_day_profile.id
        print(f"  found day profile id={day_profile_id}")

        # -- Save original configs so we can restore them later --
        # Only save the fields we modify (trigger + multiple_exposures).
        # Restoring the full profile would include read-only / computed
        # fields that the PUT endpoint rejects.

        print("\nSaving original configuration...")

        orig_night_trigger = found_profile.trigger
        orig_night_multi_exp = found_profile.multiple_exposures
        print(f"  saved night profile {profile_id}"
              f" (trigger + multipleExposures)")

        orig_day_trigger = found_day_profile.trigger
        orig_day_multi_exp = found_day_profile.multiple_exposures
        print(f"  saved day profile {day_profile_id}"
              f" (trigger + multipleExposures)")

        orig_analytics = rest.get_analytics_config()
        print("  saved analytics config")

        try:
            orig_rac = rest.get_restapi_client_config(0)
            had_rac = True
            print("  saved RAC server 0 config")
        except Exception:
            orig_rac = None
            had_rac = False
            print("  RAC server 0 not yet configured (will create)")

        # -- Configure both profiles with continuous trigger --

        print("\nConfiguring night profile "
              "(2 exposures, different flash power)...")
        night_profile = build_night_profile(profile_id)
        result = rest.update_profile_by_id(profile_id, night_profile)
        print(f"  night profile {profile_id} updated:")
        print(f"    {json.dumps(result.to_dict(), indent=2)[:400]}")

        print("\nConfiguring day profile "
              "(single exposure, constant trigger)...")
        day_profile = build_day_profile(day_profile_id)
        result = rest.update_profile_by_id(day_profile_id, day_profile)
        print(f"  day profile {day_profile_id} updated:")
        print(f"    {json.dumps(result.to_dict(), indent=2)[:400]}")

        # -- Configure analytics with majority voting --

        print("\nConfiguring analytics (majority voting)...")
        voting_cfg = build_voting_config()
        result = rest.set_analytics_config(voting_cfg)
        print("  voting enabled:")
        print(f"    {json.dumps(result.to_dict(), indent=2)[:400]}")

        # -- Configure RAC server --

        print("\nConfiguring RAC server 0 (HTTP webhook)...")
        rac_cfg = build_rac_config(rac_host, rac_port, rac_path)
        rest.set_restapi_client_config(0, rac_cfg)
        print("  RAC server 0 enabled:")
        print(f"    URL: http://{rac_host}:{rac_port}{rac_path}")
        print("    body: {plate, timestamp, image(base64)}")

    # -- Connect binary client and watch captures for the duration --

    print("\nConnecting binary client to receive trigger events...")

    frame_count = 0
    group_count = 0

    def on_trigger(result) -> None:
        nonlocal frame_count
        frame_count += 1
        print(
            f"  [trigger] frame #{frame_count}"
            f", RID={result.info.request_id}"
            f", exp {result.info.multi_exp_index + 1}"
            f"/{result.info.multi_exp_length}"
            f", {len(result.jpeg)} bytes"
        )

    with ItscamClient(host, timeout_ms=10000) as camera:
        if password:
            camera.authenticate(password)

        camera.on_trigger_image(on_trigger)
        camera.subscribe_captures()

        print(f"\nFreeflow running for {duration_sec} seconds...")
        print(f"  (Camera is sending captures to RAC at "
              f"http://{rac_host}:{rac_port}{rac_path})")

        time.sleep(duration_sec)

    print(f"\nFreeflow phase complete.")
    print(f"  total trigger frames: {frame_count}")

    # -- Restore original configuration --

    print("\n--- Restoring original configuration ---")

    with ItscamRestClient() as rest:
        rest.set_base_url(host, port, scheme)
        if https and insecure:
            rest.set_verify_server_certificate(False)
        rest.login(user, password)

        restore_night = rt.ProfileConfig(
            id=profile_id,
            trigger=orig_night_trigger,
            multiple_exposures=orig_night_multi_exp,
        )
        rest.update_profile_by_id(profile_id, restore_night)
        print(f"  night profile {profile_id} restored")

        restore_day = rt.ProfileConfig(
            id=day_profile_id,
            trigger=orig_day_trigger,
            multiple_exposures=orig_day_multi_exp,
        )
        rest.update_profile_by_id(day_profile_id, restore_day)
        print(f"  day profile {day_profile_id} restored")

        rest.set_analytics_config(orig_analytics)
        print("  analytics config restored")

        if had_rac and orig_rac is not None:
            rest.set_restapi_client_config(0, orig_rac)
            print("  RAC server 0 restored")


def main() -> int:
    p = argparse.ArgumentParser(
        description="ITSCAM Snapshot-to-Freeflow Swap Example",
    )
    p.add_argument("host", help="camera hostname or IP")
    p.add_argument("user", help="REST API username")
    p.add_argument("password", help="REST API password")
    p.add_argument("--https", action="store_true",
                   help="use HTTPS instead of HTTP")
    p.add_argument("--strict-tls", action="store_true",
                   help="require valid TLS certificate (default: insecure)")
    p.add_argument("--rac-host", default="localhost",
                   help="RAC destination host (default: localhost)")
    p.add_argument("--rac-port", type=int, default=8080,
                   help="RAC destination port (default: 8080)")
    p.add_argument("--rac-path", default="/api/captures",
                   help="RAC destination path (default: /api/captures)")
    p.add_argument("--day-profile", default="Diurno",
                   help="day profile name (default: Diurno)")
    p.add_argument("--profile-name", default="Noturno",
                   help="night profile name to configure (default: Noturno)")
    p.add_argument("--duration", type=int, default=30,
                   help="freeflow run duration in seconds (default: 30)")
    args = p.parse_args()

    print("ITSCAM Snapshot-to-Freeflow Swap Example")
    print(f"Target: {args.host}")

    # Phase 1: snapshot mode (CGI)
    snapshot_mode(args.host, https=args.https,
                   insecure=not args.strict_tls)

    # Phase 2: freeflow mode (REST + binary)
    freeflow_mode(
        args.host, args.user, args.password,
        https=args.https,
        insecure=not args.strict_tls,
        rac_host=args.rac_host,
        rac_port=args.rac_port,
        rac_path=args.rac_path,
        profile_name=args.profile_name,
        day_profile_name=args.day_profile,
        duration_sec=args.duration,
    )

    print("\nAll done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
