"""
ITSCAM SDK - Utility Functions

Cross-platform utility functions for file operations and time.
"""

from ctypes import byref, c_size_t, POINTER, c_ubyte, create_string_buffer
from typing import Union

from . import bindings as _b
from .types import Timestamp


def get_system_local_time() -> Timestamp:
    """Get the current system local time."""
    lib = _b.get_lib()
    ts = lib.ITSCAM_getSystemLocalTime()
    return Timestamp(
        year=ts.year,
        month=ts.month,
        day=ts.day,
        hour=ts.hour,
        minute=ts.minute,
        second=ts.second,
        millisecond=ts.millisecond,
        timezone_offset=ts.timezone_offset
    )


def get_system_utc_time() -> Timestamp:
    """Get the current system UTC time."""
    lib = _b.get_lib()
    ts = lib.ITSCAM_getSystemUtcTime()
    return Timestamp(
        year=ts.year,
        month=ts.month,
        day=ts.day,
        hour=ts.hour,
        minute=ts.minute,
        second=ts.second,
        millisecond=ts.millisecond,
        timezone_offset=ts.timezone_offset
    )


def get_epoch_time() -> int:
    """Get the Unix epoch time in seconds."""
    lib = _b.get_lib()
    return lib.ITSCAM_getEpochTime()


def get_epoch_time_ms() -> int:
    """Get the Unix epoch time in milliseconds."""
    lib = _b.get_lib()
    return lib.ITSCAM_getEpochTimeMs()


def store_file(path: str, data: Union[bytes, bytearray], overwrite: bool = True) -> bool:
    """
    Store data to a file.
    
    Args:
        path: File path.
        data: Data to write.
        overwrite: Overwrite if file exists.
    
    Returns:
        True on success, False on failure.
    """
    lib = _b.get_lib()
    data_bytes = bytes(data)
    data_array = (c_ubyte * len(data_bytes)).from_buffer_copy(data_bytes)
    return bool(lib.ITSCAM_storeFile(
        path.encode("utf-8"),
        data_array,
        len(data_bytes),
        1 if overwrite else 0
    ))


def create_folder(path: str, recursive: bool = True) -> bool:
    """
    Create a directory.
    
    Args:
        path: Directory path.
        recursive: Create parent directories if needed.
    
    Returns:
        True on success, False on failure.
    """
    lib = _b.get_lib()
    return bool(lib.ITSCAM_createFolder(
        path.encode("utf-8"),
        1 if recursive else 0
    ))


def file_exists(path: str) -> bool:
    """Check if a file exists."""
    lib = _b.get_lib()
    return bool(lib.ITSCAM_fileExists(path.encode("utf-8")))


def folder_exists(path: str) -> bool:
    """Check if a directory exists."""
    lib = _b.get_lib()
    return bool(lib.ITSCAM_folderExists(path.encode("utf-8")))


def get_version() -> str:
    """Get the SDK version string."""
    lib = _b.get_lib()
    version = lib.ITSCAM_getVersion()
    return version.decode("utf-8") if version else ""
