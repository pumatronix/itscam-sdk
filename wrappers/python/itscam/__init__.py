"""
ITSCAM SDK - Python Bindings

Pumatronix ITSCAM Camera Client SDK for Python.

This module provides Pythonic access to ITSCAM network cameras through
the native C/C++ SDK library using ctypes FFI.

Example usage:
    from itscam import ItscamClient

    with ItscamClient("192.168.1.100") as client:
        client.authenticate("password")
        result = client.capture_snapshot()
        with open("image.jpg", "wb") as f:
            f.write(result[0].jpeg)

Copyright (c) 2026 Pumatronix
"""

from .client import ItscamClient
from .types import (
    Timestamp,
    FrameInfo,
    CaptureResult,
    ProfileInfo,
    AutoReconnectConfig,
    EventSubscription,
    CaptureSubscriptionConfig,
    SnapshotRequest,
    ConnectionState,
    LogLevel,
    ItscamError,
    ItscamConnectionError,
    ItscamTimeoutError,
    ItscamAuthError,
)
from .utils import (
    get_system_local_time,
    get_system_utc_time,
    get_epoch_time,
    get_epoch_time_ms,
    store_file,
    create_folder,
    file_exists,
    folder_exists,
    get_version,
)

__version__ = "1.0.0"
__all__ = [
    # Main client
    "ItscamClient",
    # Types
    "Timestamp",
    "FrameInfo",
    "CaptureResult",
    "ProfileInfo",
    "AutoReconnectConfig",
    "EventSubscription",
    "CaptureSubscriptionConfig",
    "SnapshotRequest",
    "ConnectionState",
    "LogLevel",
    # Exceptions
    "ItscamError",
    "ItscamConnectionError",
    "ItscamTimeoutError",
    "ItscamAuthError",
    # Utilities
    "get_system_local_time",
    "get_system_utc_time",
    "get_epoch_time",
    "get_epoch_time_ms",
    "store_file",
    "create_folder",
    "file_exists",
    "folder_exists",
    "get_version",
]
