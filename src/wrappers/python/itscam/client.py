"""
ITSCAM SDK - Python Client

High-level Pythonic wrapper for the ITSCAM camera client.
"""

from ctypes import byref, c_uint32, c_size_t, POINTER, c_ubyte, cast, c_char_p
from typing import Optional, List, Callable, Any
import threading
import weakref

from . import bindings as _b
from ._native_handle import NativeHandle
from .types import (
    Timestamp, FrameInfo, CaptureResult, ProfileInfo,
    AutoReconnectConfig, EventSubscription, CaptureSubscriptionConfig,
    SnapshotRequest,
    ConnectionState, LogLevel, ErrorCode,
    _raise_for_error,
)


def _decode(text) -> str:
    return text.decode("utf-8") if text else ""


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
        handle = self._lib.ITSCAM_Client_create()
        if not handle:
            raise MemoryError("Failed to create ITSCAM client")
        
        # Store callbacks to prevent garbage collection, until no callback
        # can run any more
        self._callbacks: dict = {}
        self._callbacks_lock = threading.Lock()
        # The handler each native callback calls (see _set_handler())
        self._handlers: dict = {}
        lib, callbacks, handlers = self._lib, self._callbacks, self._handlers

        def destroy(h):
            lib.ITSCAM_Client_destroy(h)
            callbacks.clear()
            handlers.clear()

        self._native = NativeHandle(handle, destroy)
        
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
        """Close and clean up the client.  Calls still running on other
        threads, or the one whose callback calls this, end first: the last
        of them releases the native client."""
        native = getattr(self, "_native", None)
        if native is not None:
            native.close()
    
    @property
    def is_connected(self) -> bool:
        """Check if the client is connected."""
        with self._native.use() as h:
            if h is None:
                return False
            return bool(self._lib.ITSCAM_Client_isConnected(h))
    
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
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            reconnect_cfg = None
            if auto_reconnect:
                reconnect_cfg = _b.ITSCAM_AutoReconnectConfig(
                    enabled=1 if auto_reconnect.enabled else 0,
                    intervalMs=auto_reconnect.interval_ms,
                    maxRetries=auto_reconnect.max_retries
                )
        
            result = self._lib.ITSCAM_Client_connect(
                h,
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
        with self._native.use() as h:
            if h is not None:
                self._lib.ITSCAM_Client_disconnect(h)
    
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
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            result = self._lib.ITSCAM_Client_authenticate(
                h,
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
        with self._native.use() as h:
            if h is None:
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
                h, byref(sub), timeout_ms
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
        with self._native.use() as h:
            if h is None:
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
                h, byref(c_config), timeout_ms
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
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            request = _b.ITSCAM_SnapshotRequest(
                scenario=scenario,
                quality=quality,
                forceCapture=1 if force_capture else 0
            )
        
            result_array = _b.ITSCAM_CaptureResultArray()
        
            result = self._lib.ITSCAM_Client_captureSnapshot(
                h, byref(request), timeout_ms, byref(result_array)
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
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            byte_array = _b.ITSCAM_ByteArray()
        
            result = self._lib.ITSCAM_Client_getLastFrame(
                h, quality, timeout_ms, byref(byte_array)
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
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            profile_id = c_uint32()
            result = self._lib.ITSCAM_Client_getActiveProfileId(
                h, timeout_ms, byref(profile_id)
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
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            result = self._lib.ITSCAM_Client_setActiveProfile(
                h, profile_id, timeout_ms
            )
        
            if result != ErrorCode.OK:
                error_msg = self._lib.ITSCAM_getLastError()
                _raise_for_error(result, error_msg.decode("utf-8") if error_msg else "")
    
    def list_profiles(self, timeout_ms: int = 10000) -> List[ProfileInfo]:
        """List all available profiles."""
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            profile_array = _b.ITSCAM_ProfileArray()
            result = self._lib.ITSCAM_Client_listProfiles(
                h, timeout_ms, byref(profile_array)
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
        with self._native.use() as h:
            if h is None:
                raise RuntimeError("Client has been closed")
        
            result = self._lib.ITSCAM_Client_reboot(h, timeout_ms)
        
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
        self._set_handler("disconnect", callback,
                          _b.ITSCAM_DisconnectCallback,
                          lambda reason: (_decode(reason),),
                          self._lib.ITSCAM_Client_onDisconnect)
    
    def on_connection_state(
        self,
        callback: Optional[Callable[[ConnectionState, str], None]]
    ) -> None:
        """
        Set callback for connection state changes.
        
        Args:
            callback: Callback function receiving state and reason.
        """
        self._set_handler("connection_state", callback,
                          _b.ITSCAM_ConnectionStateCallback,
                          lambda state, reason: (ConnectionState(state),
                                                 _decode(reason)),
                          self._lib.ITSCAM_Client_onConnectionState)
    
    def on_log(
        self,
        callback: Optional[Callable[[LogLevel, str], None]]
    ) -> None:
        """
        Set callback for log messages.
        
        Args:
            callback: Callback function receiving log level and message.
        """
        self._set_handler("log", callback, _b.ITSCAM_LogCallback,
                          lambda level, message: (LogLevel(level),
                                                  _decode(message)),
                          self._lib.ITSCAM_Client_onLog)
    
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
        # Weak: the native callback must not keep the client alive
        client_ref = weakref.ref(self)

        def convert(result_ptr):
            return (client_ref()._convert_single_capture_result(result_ptr),)

        self._set_handler(name, callback, _b.ITSCAM_CaptureCallback, convert,
                          native_setter)
    
    def _set_handler(self, name: str, handler: Optional[Callable],
                     callback_type: Any, convert: Callable,
                     native_setter: Any) -> None:
        """Set the handler of ``name``.  Each callback type has one native
        callback, installed the first time and kept until the native client
        is destroyed, which calls the current handler: installing another
        one could free a callback that an SDK thread is about to call."""
        with self._native.use() as h:
            if h is None:
                return
            if handler is None:
                self._handlers.pop(name, None)
            else:
                self._handlers[name] = handler
            with self._callbacks_lock:
                native_cb = self._callbacks.get(name)
                if native_cb is None:
                    if handler is None:
                        return
                    native_cb = self._native_callback(name, callback_type,
                                                      convert)
                    self._callbacks[name] = native_cb
                    # The first callback of its type: the setter waits for
                    # nothing
                    native_setter(h, native_cb, None)
                    return
            # Setting the same callback again waits for the one in progress:
            # once this returns, the replaced handler is not running (unless
            # this is called from a callback of the client)
            native_setter(h, native_cb, None)
    
    def _native_callback(self, name: str, callback_type: Any,
                         convert: Callable) -> Any:
        handlers, native = self._handlers, self._native

        def call(*args):
            # Holds the handle while the handler runs: a close() from it
            # leaves the destroy to another thread, which waits for this
            # callback before releasing it
            with native.use() as h:
                handler = handlers.get(name)
                if h is None or handler is None:
                    return
                try:
                    handler(*convert(*args[:-1]))  # without user_data
                except Exception:
                    pass  # Don't let exceptions escape to C

        return callback_type(call)
    
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
