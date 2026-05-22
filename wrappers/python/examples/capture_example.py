#!/usr/bin/env python3
"""
ITSCAM SDK Python Example - Capture Events

This example demonstrates how to:
- Connect to an ITSCAM camera
- Authenticate
- Subscribe to capture events
- Save captured images to disk

Usage:
    python capture_example.py <camera_ip> [port]

Example:
    python capture_example.py 192.168.1.100 50000

Copyright (c) 2026 Pumatronix
"""

import sys
import time
import signal
from pathlib import Path

# Add parent directory to path for development
sys.path.insert(0, str(Path(__file__).parent.parent))

from itscam import ItscamClient, EventSubscription
from itscam.types import CaptureResult, ConnectionState


class CaptureExample:
    """Example application for capturing images from ITSCAM cameras."""
    
    def __init__(self, host: str, port: int = 50000):
        self.host = host
        self.port = port
        self.client = None
        self.running = True
        self.capture_count = 0
        self.output_dir = Path("captures")
        
    def on_capture(self, result: CaptureResult) -> None:
        """Handle capture events."""
        self.capture_count += 1
        info = result.frame_info
        
        print(f"\n[Capture #{self.capture_count}]")
        print(f"  Frame ID:   {info.frame_id}")
        print(f"  Scenario:   {info.scenario_id}")
        print(f"  Size:       {info.width}x{info.height}")
        print(f"  Timestamp:  {info.timestamp.to_iso8601()}")
        print(f"  JPEG Size:  {len(result.jpeg_data)} bytes")
        
        if info.plates:
            print(f"  Plates:     {', '.join(info.plates)}")
        
        # Save image
        filename = self.output_dir / f"capture_{info.frame_id}_{info.scenario_id}.jpg"
        result.save_jpeg(str(filename))
        print(f"  Saved:      {filename}")
    
    def on_disconnect(self, reason: str) -> None:
        """Handle disconnect events."""
        print(f"\n[Disconnected] {reason}")
    
    def on_state_change(self, state: ConnectionState, reason: str) -> None:
        """Handle connection state changes."""
        print(f"\n[State] {state.name}: {reason}")
    
    def run(self) -> int:
        """Main application loop."""
        # Create output directory
        self.output_dir.mkdir(exist_ok=True)
        
        print(f"ITSCAM SDK Python Example")
        print(f"Connecting to {self.host}:{self.port}...")
        
        try:
            with ItscamClient(self.host, self.port) as client:
                self.client = client
                
                # Connect with 10 second timeout
                client.connect(timeout=10.0)
                print("Connected!")
                
                # Authenticate (use default password)
                client.authenticate("admin", "1234")
                print("Authenticated!")
                
                # Get device info
                version = client.get_version()
                serial = client.get_serial()
                profiles = client.get_profile_count()
                active = client.get_active_profile()
                
                print(f"\nDevice Information:")
                print(f"  Version:        {version}")
                print(f"  Serial:         {serial}")
                print(f"  Profiles:       {profiles}")
                print(f"  Active Profile: {active}")
                
                # Set callbacks
                client.set_capture_callback(self.on_capture)
                client.set_disconnect_callback(self.on_disconnect)
                client.set_connection_state_callback(self.on_state_change)
                
                # Enable auto-reconnect
                client.enable_auto_reconnect(enabled=True, interval_ms=5000)
                
                # Subscribe to all capture events
                subscription = EventSubscription.all_captures()
                client.subscribe_to_events(subscription)
                
                print(f"\nListening for capture events...")
                print(f"Images will be saved to: {self.output_dir.absolute()}")
                print("Press Ctrl+C to exit.\n")
                
                # Wait for events
                while self.running:
                    time.sleep(0.1)
                    
        except KeyboardInterrupt:
            print("\n\nInterrupted by user.")
        except Exception as e:
            print(f"\nError: {e}")
            return 1
        finally:
            print(f"\nTotal captures: {self.capture_count}")
        
        return 0
    
    def stop(self) -> None:
        """Signal handler for graceful shutdown."""
        self.running = False


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <camera_ip> [port]")
        print(f"Example: {sys.argv[0]} 192.168.1.100 50000")
        return 1
    
    host = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 50000
    
    example = CaptureExample(host, port)
    
    # Set up signal handler
    signal.signal(signal.SIGINT, lambda s, f: example.stop())
    signal.signal(signal.SIGTERM, lambda s, f: example.stop())
    
    return example.run()


if __name__ == "__main__":
    sys.exit(main())
