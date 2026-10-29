"""
ITSCAM SDK - CGI client (Python)

Pythonic wrapper around the C CGI client.  Supports snapshot.cgi
(including multipart multi-exposure responses), lastframe.cgi,
mjpegvideo.cgi (callback-based streaming) and reboot.cgi.

Example usage::

    from itscam import ItscamCgiClient, SnapshotCgiRequest

    with ItscamCgiClient() as cgi:
        cgi.set_base_url("camera.example.com", 443, "https")
        cgi.set_verify_server_certificate(False)
        cgi.login("admin", "password")

        images = cgi.get_snapshot(SnapshotCgiRequest(quality=80))
        for i, img in enumerate(images):
            with open(f"snap-{i}.jpg", "wb") as f:
                f.write(img.data)

        def on_frame(frame):
            print("got frame", frame.sequence, len(frame.data), "bytes")

        cgi.start_mjpeg_stream(on_frame)
        import time; time.sleep(5)
        cgi.stop_mjpeg_stream()

Copyright (c) 2026 Pumatronix
"""
from __future__ import annotations

import ctypes
import threading
import weakref
from ctypes import POINTER, byref, c_char_p, c_int32
from dataclasses import dataclass, field
from typing import Callable, Dict, List, Optional

from . import bindings
from ._native_handle import NativeHandle
from .rest_client import _c_str, _take_string
from .types import _raise_for_error


# ============================================================================
#  Public data classes
# ============================================================================

@dataclass
class SnapshotCgiRequest:
    """Parameters for ``ItscamCgiClient.get_snapshot``."""
    shutters: List[int] = field(default_factory=list)
    gains: List[int] = field(default_factory=list)
    quality: int = -1
    mosaic: bool = False
    format: str = ""
    scenario: int = -1
    crop: str = ""
    text_overlay: str = ""
    user_metadata: Dict[str, str] = field(default_factory=dict)


@dataclass
class CgiImage:
    """Single image returned by ``lastframe.cgi`` or ``snapshot.cgi``."""
    mime_type: str
    data: bytes


@dataclass
class CgiStreamFrame:
    """Single MJPEG frame delivered to a stream callback."""
    sequence: int
    mime_type: str
    data: bytes


# ============================================================================
#  Helpers
# ============================================================================

def _consume_image(img_handle) -> CgiImage:
    """Read an ITSCAM_CgiImage*, freeing it, and return a managed CgiImage."""
    lib = bindings.get_lib()
    try:
        mime = lib.ITSCAM_CgiImage_mimeType(img_handle)
        size = lib.ITSCAM_CgiImage_size(img_handle)
        ptr  = lib.ITSCAM_CgiImage_data(img_handle)
        if ptr and size:
            buf = (ctypes.c_ubyte * size).from_address(
                ctypes.addressof(ptr.contents))
            data = bytes(buf)
        else:
            data = b""
        return CgiImage(mime_type=(mime.decode("utf-8") if mime else ""),
                        data=data)
    finally:
        lib.ITSCAM_CgiImage_destroy(img_handle)


def _borrow_image(img_handle) -> CgiImage:
    """Read an ITSCAM_CgiImage* without freeing it (for array entries)."""
    lib = bindings.get_lib()
    mime = lib.ITSCAM_CgiImage_mimeType(img_handle)
    size = lib.ITSCAM_CgiImage_size(img_handle)
    ptr  = lib.ITSCAM_CgiImage_data(img_handle)
    if ptr and size:
        buf = (ctypes.c_ubyte * size).from_address(
            ctypes.addressof(ptr.contents))
        data = bytes(buf)
    else:
        data = b""
    return CgiImage(mime_type=(mime.decode("utf-8") if mime else ""),
                    data=data)


# ============================================================================
#  Main client
# ============================================================================

StreamCallback = Callable[[CgiStreamFrame], None]


class ItscamCgiClient:
    """Pythonic wrapper for the ITSCAM CGI endpoints."""

    def __init__(self) -> None:
        self._lib = bindings.get_lib()
        handle = self._lib.ITSCAM_CgiClient_create()
        if not handle:
            raise MemoryError("ITSCAM_CgiClient_create returned NULL")
        self._mjpeg_user_cb: Optional[StreamCallback] = None
        # Guards the two above and below.  Never held across a native stop,
        # which waits for the frame callback, which may call this client.
        self._stream_lock = threading.Lock()
        self._stops_in_progress = 0
        # One ctypes callback for the client's whole life: a frame callback
        # may stop the stream or close the client while it runs.  It refers
        # to the client weakly, so that it does not keep it alive.
        client_ref = weakref.ref(self)

        def _trampoline(frame_ptr, _user_data):
            client = client_ref()
            if client is None or not frame_ptr:
                return
            # Holds the handle while the frame callback runs, and while it
            # drops the client: a close() from either leaves the destroy to
            # another thread, which waits for this callback before releasing
            # it
            with client._native.use() as h:
                if h is not None:
                    client._on_mjpeg_frame(frame_ptr.contents)
                del client

        self._mjpeg_native_cb = bindings.ITSCAM_CgiStreamCallback(_trampoline)

        # Keeps the stream callback until no frame callback can run any more
        def destroy(h, lib=self._lib, stream_cb=self._mjpeg_native_cb):
            lib.ITSCAM_CgiClient_destroy(h)

        self._native = NativeHandle(handle, destroy)

    # ----- context manager ---------------------------------------------------

    def __enter__(self) -> "ItscamCgiClient":
        return self

    def __exit__(self, *exc_info) -> None:
        self.close()

    def close(self) -> None:
        """Stop the stream and close the client.  Calls still running on
        other threads end first: the last of them releases the native
        client."""
        native = getattr(self, "_native", None)
        if native is None:
            return
        native.close(self._stop_stream)

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass

    # ----- configuration -----------------------------------------------------

    def set_base_url(self, host: str, port: int = 80,
                     scheme: str = "http") -> None:
        with self._native.use() as h:
            rc = self._lib.ITSCAM_CgiClient_setBaseUrl(
                h, _c_str(host), port, _c_str(scheme))
            _raise_for_error(rc, f"set_base_url({host}:{port})")

    def set_api_prefix(self, prefix: str) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_setApiPrefix(
                h, _c_str(prefix))

    def set_ca_cert_file(self, pem_path: str) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_setCaCertFile(
                h, _c_str(pem_path))

    def set_ca_cert_data(self, pem: str) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_setCaCertData(
                h, _c_str(pem))

    def set_verify_server_certificate(self, verify: bool) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_setVerifyServerCertificate(
                h, 1 if verify else 0)

    def set_client_certificate(self, cert_pem: str, key_pem: str) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_setClientCertificate(
                h, _c_str(cert_pem), _c_str(key_pem))

    # ----- authentication ----------------------------------------------------

    def login(self, username: str, password: str,
              timeout_ms: int = 10000) -> None:
        with self._native.use() as h:
            rc = self._lib.ITSCAM_CgiClient_login(
                h, _c_str(username), _c_str(password), timeout_ms)
            _raise_for_error(rc, "login")

    def set_auth_token(self, token: str) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_setAuthToken(
                h, _c_str(token))

    def clear_auth_token(self) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_clearAuthToken(h)

    def set_basic_auth(self, user: str, password: str) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_setBasicAuth(
                h, _c_str(user), _c_str(password))

    def clear_basic_auth(self) -> None:
        with self._native.use() as h:
            self._lib.ITSCAM_CgiClient_clearBasicAuth(h)

    # ----- /api/lastframe.cgi ------------------------------------------------

    def get_last_frame(self, timeout_ms: int = 10000) -> CgiImage:
        with self._native.use() as h:
            out = bindings.ITSCAM_CgiImage()
            rc = self._lib.ITSCAM_CgiClient_getLastFrame(
                h, timeout_ms, byref(out))
            _raise_for_error(rc, "get_last_frame")
            return _consume_image(out)

    # ----- /api/snapshot.cgi -------------------------------------------------

    def get_snapshot(self, request: Optional[SnapshotCgiRequest] = None,
                     timeout_ms: int = 15000) -> List[CgiImage]:
        with self._native.use() as h:
            request = request or SnapshotCgiRequest()
            native = bindings.ITSCAM_CgiSnapshotRequest()
            native.quality = request.quality
            native.mosaic  = 1 if request.mosaic else 0
            native.scenario = request.scenario
            native.format = request.format.encode("utf-8") if request.format else None
            native.crop = request.crop.encode("utf-8") if request.crop else None
            native.textOverlay = request.text_overlay.encode("utf-8") if request.text_overlay else None

            shutters_arr = None
            gains_arr = None
            if request.shutters:
                shutters_arr = (c_int32 * len(request.shutters))(*request.shutters)
                native.shutters = shutters_arr
                native.shuttersLen = len(request.shutters)
            if request.gains:
                gains_arr = (c_int32 * len(request.gains))(*request.gains)
                native.gains = gains_arr
                native.gainsLen = len(request.gains)

            # Keep references to the byte strings alive for the duration of the
            # FFI call (otherwise the GC may reclaim them while C is reading).
            keys_bufs: List[bytes] = []
            vals_bufs: List[bytes] = []
            if request.user_metadata:
                for k, v in request.user_metadata.items():
                    keys_bufs.append(k.encode("utf-8"))
                    vals_bufs.append(v.encode("utf-8"))
                keys_arr = (c_char_p * (len(keys_bufs) + 1))(
                    *keys_bufs, None)
                vals_arr = (c_char_p * (len(vals_bufs) + 1))(
                    *vals_bufs, None)
                native.userMetadataKeys   = keys_arr
                native.userMetadataValues = vals_arr
            else:
                keys_arr = None
                vals_arr = None

            out = bindings.ITSCAM_CgiImageArray()
            rc = self._lib.ITSCAM_CgiClient_getSnapshot(
                h, byref(native), timeout_ms, byref(out))
            _raise_for_error(rc, "get_snapshot")
            try:
                n = self._lib.ITSCAM_CgiImageArray_size(out)
                return [_borrow_image(self._lib.ITSCAM_CgiImageArray_get(out, i))
                        for i in range(n)]
            finally:
                self._lib.ITSCAM_CgiImageArray_destroy(out)

    # ----- /api/mjpegvideo.cgi -----------------------------------------------

    def start_mjpeg_stream(self, callback: StreamCallback,
                           timeout_ms: int = 10000) -> None:
        """Begin streaming MJPEG frames.  ``callback`` is invoked from the
        SDK's worker thread for every frame.  Use a queue (or run a UI
        toolkit's marshalling primitive) if you need to interact with
        non-thread-safe state.  The callback may stop or restart the
        stream, or close the client."""
        with self._native.use() as h, self._stream_lock:
            if self._mjpeg_user_cb is not None:
                raise RuntimeError("mjpeg stream already running")
            # A stop in progress would stop the new stream
            if self._stops_in_progress:
                raise RuntimeError("mjpeg stream still stopping")
            self._mjpeg_user_cb = callback
            # Never waits for a frame callback
            rc = self._lib.ITSCAM_CgiClient_startMjpegStream(
                h, self._mjpeg_native_cb, None, timeout_ms)
            if rc != 0:
                self._mjpeg_user_cb = None
                _raise_for_error(rc, "start_mjpeg_stream")

    def _on_mjpeg_frame(self, frame) -> None:
        callback = self._mjpeg_user_cb
        if callback is None:
            return
        mime = frame.mimeType.decode("utf-8") if frame.mimeType else ""
        if frame.data and frame.dataLen:
            buf = (ctypes.c_ubyte * frame.dataLen).from_address(
                ctypes.addressof(frame.data.contents))
            data = bytes(buf)
        else:
            data = b""
        try:
            callback(CgiStreamFrame(
                sequence=frame.sequence,
                mime_type=mime,
                data=data))
        except Exception:
            # Never let a Python exception cross into native code.
            import traceback
            traceback.print_exc()

    def stop_mjpeg_stream(self) -> None:
        """Stop the stream.  From another thread, this waits for the frame
        callback in progress; from the frame callback, it returns at
        once."""
        with self._native.use() as h:
            if not h:
                return
            self._stop_stream(h)

    def _stop_stream(self, h) -> None:
        with self._stream_lock:
            self._mjpeg_user_cb = None
            self._stops_in_progress += 1
        # Concurrent stops all wait: the native stop returns once no frame
        # callback runs, whichever stop ended the stream
        try:
            self._lib.ITSCAM_CgiClient_stopMjpegStream(h)
        finally:
            with self._stream_lock:
                self._stops_in_progress -= 1

    def is_mjpeg_stream_running(self) -> bool:
        with self._native.use() as h:
            return bool(self._lib.ITSCAM_CgiClient_isMjpegStreamRunning(
                h))

    # ----- /api/trigger.cgi force / /api/reboot.cgi --------------------------

    def force_trigger(self, timeout_ms: int = 10000) -> str:
        with self._native.use() as h:
            out = bindings.ITSCAM_String()
            rc = self._lib.ITSCAM_CgiClient_forceTrigger(
                h, timeout_ms, byref(out))
            body = _take_string(out)
            _raise_for_error(rc, "force_trigger")
            return body

    def reboot(self, timeout_ms: int = 10000) -> str:
        with self._native.use() as h:
            out = bindings.ITSCAM_String()
            rc = self._lib.ITSCAM_CgiClient_reboot(
                h, timeout_ms, byref(out))
            body = _take_string(out)
            _raise_for_error(rc, "reboot")
            return body
