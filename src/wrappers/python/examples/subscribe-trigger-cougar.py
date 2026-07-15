import faulthandler
faulthandler.enable()

import json
import logging
import threading
from itscam.client import ItscamClient
from itscam.types import CaptureSubscriptionConfig, CaptureResult, ItscamError
import traceback, sys

logging.basicConfig(level=logging.INFO)

trigger_count = 0

def on_trigger(result: CaptureResult):
    global trigger_count
    trigger_count += 1
    filename = f"trigger_{trigger_count}.jpeg"
    result.save(filename)
    plates = result.plates
    print(f"[trigger #{trigger_count}] saved {filename} | plates: {plates}")

    # Print metadata (tags) if available
    metadata = result.info.metadata or {}
    if metadata:
        print(f"  metadata keys: {list(metadata.keys())}")
        vehicle_list_raw = metadata.get('VehicleList')
        if vehicle_list_raw:
            try:
                vehicles = json.loads(vehicle_list_raw)
                for v in vehicles:
                    plate = v.get('plate', {})
                    text = plate.get('text', '')
                    probs = plate.get('charProb', [])
                    print(f"  vehicle plate={text} probs={[str(p) for p in probs]}")
            except json.JSONDecodeError:
                print(f"  VehicleList: (invalid JSON)")

def main():
    try:
        with ItscamClient("192.168.255.254", port=60000) as client: # Replace with actual camera IP
            client.authenticate("****") # Replace with actual password if needed
            print("Authenticated successfully!")

            client.subscribe_captures(
                CaptureSubscriptionConfig(
                    include_trigger=True,
                    include_snapshot=False,
                    include_metadata=True,
                    embed_comments=True,
                    trigger_quality=80,
                )
            )
            print("Subscribed to trigger events. Waiting...")

            client.on_trigger_image(on_trigger)

            # Keep alive until Ctrl+C
            stop = threading.Event()
            try:
                stop.wait()
            except KeyboardInterrupt:
                print("Stopping...")

    except ItscamError as e:
        print(f"Camera error: {e}")
    except Exception:
        traceback.print_exc(file=sys.stderr)

if __name__ == "__main__":
    main()
