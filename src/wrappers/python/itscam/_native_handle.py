"""A client's native handle, shared by its calls (internal)."""

import threading
from contextlib import contextmanager


class NativeHandle:
    """A client's native handle, shared by its calls.

    close() may run while another thread is still in a native call of the
    client, or in one of its callbacks: the handle is then destroyed once the
    last such call returns.
    """

    def __init__(self, handle, destroy):
        self._lock = threading.Lock()
        self._handle = handle
        self._destroy = destroy
        self._users = 0
        self._closed = False

    @contextmanager
    def use(self):
        """Holds the handle for a native call; yields None once closed, so
        that the native call reports its usual null-handle error."""
        with self._lock:
            if self._closed:
                handle = None
            else:
                self._users += 1
                handle = self._handle
        if handle is None:
            yield None
            return
        try:
            yield handle
        finally:
            self._release(deferred=True)

    def close(self, before_destroy=None):
        """Marks the client closed and calls before_destroy(handle), once.
        The handle is destroyed here if no call holds it, otherwise when the
        last one returns."""
        with self._lock:
            if self._closed:
                return
            self._closed = True
            self._users += 1
            handle = self._handle
        try:
            if before_destroy is not None:
                before_destroy(handle)
        finally:
            self._release(deferred=False)

    def _release(self, deferred):
        with self._lock:
            self._users -= 1
            if self._users > 0 or not self._closed:
                return
            handle, self._handle = self._handle, None
        if deferred:
            # The last call may be a callback, or run on a thread that a
            # callback waits for: destroying waits for the callback in
            # progress
            threading.Thread(target=self._destroy, args=(handle,),
                             daemon=True).start()
        else:
            self._destroy(handle)
