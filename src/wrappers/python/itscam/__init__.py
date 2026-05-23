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
from .rest_client import ItscamRestClient
from .cgi_client import (
    ItscamCgiClient,
    SnapshotCgiRequest,
    CgiImage,
    CgiStreamFrame,
)
# Auto-generated POCOs for the typed REST surface (Phase 1 + Phase 2).
# Re-export the most useful entry points; the full module is also available
# as ``itscam.rest_types``.
from . import rest_types
from .rest_types import (
    ProfileConfig,
    OcrConfig,
    AnalyticsConfig,
    ClassifierConfig,
    AutoFocus,
    StreamConfig,
    Misc,
    MiscVolatile,
    ItscamproConfig,
    ItscamproStatus,
    ImageSignConfig,
    FTPConfig,
    LinceConfig,
    LinceStatus,
    VehicleIndicatorConfig,
    ProtocolsConfig,
    ProfileTransitioner,
    LanesConfig,
    IoConfig,
    IoBasic,
    RESTAPIClientConfig,
    RESTAPIClientStatus,
    Licenses,
)
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
from .jpeg_utils import (
    extract_jpeg_comment,
    parse_jpeg_comment_tags,
    parse_jpeg_metadata,
    PlateRecognition,
    ObjectDetection,
    JpegCommentMetadata,
    extract_plate_recognitions,
    extract_object_detections,
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

try:
    from ._version import (
        __version__,
        __version_full__,
        __git_sha__,
        __git_sha_short__,
        __build_date__,
    )
except ImportError:
    __version__ = "0.0.0"
    __version_full__ = __version__
    __git_sha__ = "unknown"
    __git_sha_short__ = "unknown"
    __build_date__ = "unknown"

__all__ = [
    # Clients
    "ItscamClient",
    "ItscamRestClient",
    "ItscamCgiClient",
    # CGI types
    "SnapshotCgiRequest",
    "CgiImage",
    "CgiStreamFrame",
    # Binary-client types
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
    # Typed REST POCOs (Phase 1 + Phase 2)
    "rest_types",
    "ProfileConfig",
    "OcrConfig",
    "AnalyticsConfig",
    "ClassifierConfig",
    "AutoFocus",
    "StreamConfig",
    "Misc",
    "MiscVolatile",
    "ItscamproConfig",
    "ItscamproStatus",
    "ImageSignConfig",
    "FTPConfig",
    "LinceConfig",
    "LinceStatus",
    "VehicleIndicatorConfig",
    "ProtocolsConfig",
    "ProfileTransitioner",
    "LanesConfig",
    "IoConfig",
    "IoBasic",
    "RESTAPIClientConfig",
    "RESTAPIClientStatus",
    "Licenses",
    # JPEG metadata
    "extract_jpeg_comment",
    "parse_jpeg_comment_tags",
    "parse_jpeg_metadata",
    "PlateRecognition",
    "ObjectDetection",
    "JpegCommentMetadata",
    "extract_plate_recognitions",
    "extract_object_detections",
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
