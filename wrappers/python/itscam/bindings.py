"""
ITSCAM SDK - ctypes FFI Bindings

Low-level ctypes bindings to the ITSCAM native library.
This module should not be used directly; use the high-level wrappers instead.
"""

import ctypes
from ctypes import (
    c_int, c_uint16, c_uint32, c_uint64, c_int32,
    c_size_t, c_char_p, c_void_p, c_ubyte,
    Structure, POINTER, CFUNCTYPE, byref, cast
)
import os
import sys
from pathlib import Path
from typing import Optional


# ============================================================================
#  Library Loading
# ============================================================================

def _find_library() -> str:
    """Find the ITSCAM SDK library."""
    # Library names by platform
    if sys.platform == "win32":
        lib_names = ["itscam_sdk.dll", "libitscam_sdk.dll"]
    elif sys.platform == "darwin":
        lib_names = ["libitscam_sdk.dylib"]
    else:
        lib_names = ["libitscam_sdk.so", "libitscam_sdk.so.1"]
    
    # Search paths
    search_paths = [
        # Same directory as this module
        Path(__file__).parent,
        # Parent directory (sdk/)
        Path(__file__).parent.parent,
        # Build directories
        Path(__file__).parent.parent / "build",
        # System paths
        Path("/usr/local/lib"),
        Path("/usr/lib"),
    ]
    
    # Add LD_LIBRARY_PATH directories
    if "LD_LIBRARY_PATH" in os.environ:
        for p in os.environ["LD_LIBRARY_PATH"].split(os.pathsep):
            search_paths.append(Path(p))
    
    # Search for the library
    for search_path in search_paths:
        for lib_name in lib_names:
            lib_path = search_path / lib_name
            if lib_path.exists():
                return str(lib_path)
    
    # Try loading by name (system search)
    for lib_name in lib_names:
        try:
            if sys.platform == "win32":
                ctypes.WinDLL(lib_name)
            else:
                ctypes.CDLL(lib_name)
            return lib_name
        except OSError:
            continue
    
    raise OSError(
        f"Could not find ITSCAM SDK library. Searched: {lib_names}\n"
        f"Search paths: {[str(p) for p in search_paths]}"
    )


def _load_library():
    """Load and return the ITSCAM SDK library."""
    lib_path = _find_library()
    if sys.platform == "win32":
        return ctypes.WinDLL(lib_path)
    else:
        return ctypes.CDLL(lib_path)


# Load the library
_lib: Optional[ctypes.CDLL] = None

def get_lib() -> ctypes.CDLL:
    """Get the loaded library, loading it on first access."""
    global _lib
    if _lib is None:
        _lib = _load_library()
        _setup_prototypes(_lib)
    return _lib


# ============================================================================
#  C Structure Definitions
# ============================================================================

class ITSCAM_Timestamp(Structure):
    """C struct for timestamp."""
    _fields_ = [
        ("year", c_uint16),
        ("month", c_uint16),
        ("day", c_uint16),
        ("hour", c_uint16),
        ("minute", c_uint16),
        ("second", c_uint16),
        ("millisecond", c_uint16),
        ("timezone_offset", c_int32),
    ]


class ITSCAM_FrameInfo(Structure):
    """C struct for frame info."""
    _fields_ = [
        ("requestId", c_uint32),
        ("scenario", c_int32),
        ("multiExpIndex", c_int32),
        ("multiExpLength", c_int32),
        ("timestampUs", c_uint64),
        ("width", c_uint32),
        ("height", c_uint32),
    ]


class ITSCAM_AutoReconnectConfig(Structure):
    """C struct for auto-reconnect config."""
    _fields_ = [
        ("enabled", c_int),
        ("intervalMs", c_uint32),
        ("maxRetries", c_uint32),
    ]


class ITSCAM_EventSubscription(Structure):
    """C struct for event subscription."""
    _fields_ = [
        ("pipeline", c_int),
        ("triggerMetadata", c_int),
        ("triggerImage", c_int),
        ("snapshotMetadata", c_int),
        ("snapshotImage", c_int),
        ("previewMetadata", c_int),
        ("previewImage", c_int),
        ("gpio", c_int),
        ("serial1", c_int),
        ("serial2", c_int),
    ]


class ITSCAM_CaptureSubscriptionConfig(Structure):
    """C struct for high-level capture subscription config."""
    _fields_ = [
        ("includeTrigger", c_int),
        ("includeSnapshot", c_int),
        ("includeMetadata", c_int),
        ("embedComments", c_int),
        ("embedExif", c_int),
        ("embedSignature", c_int),
        ("triggerQuality", c_int),
        ("snapshotQuality", c_int),
    ]


class ITSCAM_SnapshotRequest(Structure):
    """C struct for snapshot request."""
    _fields_ = [
        ("scenario", c_int),
        ("quality", c_int),
        ("forceCapture", c_int),
    ]


class ITSCAM_ProfileInfo(Structure):
    """C struct for profile info."""
    _fields_ = [
        ("id", c_uint32),
        ("name", c_char_p),
        ("description", c_char_p),
        ("isActive", c_int),
    ]


# Opaque handle types
ITSCAM_Client = c_void_p
ITSCAM_CaptureResult = c_void_p
ITSCAM_CaptureResultArray = c_void_p
ITSCAM_ByteArray = c_void_p
ITSCAM_ProfileArray = c_void_p


# ============================================================================
#  Callback Types
# ============================================================================

# typedef void (*ITSCAM_CaptureCallback)(const ITSCAM_CaptureResult*, void*)
ITSCAM_CaptureCallback = CFUNCTYPE(None, c_void_p, c_void_p)

# typedef void (*ITSCAM_DisconnectCallback)(const char*, void*)
ITSCAM_DisconnectCallback = CFUNCTYPE(None, c_char_p, c_void_p)

# typedef void (*ITSCAM_ConnectionStateCallback)(int, const char*, void*)
ITSCAM_ConnectionStateCallback = CFUNCTYPE(None, c_int, c_char_p, c_void_p)

# typedef void (*ITSCAM_LogCallback)(int, const char*, void*)
ITSCAM_LogCallback = CFUNCTYPE(None, c_int, c_char_p, c_void_p)


# ============================================================================
#  Function Prototypes
# ============================================================================

def _setup_prototypes(lib: ctypes.CDLL) -> None:
    """Set up function prototypes for type checking."""
    
    # Client lifecycle
    lib.ITSCAM_Client_create.argtypes = []
    lib.ITSCAM_Client_create.restype = ITSCAM_Client
    
    lib.ITSCAM_Client_destroy.argtypes = [ITSCAM_Client]
    lib.ITSCAM_Client_destroy.restype = None
    
    # Connection
    lib.ITSCAM_Client_connect.argtypes = [
        ITSCAM_Client, c_char_p, c_uint16, c_uint32,
        POINTER(ITSCAM_AutoReconnectConfig)
    ]
    lib.ITSCAM_Client_connect.restype = c_int
    
    lib.ITSCAM_Client_disconnect.argtypes = [ITSCAM_Client]
    lib.ITSCAM_Client_disconnect.restype = None
    
    lib.ITSCAM_Client_isConnected.argtypes = [ITSCAM_Client]
    lib.ITSCAM_Client_isConnected.restype = c_int
    
    # Authentication
    lib.ITSCAM_Client_authenticate.argtypes = [ITSCAM_Client, c_char_p, c_uint32]
    lib.ITSCAM_Client_authenticate.restype = c_int
    
    # Subscription
    lib.ITSCAM_Client_subscribe.argtypes = [
        ITSCAM_Client, POINTER(ITSCAM_EventSubscription), c_uint32
    ]
    lib.ITSCAM_Client_subscribe.restype = c_int

    lib.ITSCAM_Client_subscribeCaptures.argtypes = [
        ITSCAM_Client, POINTER(ITSCAM_CaptureSubscriptionConfig), c_uint32
    ]
    lib.ITSCAM_Client_subscribeCaptures.restype = c_int
    
    # Capture
    lib.ITSCAM_Client_captureSnapshot.argtypes = [
        ITSCAM_Client, POINTER(ITSCAM_SnapshotRequest), c_uint32,
        POINTER(ITSCAM_CaptureResultArray)
    ]
    lib.ITSCAM_Client_captureSnapshot.restype = c_int
    
    lib.ITSCAM_Client_getLastFrame.argtypes = [
        ITSCAM_Client, c_int, c_uint32, POINTER(ITSCAM_ByteArray)
    ]
    lib.ITSCAM_Client_getLastFrame.restype = c_int
    
    # Profile management
    lib.ITSCAM_Client_getActiveProfileId.argtypes = [
        ITSCAM_Client, c_uint32, POINTER(c_uint32)
    ]
    lib.ITSCAM_Client_getActiveProfileId.restype = c_int
    
    lib.ITSCAM_Client_setActiveProfile.argtypes = [
        ITSCAM_Client, c_uint32, c_uint32
    ]
    lib.ITSCAM_Client_setActiveProfile.restype = c_int
    
    lib.ITSCAM_Client_listProfiles.argtypes = [
        ITSCAM_Client, c_uint32, POINTER(ITSCAM_ProfileArray)
    ]
    lib.ITSCAM_Client_listProfiles.restype = c_int
    
    # System
    lib.ITSCAM_Client_reboot.argtypes = [ITSCAM_Client, c_uint32]
    lib.ITSCAM_Client_reboot.restype = c_int
    
    # Callbacks
    lib.ITSCAM_Client_onTriggerImage.argtypes = [
        ITSCAM_Client, ITSCAM_CaptureCallback, c_void_p
    ]
    lib.ITSCAM_Client_onTriggerImage.restype = None
    
    lib.ITSCAM_Client_onSnapshotImage.argtypes = [
        ITSCAM_Client, ITSCAM_CaptureCallback, c_void_p
    ]
    lib.ITSCAM_Client_onSnapshotImage.restype = None
    
    lib.ITSCAM_Client_onDisconnect.argtypes = [
        ITSCAM_Client, ITSCAM_DisconnectCallback, c_void_p
    ]
    lib.ITSCAM_Client_onDisconnect.restype = None
    
    lib.ITSCAM_Client_onConnectionState.argtypes = [
        ITSCAM_Client, ITSCAM_ConnectionStateCallback, c_void_p
    ]
    lib.ITSCAM_Client_onConnectionState.restype = None
    
    lib.ITSCAM_Client_onLog.argtypes = [
        ITSCAM_Client, ITSCAM_LogCallback, c_void_p
    ]
    lib.ITSCAM_Client_onLog.restype = None
    
    # CaptureResult accessors
    lib.ITSCAM_CaptureResultArray_size.argtypes = [ITSCAM_CaptureResultArray]
    lib.ITSCAM_CaptureResultArray_size.restype = c_size_t
    
    lib.ITSCAM_CaptureResultArray_get.argtypes = [
        ITSCAM_CaptureResultArray, c_size_t
    ]
    lib.ITSCAM_CaptureResultArray_get.restype = ITSCAM_CaptureResult
    
    lib.ITSCAM_CaptureResultArray_destroy.argtypes = [ITSCAM_CaptureResultArray]
    lib.ITSCAM_CaptureResultArray_destroy.restype = None
    
    lib.ITSCAM_CaptureResult_getInfo.argtypes = [ITSCAM_CaptureResult]
    lib.ITSCAM_CaptureResult_getInfo.restype = ITSCAM_FrameInfo
    
    lib.ITSCAM_CaptureResult_getJpeg.argtypes = [
        ITSCAM_CaptureResult, POINTER(c_size_t)
    ]
    lib.ITSCAM_CaptureResult_getJpeg.restype = POINTER(c_ubyte)
    
    lib.ITSCAM_CaptureResult_getPlateCount.argtypes = [ITSCAM_CaptureResult]
    lib.ITSCAM_CaptureResult_getPlateCount.restype = c_size_t
    
    lib.ITSCAM_CaptureResult_getPlate.argtypes = [ITSCAM_CaptureResult, c_size_t]
    lib.ITSCAM_CaptureResult_getPlate.restype = c_char_p
    
    # ByteArray accessors
    lib.ITSCAM_ByteArray_size.argtypes = [ITSCAM_ByteArray]
    lib.ITSCAM_ByteArray_size.restype = c_size_t
    
    lib.ITSCAM_ByteArray_data.argtypes = [ITSCAM_ByteArray]
    lib.ITSCAM_ByteArray_data.restype = POINTER(c_ubyte)
    
    lib.ITSCAM_ByteArray_destroy.argtypes = [ITSCAM_ByteArray]
    lib.ITSCAM_ByteArray_destroy.restype = None
    
    # ProfileArray accessors
    lib.ITSCAM_ProfileArray_size.argtypes = [ITSCAM_ProfileArray]
    lib.ITSCAM_ProfileArray_size.restype = c_size_t
    
    lib.ITSCAM_ProfileArray_get.argtypes = [
        ITSCAM_ProfileArray, c_size_t, POINTER(ITSCAM_ProfileInfo)
    ]
    lib.ITSCAM_ProfileArray_get.restype = c_int
    
    lib.ITSCAM_ProfileArray_destroy.argtypes = [ITSCAM_ProfileArray]
    lib.ITSCAM_ProfileArray_destroy.restype = None
    
    # Utility functions
    lib.ITSCAM_getSystemLocalTime.argtypes = []
    lib.ITSCAM_getSystemLocalTime.restype = ITSCAM_Timestamp
    
    lib.ITSCAM_getSystemUtcTime.argtypes = []
    lib.ITSCAM_getSystemUtcTime.restype = ITSCAM_Timestamp
    
    lib.ITSCAM_getEpochTime.argtypes = []
    lib.ITSCAM_getEpochTime.restype = c_uint64
    
    lib.ITSCAM_getEpochTimeMs.argtypes = []
    lib.ITSCAM_getEpochTimeMs.restype = c_uint64
    
    lib.ITSCAM_storeFile.argtypes = [c_char_p, POINTER(c_ubyte), c_size_t, c_int]
    lib.ITSCAM_storeFile.restype = c_int
    
    lib.ITSCAM_createFolder.argtypes = [c_char_p, c_int]
    lib.ITSCAM_createFolder.restype = c_int
    
    lib.ITSCAM_fileExists.argtypes = [c_char_p]
    lib.ITSCAM_fileExists.restype = c_int
    
    lib.ITSCAM_folderExists.argtypes = [c_char_p]
    lib.ITSCAM_folderExists.restype = c_int
    
    lib.ITSCAM_getLastError.argtypes = []
    lib.ITSCAM_getLastError.restype = c_char_p
    
    lib.ITSCAM_getVersion.argtypes = []
    lib.ITSCAM_getVersion.restype = c_char_p
