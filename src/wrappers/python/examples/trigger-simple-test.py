from itscam import ItscamClient
from itscam.types import ItscamTimeoutError, ItscamError
from itscam.types import (
    CaptureSubscriptionConfig)
import time

# Connect to camera
with ItscamClient("192.168.255.254") as client:
    client.authenticate("password")  # TODO: replace with the camera password
    # Authenticate
    print("Authenticated")

    # Subscribe to snapshot image events (required before capture_snapshot)
    client.subscribe_captures()
    print("Subscribed to captures")

    # Try get_last_frame first (simpler - no trigger required)
    try:
        print("Trying get_last_frame...")
        jpeg_bytes = client.get_last_frame(quality=80, timeout_ms=10000)
        with open("last_frame.jpg", "wb") as f:
            f.write(jpeg_bytes)
        print(f"Last frame saved ({len(jpeg_bytes)} bytes)")
    except ItscamTimeoutError as e:
        print(f"get_last_frame timeout: {e}")
    except ItscamError as e:
        print(f"get_last_frame error: {e}")

    try:  # this operation can fail if the trigger is not connected
        # Capture snapshot with increased timeout (30 seconds)
        print("Trying capture_snapshot...")
        results = client.capture_snapshot(timeout_ms=30000)

        # Save image
        results[0].save("snapshot.jpg")
        print("Snapshot saved to snapshot.jpg")

        # Access recognized plates
        for plate in results[0].plates:
            print(f"Plate: {plate}")

    except ItscamTimeoutError as e:
        print(f"Timeout error: camera did not respond in time. ({e})")
    except ItscamError as e:
        print(f"Camera error: {e}")

    try:  # this operation can fail if the trigger is not connected
        client.subscribe_captures(
            CaptureSubscriptionConfig(
                include_trigger=True,
                include_snapshot=False,
                include_metadata=True,
                embed_comments=True,
                trigger_quality=50,
            )
        )

        print("Subscription successful!")

        time.sleep(10)  # Wait for potential subscription errors
    except ItscamError as e:
        print(f"Camera error during subscription: {e}")
    except Exception as e:
        print(f"Unexpected error during subscription: {e}")
