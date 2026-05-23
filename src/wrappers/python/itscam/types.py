"""
ITSCAM SDK - Python Type Definitions

Data classes and exception types for the ITSCAM SDK.
"""

from dataclasses import dataclass, field
from enum import IntEnum
from typing import List, Optional
from datetime import datetime


class ConnectionState(IntEnum):
    """Connection state enumeration.
    
    Values must match the C++ SDK enum in itscam_types.h.
    """
    CONNECTED = 0
    DISCONNECTED = 1
    RECONNECTING = 2
    RECONNECTED = 3


class LogLevel(IntEnum):
    """Log level enumeration.
    
    Values must match the C++ SDK enum in itscam_types.h.
    """
    INFO = 0
    ERROR = 1


class ErrorCode(IntEnum):
    """SDK error codes."""
    OK = 0
    CONNECTION_FAILED = 1
    TIMEOUT = 2
    NOT_AUTHENTICATED = 3
    INVALID_PARAMETER = 4
    SERVER_ERROR = 5
    DISCONNECTED = 6
    UNKNOWN = 7
    NULL_HANDLE = 8
    ALLOCATION_FAILED = 9


# ============================================================================
#  Exceptions
# ============================================================================

class ItscamError(Exception):
    """Base exception for ITSCAM SDK errors."""
    
    def __init__(self, message: str, code: ErrorCode = ErrorCode.UNKNOWN):
        super().__init__(message)
        self.code = code
        self.message = message
    
    def __str__(self) -> str:
        return f"[{self.code.name}] {self.message}"


class ItscamConnectionError(ItscamError):
    """Connection-related errors."""
    pass


class ItscamTimeoutError(ItscamError):
    """Timeout errors."""
    pass


class ItscamAuthError(ItscamError):
    """Authentication errors."""
    pass


def _raise_for_error(code: int, message: str = "") -> None:
    """Raise appropriate exception for error code."""
    if code == ErrorCode.OK:
        return
    
    error_code = ErrorCode(code) if code < len(ErrorCode) else ErrorCode.UNKNOWN
    
    if error_code == ErrorCode.CONNECTION_FAILED:
        raise ItscamConnectionError(message, error_code)
    elif error_code == ErrorCode.TIMEOUT:
        raise ItscamTimeoutError(message, error_code)
    elif error_code in (ErrorCode.NOT_AUTHENTICATED,):
        raise ItscamAuthError(message, error_code)
    elif error_code == ErrorCode.DISCONNECTED:
        raise ItscamConnectionError(message, error_code)
    else:
        raise ItscamError(message, error_code)


# ============================================================================
#  Data Classes
# ============================================================================

@dataclass
class Timestamp:
    """Timestamp with timezone information."""
    year: int = 0
    month: int = 0
    day: int = 0
    hour: int = 0
    minute: int = 0
    second: int = 0
    millisecond: int = 0
    timezone_offset: int = 0  # Seconds from UTC
    
    def to_datetime(self) -> datetime:
        """Convert to Python datetime object (naive, local time)."""
        return datetime(
            self.year, self.month, self.day,
            self.hour, self.minute, self.second,
            self.millisecond * 1000
        )
    
    def to_iso8601(self) -> str:
        """Format as ISO 8601 string."""
        return f"{self.year:04d}-{self.month:02d}-{self.day:02d}T" \
               f"{self.hour:02d}:{self.minute:02d}:{self.second:02d}." \
               f"{self.millisecond:03d}"
    
    def to_compact(self) -> str:
        """Format as compact string (YYYYMMDD_HHMMSS)."""
        return f"{self.year:04d}{self.month:02d}{self.day:02d}_" \
               f"{self.hour:02d}{self.minute:02d}{self.second:02d}"


@dataclass
class FrameInfo:
    """Metadata about a captured frame."""
    request_id: int = 0
    scenario: int = -1
    multi_exp_index: int = 0
    multi_exp_length: int = 1
    timestamp_us: int = 0
    width: int = 0
    height: int = 0
    plates: List[str] = field(default_factory=list)
    
    @property
    def timestamp(self) -> float:
        """Get timestamp in seconds."""
        return self.timestamp_us / 1_000_000.0


@dataclass
class CaptureResult:
    """Result of a capture operation."""
    info: FrameInfo = field(default_factory=FrameInfo)
    jpeg: bytes = b""
    
    @property
    def plates(self) -> List[str]:
        """Shortcut to access recognized plates."""
        return self.info.plates
    
    def save(self, path: str) -> None:
        """Save JPEG data to file."""
        with open(path, "wb") as f:
            f.write(self.jpeg)


@dataclass
class ProfileInfo:
    """Information about a camera profile."""
    id: int = 0
    name: str = ""
    description: str = ""
    is_active: bool = False


@dataclass
class AutoReconnectConfig:
    """Auto-reconnect configuration."""
    enabled: bool = False
    interval_ms: int = 3000
    max_retries: int = 0  # 0 = unlimited


@dataclass
class EventSubscription:
    """Event subscription configuration."""
    pipeline: bool = False
    trigger_metadata: bool = False
    trigger_image: bool = False
    snapshot_metadata: bool = False
    snapshot_image: bool = False
    preview_metadata: bool = False
    preview_image: bool = False
    pipeline_start: bool = False
    gpio: bool = False
    serial: bool = False
    serial1: bool = False
    serial2: bool = False


@dataclass
class CaptureSubscriptionConfig:
    """High-level capture subscription configuration."""
    include_trigger: bool = True
    include_snapshot: bool = True
    include_metadata: bool = True
    embed_comments: bool = True
    embed_exif: bool = True
    embed_signature: bool = False
    trigger_quality: int = -1
    snapshot_quality: int = -1


@dataclass
class SnapshotRequest:
    """Snapshot request configuration."""
    scenario: int = -1  # -1 = default
    quality: int = 80   # JPEG quality (1-100)
    force_capture: bool = True
