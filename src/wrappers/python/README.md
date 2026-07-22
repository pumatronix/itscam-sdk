# ITSCAM SDK - Python Bindings

Python bindings for the Pumatronix ITSCAM Camera SDK.

## Installation

### Option A — Installed package (production / camera target)

```bash
pip install .
```

The native library (`libitscam_sdk.so` on Linux, `itscam_sdk.dll` on Windows) must be available in:

- The package directory
- System library paths (`/usr/lib`, `/usr/local/lib`)
- `LD_LIBRARY_PATH` (Linux) / `PATH` (Windows) environment variable

### Option B — Running directly from the source checkout (development)

Build the host library first (only needed once or after SDK changes):

```bash
# From the SDK repo root
make -C src/core linux          # x86-64 host
# make -C src/core linux-arm64  # only needed to cross-validate; not loadable on x86
```

Then, every time you run an example on your development machine, set two variables
so that Python picks up **this checkout** instead of any globally installed package:

```bash
export PYTHONPATH=/path/to/itscam-sdk/src/wrappers/python:$PYTHONPATH
export LD_LIBRARY_PATH=/path/to/itscam-sdk/src/core/build/linux:$LD_LIBRARY_PATH
```

Replace `/path/to/itscam-sdk` with the actual path on your machine.  
**Never** set `LD_LIBRARY_PATH` to `build/linux-arm64` when running on an
x86-64 host — that is an AArch64 library and `ctypes` cannot load it.

#### One-liner for the Pumatronix dev tree

```bash
SDK=/path/to/itscam-sdk
export PYTHONPATH=$SDK/src/wrappers/python:$PYTHONPATH
export LD_LIBRARY_PATH=$SDK/src/core/build/linux:$LD_LIBRARY_PATH
cd $SDK/src/wrappers/python
python3 examples/subscribe-trigger-cougar.py
```

## Quick Start

```python
from itscam import ItscamClient

# Connect to camera
with ItscamClient("192.168.1.100") as client:
    # Authenticate
    client.authenticate("password")

    # Capture snapshot
    results = client.capture_snapshot()

    # Save image
    results[0].save("snapshot.jpg")

    # Access recognized plates
    for plate in results[0].plates:
        print(f"Plate: {plate}")
```

## Features

- **Context manager support** - Automatic resource cleanup
- **Type hints** - Full typing support for IDE completion
- **Pythonic API** - snake_case naming, data classes
- **Callbacks** - Event-driven programming support
- **Cross-platform** - Works on Linux and Windows

## API Reference

### ItscamClient

```python
class ItscamClient:
    def __init__(
        self,
        address: str = None,
        port: int = 60000,
        timeout_ms: int = 5000,
        auto_reconnect: AutoReconnectConfig = None
    )

    # Connection
    def connect(address, port=60000, timeout_ms=5000, auto_reconnect=None)
    def disconnect()
    @property
    def is_connected -> bool

    # Authentication
    def authenticate(password, timeout_ms=10000)

    # Capture
    def capture_snapshot(scenario=-1, quality=80, force_capture=True, timeout_ms=15000) -> List[CaptureResult]
    def get_last_frame(quality=80, timeout_ms=10000) -> bytes

    # Profiles
    def get_active_profile_id(timeout_ms=10000) -> int
    def set_active_profile(profile_id, timeout_ms=10000)
    def list_profiles(timeout_ms=10000) -> List[ProfileInfo]

    # Callbacks
    def on_trigger_image(callback: Callable[[CaptureResult], None])
    def on_snapshot_image(callback: Callable[[CaptureResult], None])
    def on_disconnect(callback: Callable[[str], None])
    def on_connection_state(callback: Callable[[ConnectionState, str], None])
```

### Utility Functions

```python
from itscam import (
    get_system_local_time,
    get_system_utc_time,
    get_epoch_time,
    store_file,
    create_folder,
    file_exists,
    folder_exists,
)
```

## License

Copyright (c) 2026 Pumatronix. All rights reserved.
