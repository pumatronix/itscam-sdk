"""
ITSCAM SDK - Python Client

High-level Pythonic wrapper for the ITSCAM camera client.
"""

from ctypes import byref, c_uint32, c_size_t, POINTER, c_ubyte, cast, c_char_p
from typing import Optional, List, Callable, Any
import weakref

from . import bindings as _b
from .types import (
    Timestamp, FrameInfo, CaptureResult, ProfileInfo,
    AutoReconnectConfig, EventSubscription, CaptureSubscriptionConfig,
    SnapshotRequest,
    ConnectionState, LogLevel, ErrorCode,
    _raise_for_error,
)


class ItscamClient:
    """
    ITSCAM Camera Client.
    
    Provides high-level access to ITSCAM network cameras. Supports both
    synchronous and callback-based operation.
    
    Can be used as a context manager for automatic cleanup:
        
        with ItscamClient("192.168.1.100") as client:
            client.authenticate("password")
            results = client.capture_snapshot()
    
    Args:
        address: IP address of the camera (optional, can connect later).
        port: TCP port (default 60000).
        timeout_ms: Connection timeout in milliseconds.
        auto_reconnect: Auto-reconnect configuration.
    """
    
    def __init__(
        self,
        address: Optional[str] = None,
        port: int = 60000,
        timeout_ms: int = 5000,
        auto_reconnect: Optional[AutoReconnectConfig] = None
    ):
        self._lib = _b.get_lib()
        self._handle = self._lib.ITSCAM_Client_create()
        if not self._handle:
            raise MemoryError("Failed to create ITSCAM client")
        
        # Store callbacks to prevent garbage collection
        self._callbacks: dict = {}
        
        # Auto-connect if address provided
        if address:
            self.connect(address, port, timeout_ms, auto_reconnect)
    
    def __del__(self):
        self.close()
    
    def __enter__(self) -> "ItscamClient":
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        self.close()
    
    def close(self) -> None:
        """Close and clean up the client."""
        if hasattr(self, "_handle") and self._handle:
            self._lib.ITSCAM_Client_destroy(self._handle)
            self._handle = None
            self._callbacks.clear()
    
    @property
    def is_connected(self) -> bool:
        """Check if the client is connected."""
        if not self._handle:
            return False
        return bool(self._lib.ITSCAM_Client_isConnected(self._handle))
    
    # =========================================================================
    #  Connection
    # =========================================================================
    
    def connect(
        self,
        address: str,
        port: int = 60000,
        timeout_ms: int = 5000,
        auto_reconnect: Optional[AutoReconnectConfig] = None
    ) -> None:
        """
        Connect to an ITSCAM camera.
        
        Args:
            address: IP address of the camera.
            port: TCP port (default 60000).
            timeout_ms: Connection timeout in milliseconds.
            auto_reconnect: Auto-reconnect configuration.
        
        Raises:
            ItscamConnectionError: If connection fails.
        """
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        reconnect_cfg = None
        if auto_reconnect:
            reconnect_cfg = _b.ITSCAM_AutoReconnectConfig(
                enabled=1 if auto_reconnect.enabled else 0,
                intervalMs=auto_reconnect.interval_ms,
                maxRetries=auto_reconnect.max_retries
            )
        
        result = self._lib.ITSCAM_Client_connect(
            self._handle,
            address.encode("utf-8"),
            port,
            timeout_ms,
            byref(reconnect_cfg) if reconnect_cfg else None
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
    
    def disconnect(self) -> None:
        """Disconnect from the camera."""
        if self._handle:
            self._lib.ITSCAM_Client_disconnect(self._handle)
    
    # =========================================================================
    #  Authentication
    # =========================================================================
    
    def authenticate(self, password: str, timeout_ms: int = 10000) -> None:
        """
        Authenticate with the camera.
        
        Args:
            password: Authentication password.
            timeout_ms: Request timeout in milliseconds.
        
        Raises:
            ItscamAuthError: If authentication fails.
        """
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        result = self._lib.ITSCAM_Client_authenticate(
            self._handle,
            password.encode("utf-8"),
            timeout_ms
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
    
    # =========================================================================
    #  Event Subscription
    # =========================================================================
    
    def subscribe(
        self,
        events: EventSubscription,
        timeout_ms: int = 10000
    ) -> None:
        """
        Subscribe to camera events.
        
        Args:
            events: Event subscription configuration.
            timeout_ms: Request timeout in milliseconds.
        
        Raises:
            ItscamError: If subscription fails.
        """
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        sub = _b.ITSCAM_EventSubscription(
            pipeline=1 if (events.pipeline or events.pipeline_start) else 0,
            triggerMetadata=1 if events.trigger_metadata else 0,
            triggerImage=1 if events.trigger_image else 0,
            snapshotMetadata=1 if events.snapshot_metadata else 0,
            snapshotImage=1 if events.snapshot_image else 0,
            previewMetadata=1 if events.preview_metadata else 0,
            previewImage=1 if events.preview_image else 0,
            gpio=1 if events.gpio else 0,
            serial1=1 if (events.serial1 or events.serial) else 0,
            serial2=1 if (events.serial2 or events.serial) else 0,
        )
        
        result = self._lib.ITSCAM_Client_subscribe(
            self._handle, byref(sub), timeout_ms
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")

    def subscribe_captures(
        self,
        config: Optional[CaptureSubscriptionConfig] = None,
        timeout_ms: int = 10000
    ) -> None:
        """Subscribe to capture events using the SDK high-level defaults."""
        if not self._handle:
            raise RuntimeError("Client has been closed")

        capture_config = config or CaptureSubscriptionConfig()
        c_config = _b.ITSCAM_CaptureSubscriptionConfig(
            includeTrigger=1 if capture_config.include_trigger else 0,
            includeSnapshot=1 if capture_config.include_snapshot else 0,
            includeMetadata=1 if capture_config.include_metadata else 0,
            embedComments=1 if capture_config.embed_comments else 0,
            embedExif=1 if capture_config.embed_exif else 0,
            embedSignature=1 if capture_config.embed_signature else 0,
            triggerQuality=capture_config.trigger_quality,
            snapshotQuality=capture_config.snapshot_quality,
        )

        result = self._lib.ITSCAM_Client_subscribeCaptures(
            self._handle, byref(c_config), timeout_ms
        )

        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
    
    # =========================================================================
    #  Capture
    # =========================================================================
    
    def capture_snapshot(
        self,
        scenario: int = -1,
        quality: int = 80,
        force_capture: bool = True,
        timeout_ms: int = 15000
    ) -> List[CaptureResult]:
        """
        Capture a snapshot from the camera.
        
        Args:
            scenario: Capture scenario (-1 for default).
            quality: JPEG quality (1-100).
            force_capture: Force new capture vs return cached.
            timeout_ms: Request timeout in milliseconds.
        
        Returns:
            List of capture results (one per exposure for multi-exposure).
        
        Raises:
            ItscamError: If capture fails.
        """
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        request = _b.ITSCAM_SnapshotRequest(
            scenario=scenario,
            quality=quality,
            forceCapture=1 if force_capture else 0
        )
        
        result_array = _b.ITSCAM_CaptureResultArray()
        
        result = self._lib.ITSCAM_Client_captureSnapshot(
            self._handle, byref(request), timeout_ms, byref(result_array)
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
        
        try:
            return self._convert_capture_results(result_array)
        finally:
            self._lib.ITSCAM_CaptureResultArray_destroy(result_array)
    
    def get_last_frame(
        self,
        quality: int = 80,
        timeout_ms: int = 10000
    ) -> bytes:
        """
        Get the most recent frame without triggering a new capture.
        
        Args:
            quality: JPEG quality (1-100).
            timeout_ms: Request timeout in milliseconds.
        
        Returns:
            JPEG image data.
        
        Raises:
            ItscamError: If request fails.
        """
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        byte_array = _b.ITSCAM_ByteArray()
        
        result = self._lib.ITSCAM_Client_getLastFrame(
            self._handle, quality, timeout_ms, byref(byte_array)
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
        
        try:
            size = self._lib.ITSCAM_ByteArray_size(byte_array)
            data_ptr = self._lib.ITSCAM_ByteArray_data(byte_array)
            if data_ptr and size > 0:
                return bytes(data_ptr[:size])
            return b""
        finally:
            self._lib.ITSCAM_ByteArray_destroy(byte_array)
    
    # =========================================================================
    #  Profile Management
    # =========================================================================
    
    def get_active_profile_id(self, timeout_ms: int = 10000) -> int:
        """Get the active profile ID."""
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        profile_id = c_uint32()
        result = self._lib.ITSCAM_Client_getActiveProfileId(
            self._handle, timeout_ms, byref(profile_id)
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
        
        return profile_id.value
    
    def set_active_profile(
        self,
        profile_id: int,
        timeout_ms: int = 10000
    ) -> None:
        """Set the active profile."""
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        result = self._lib.ITSCAM_Client_setActiveProfile(
            self._handle, profile_id, timeout_ms
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
    
    def list_profiles(self, timeout_ms: int = 10000) -> List[ProfileInfo]:
        """List all available profiles."""
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        profile_array = _b.ITSCAM_ProfileArray()
        result = self._lib.ITSCAM_Client_listProfiles(
            self._handle, timeout_ms, byref(profile_array)
        )
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
        
        try:
            profiles = []
            size = self._lib.ITSCAM_ProfileArray_size(profile_array)
            for i in range(size):
                info = _b.ITSCAM_ProfileInfo()
                if self._lib.ITSCAM_ProfileArray_get(profile_array, i, byref(info)):
                    profiles.append(ProfileInfo(
                        id=info.id,
                        name=info.name.decode("utf-8") if info.name else "",
                        description=info.description.decode("utf-8") if info.description else "",
                        is_active=bool(info.isActive)
                    ))
            return profiles
        finally:
            self._lib.ITSCAM_ProfileArray_destroy(profile_array)
    
    # =========================================================================
    #  System
    # =========================================================================
    
    def reboot(self, timeout_ms: int = 10000) -> None:
        """Reboot the camera."""
        if not self._handle:
            raise RuntimeError("Client has been closed")
        
        result = self._lib.ITSCAM_Client_reboot(self._handle, timeout_ms)
        
        if result != ErrorCode.OK:
            error_msg = self._lib.ITSCAM_getLastError()
            _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
    
    # =========================================================================
    #  Callbacks
    # =========================================================================
    
    def on_trigger_image(
        self,
        callback: Optional[Callable[[CaptureResult], None]]
    ) -> None:
        """
        Set callback for trigger images.
        
        Args:
            callback: Callback function, or None to unset.
        """
        self._set_capture_callback("trigger_image", callback,
                                   self._lib.ITSCAM_Client_onTriggerImage)
    
    def on_snapshot_image(
        self,
        callback: Optional[Callable[[CaptureResult], None]]
    ) -> None:
        """
        Set callback for snapshot images.
        
        Args:
            callback: Callback function, or None to unset.
        """
        self._set_capture_callback("snapshot_image", callback,
                                   self._lib.ITSCAM_Client_onSnapshotImage)
    
    def on_disconnect(
        self,
        callback: Optional[Callable[[str], None]]
    ) -> None:
        """
        Set callback for disconnect events.
        
        Args:
            callback: Callback function receiving reason string.
        """
        if not self._handle:
            return
        
        if callback:
            @_b.ITSCAM_DisconnectCallback
            def wrapper(reason, user_data):
                try:
                    callback(reason.decode("utf-8") if reason else "")
                except Exception:
                    pass  # Don't let exceptions escape to C
            
            self._callbacks["disconnect"] = wrapper
            self._lib.ITSCAM_Client_onDisconnect(self._handle, wrapper, None)
        else:
            self._callbacks.pop("disconnect", None)
            self._lib.ITSCAM_Client_onDisconnect(self._handle, None, None)
    
    def on_connection_state(
        self,
        callback: Optional[Callable[[ConnectionState, str], None]]
    ) -> None:
        """
        Set callback for connection state changes.
        
        Args:
            callback: Callback function receiving state and reason.
        """
        if not self._handle:
            return
        
        if callback:
            @_b.ITSCAM_ConnectionStateCallback
            def wrapper(state, reason, user_data):
                try:
                    callback(ConnectionState(state),
                            reason.decode("utf-8") if reason else "")
                except Exception:
                    pass
            
            self._callbacks["connection_state"] = wrapper
            self._lib.ITSCAM_Client_onConnectionState(self._handle, wrapper, None)
        else:
            self._callbacks.pop("connection_state", None)
            self._lib.ITSCAM_Client_onConnectionState(self._handle, None, None)
    
    def on_log(
        self,
        callback: Optional[Callable[[LogLevel, str], None]]
    ) -> None:
        """
        Set callback for log messages.
        
        Args:
            callback: Callback function receiving log level and message.
        """
        if not self._handle:
            return
        
        if callback:
            @_b.ITSCAM_LogCallback
            def wrapper(level, message, user_data):
                try:
                    callback(LogLevel(level),
                            message.decode("utf-8") if message else "")
                except Exception:
                    pass
            
            self._callbacks["log"] = wrapper
            self._lib.ITSCAM_Client_onLog(self._handle, wrapper, None)
        else:
            self._callbacks.pop("log", None)
            self._lib.ITSCAM_Client_onLog(self._handle, None, None)
    
    # =========================================================================
    #  Private Helpers
    # =========================================================================
    
    def _set_capture_callback(
        self,
        name: str,
        callback: Optional[Callable[[CaptureResult], None]],
        native_setter: Any
    ) -> None:
        """Set a capture callback."""
        if not self._handle:
            return
        
        if callback:
            @_b.ITSCAM_CaptureCallback
            def wrapper(result_ptr, user_data):
                try:
                    result = self._convert_single_capture_result(result_ptr)
                    callback(result)
                except Exception:
                    pass
            
            self._callbacks[name] = wrapper
            native_setter(self._handle, wrapper, None)
        else:
            self._callbacks.pop(name, None)
            native_setter(self._handle, None, None)
    
    def _convert_capture_results(self, array) -> List[CaptureResult]:
        """Convert native capture result array to Python objects."""
        results = []
        size = self._lib.ITSCAM_CaptureResultArray_size(array)
        for i in range(size):
            result_ptr = self._lib.ITSCAM_CaptureResultArray_get(array, i)
            if result_ptr:
                results.append(self._convert_single_capture_result(result_ptr))
        return results
    
    def _convert_single_capture_result(self, result_ptr) -> CaptureResult:
        """Convert a single native capture result to Python object."""
        # Get frame info
        native_info = self._lib.ITSCAM_CaptureResult_getInfo(result_ptr)
        
        # Get plates
        plates = []
        plate_count = self._lib.ITSCAM_CaptureResult_getPlateCount(result_ptr)
        for i in range(plate_count):
            plate = self._lib.ITSCAM_CaptureResult_getPlate(result_ptr, i)
            if plate:
                plates.append(plate.decode("utf-8"))
        
        info = FrameInfo(
            request_id=native_info.requestId,
            scenario=native_info.scenario,
            multi_exp_index=native_info.multiExpIndex,
            multi_exp_length=native_info.multiExpLength,
            timestamp_us=native_info.timestampUs,
            width=native_info.width,
            height=native_info.height,
            plates=plates
        )
        
        # Get JPEG data
        jpeg_size = c_size_t()
        jpeg_ptr = self._lib.ITSCAM_CaptureResult_getJpeg(result_ptr, byref(jpeg_size))
        jpeg = bytes(jpeg_ptr[:jpeg_size.value]) if jpeg_ptr and jpeg_size.value > 0 else b""
        
        return CaptureResult(info=info, jpeg=jpeg)
