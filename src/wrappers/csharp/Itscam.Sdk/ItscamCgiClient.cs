// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Pumatronix.Itscam.Native;

namespace Pumatronix.Itscam
{
    /// <summary>
    /// .NET wrapper for the legacy ITSCAM CGI endpoints (snapshot.cgi,
    /// lastframe.cgi, mjpegvideo.cgi, reboot.cgi).  Supports both HTTP
    /// and HTTPS through the statically-linked mbedTLS backend.
    /// </summary>
    public sealed class ItscamCgiClient : IDisposable
    {
        private IntPtr _handle;
        private bool _disposed;

        // Hold the delegate to prevent garbage collection while the
        // native stream worker is calling back into managed code.
        private NativeMethods.NativeStreamCallback _streamDelegate;

        /// <summary>
        /// Raised when a new MJPEG frame arrives.  Handlers run on the
        /// native stream worker thread; do not block.
        /// </summary>
        public event EventHandler<CgiStreamFrame> MjpegFrame;

        public ItscamCgiClient()
        {
            _handle = NativeMethods.Cgi_create();
            if (_handle == IntPtr.Zero)
                throw new ItscamException(ItscamErrorCode.AllocationFailed,
                    "Failed to allocate native CGI client.");
        }

        // ====================================================================
        //  Configuration
        // ====================================================================

        public void SetBaseUrl(string host, ushort port = 80,
                               string scheme = "http")
        {
            ThrowIfDisposed();
            if (host == null) throw new ArgumentNullException(nameof(host));
            var rc = NativeMethods.Cgi_setBaseUrl(_handle, host, port,
                                                   scheme ?? "http");
            ItscamException.ThrowIfFailed(rc, "Cgi_setBaseUrl");
        }

        public void SetApiPrefix(string prefix)
        {
            ThrowIfDisposed();
            NativeMethods.Cgi_setApiPrefix(_handle, prefix ?? "/api");
        }

        public void SetCaCertFile(string pemPath) {
            ThrowIfDisposed();
            NativeMethods.Cgi_setCaCertFile(_handle, pemPath ?? string.Empty);
        }

        public void SetCaCertData(string pem) {
            ThrowIfDisposed();
            NativeMethods.Cgi_setCaCertData(_handle, pem ?? string.Empty);
        }

        public void SetVerifyServerCertificate(bool verify) {
            ThrowIfDisposed();
            NativeMethods.Cgi_setVerifyServerCertificate(_handle,
                                                          verify ? 1 : 0);
        }

        public void SetClientCertificate(string certPem, string keyPem) {
            ThrowIfDisposed();
            NativeMethods.Cgi_setClientCertificate(_handle,
                certPem ?? string.Empty, keyPem ?? string.Empty);
        }

        // ====================================================================
        //  Authentication
        // ====================================================================

        public Task LoginAsync(string username, string password,
                               uint timeoutMs = 10000)
        {
            ThrowIfDisposed();
            if (username == null) throw new ArgumentNullException(nameof(username));
            if (password == null) throw new ArgumentNullException(nameof(password));
            return Task.Run(() =>
            {
                var rc = NativeMethods.Cgi_login(_handle, username, password,
                                                  timeoutMs);
                ItscamException.ThrowIfFailed(rc, "Cgi.login");
            });
        }

        public void SetAuthToken(string token) {
            ThrowIfDisposed();
            NativeMethods.Cgi_setAuthToken(_handle, token ?? string.Empty);
        }

        public void ClearAuthToken() {
            ThrowIfDisposed();
            NativeMethods.Cgi_clearAuthToken(_handle);
        }

        // ====================================================================
        //  /api/lastframe.cgi
        // ====================================================================

        public Task<CgiImage> GetLastFrameAsync(uint timeoutMs = 10000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.Cgi_getLastFrame(_handle, timeoutMs,
                                                        out var imgPtr);
                if (rc != NativeErrorCode.Ok)
                {
                    if (imgPtr != IntPtr.Zero) NativeMethods.CgiImage_destroy(imgPtr);
                    ItscamException.ThrowIfFailed(rc, "Cgi.getLastFrame");
                }
                try { return ToManaged(imgPtr); }
                finally { NativeMethods.CgiImage_destroy(imgPtr); }
            });
        }

        // ====================================================================
        //  /api/snapshot.cgi
        // ====================================================================

        public Task<IReadOnlyList<CgiImage>> GetSnapshotAsync(
            SnapshotCgiRequest request, uint timeoutMs = 15000)
        {
            ThrowIfDisposed();
            if (request == null) throw new ArgumentNullException(nameof(request));
            return Task.Run(() =>
            {
                using var pinned = SnapshotRequestPinner.Pin(request);
                var nativeReq = pinned.Native;
                var rc = NativeMethods.Cgi_getSnapshot(_handle, ref nativeReq,
                                                       timeoutMs, out var arrPtr);
                if (rc != NativeErrorCode.Ok)
                {
                    if (arrPtr != IntPtr.Zero) NativeMethods.CgiImageArray_destroy(arrPtr);
                    ItscamException.ThrowIfFailed(rc, "Cgi.getSnapshot");
                }
                try
                {
                    int n = (int)NativeMethods.CgiImageArray_size(arrPtr);
                    var list = new List<CgiImage>(n);
                    for (int i = 0; i < n; ++i)
                    {
                        var imgPtr = NativeMethods.CgiImageArray_get(arrPtr,
                            (UIntPtr)i);
                        list.Add(ToManaged(imgPtr));
                    }
                    return (IReadOnlyList<CgiImage>)list;
                }
                finally
                {
                    NativeMethods.CgiImageArray_destroy(arrPtr);
                }
            });
        }

        // ====================================================================
        //  /api/mjpegvideo.cgi
        // ====================================================================

        /// <summary>
        /// Begin streaming MJPEG frames.  Subscribers to <see cref="MjpegFrame"/>
        /// receive each frame on the native worker thread.
        /// </summary>
        public void StartMjpegStream(uint timeoutMs = 10000)
        {
            ThrowIfDisposed();
            if (NativeMethods.Cgi_isMjpegStreamRunning(_handle) != 0)
                throw new InvalidOperationException(
                    "MJPEG stream already running.");

            // Capture the delegate in a field to prevent GC.
            _streamDelegate = OnNativeStreamFrame;
            var rc = NativeMethods.Cgi_startMjpegStream(_handle,
                _streamDelegate, IntPtr.Zero, timeoutMs);
            if (rc != NativeErrorCode.Ok)
            {
                _streamDelegate = null;
                ItscamException.ThrowIfFailed(rc, "Cgi.startMjpegStream");
            }
        }

        public void StopMjpegStream()
        {
            if (_disposed) return;
            NativeMethods.Cgi_stopMjpegStream(_handle);
            _streamDelegate = null;
        }

        public bool IsMjpegStreamRunning
        {
            get
            {
                ThrowIfDisposed();
                return NativeMethods.Cgi_isMjpegStreamRunning(_handle) != 0;
            }
        }

        private void OnNativeStreamFrame(IntPtr framePtr, IntPtr userData)
        {
            if (framePtr == IntPtr.Zero) return;
            var native = Marshal.PtrToStructure<NativeMethods.NativeStreamFrame>(framePtr);
            byte[] data = NativeMethods.CopyBytes(native.data,
                (int)native.dataLen);
            var frame = new CgiStreamFrame(native.sequence,
                NativeMethods.PtrToStringUtf8(native.mimeType), data);
            MjpegFrame?.Invoke(this, frame);
        }

        // ====================================================================
        //  /api/trigger.cgi force
        //  /api/reboot.cgi
        // ====================================================================

        public Task<string> ForceTriggerAsync(uint timeoutMs = 10000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.Cgi_forceTrigger(_handle, timeoutMs,
                    out var ptr);
                if (rc != NativeErrorCode.Ok)
                {
                    if (ptr != IntPtr.Zero) NativeMethods.String_destroy(ptr);
                    ItscamException.ThrowIfFailed(rc, "Cgi.forceTrigger");
                }
                return NativeMethods.TakeString(ptr);
            });
        }

        public Task<string> RebootAsync(uint timeoutMs = 10000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.Cgi_reboot(_handle, timeoutMs,
                    out var ptr);
                if (rc != NativeErrorCode.Ok)
                {
                    if (ptr != IntPtr.Zero) NativeMethods.String_destroy(ptr);
                    ItscamException.ThrowIfFailed(rc, "Cgi.reboot");
                }
                return NativeMethods.TakeString(ptr);
            });
        }

        // ====================================================================
        //  Lifecycle
        // ====================================================================

        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            if (_handle != IntPtr.Zero)
            {
                NativeMethods.Cgi_stopMjpegStream(_handle);
                NativeMethods.Cgi_destroy(_handle);
                _handle = IntPtr.Zero;
            }
            _streamDelegate = null;
            GC.SuppressFinalize(this);
        }

        ~ItscamCgiClient() { Dispose(); }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(ItscamCgiClient));
        }

        // ====================================================================
        //  Helpers
        // ====================================================================

        private static CgiImage ToManaged(IntPtr imgPtr)
        {
            if (imgPtr == IntPtr.Zero)
                return new CgiImage(string.Empty, Array.Empty<byte>());
            var mime = NativeMethods.PtrToStringUtf8(
                NativeMethods.CgiImage_mimeType(imgPtr));
            var size = (int)NativeMethods.CgiImage_size(imgPtr);
            var data = NativeMethods.CopyBytes(
                NativeMethods.CgiImage_data(imgPtr), size);
            return new CgiImage(mime, data);
        }
    }

    /// <summary>
    /// Pins all managed objects referenced by a SnapshotCgiRequest so the
    /// native side can read them safely for the duration of one call.
    /// </summary>
    internal sealed class SnapshotRequestPinner : IDisposable
    {
        public NativeMethods.NativeSnapshotRequest Native;

        private GCHandle _shuttersHandle;
        private GCHandle _gainsHandle;
        private IntPtr _formatPtr;
        private IntPtr _cropPtr;
        private IntPtr _textPtr;
        private IntPtr[] _keyPtrs;
        private IntPtr[] _valPtrs;
        private GCHandle _keysHandle;
        private GCHandle _valsHandle;

        public static SnapshotRequestPinner Pin(SnapshotCgiRequest req)
        {
            var p = new SnapshotRequestPinner();
            p.Build(req);
            return p;
        }

        private void Build(SnapshotCgiRequest req)
        {
            Native = default;
            Native.quality  = req.Quality;
            Native.mosaic   = req.Mosaic ? 1 : 0;
            Native.scenario = req.Scenario;

            if (req.Shutters != null && req.Shutters.Count > 0)
            {
                var arr = new int[req.Shutters.Count];
                for (int i = 0; i < arr.Length; ++i) arr[i] = req.Shutters[i];
                _shuttersHandle = GCHandle.Alloc(arr, GCHandleType.Pinned);
                Native.shutters = _shuttersHandle.AddrOfPinnedObject();
                Native.shuttersLen = (UIntPtr)arr.Length;
            }
            if (req.Gains != null && req.Gains.Count > 0)
            {
                var arr = new int[req.Gains.Count];
                for (int i = 0; i < arr.Length; ++i) arr[i] = req.Gains[i];
                _gainsHandle = GCHandle.Alloc(arr, GCHandleType.Pinned);
                Native.gains = _gainsHandle.AddrOfPinnedObject();
                Native.gainsLen = (UIntPtr)arr.Length;
            }

            _formatPtr = NativeMethods.AllocUtf8(req.Format);
            _cropPtr   = NativeMethods.AllocUtf8(req.Crop);
            _textPtr   = NativeMethods.AllocUtf8(req.TextOverlay);
            Native.format      = _formatPtr;
            Native.crop        = _cropPtr;
            Native.textOverlay = _textPtr;

            if (req.UserMetadata != null && req.UserMetadata.Count > 0)
            {
                var n = req.UserMetadata.Count;
                _keyPtrs = new IntPtr[n + 1];
                _valPtrs = new IntPtr[n + 1];
                int i = 0;
                foreach (var kv in req.UserMetadata)
                {
                    _keyPtrs[i] = NativeMethods.AllocUtf8(kv.Key);
                    _valPtrs[i] = NativeMethods.AllocUtf8(kv.Value);
                    ++i;
                }
                _keyPtrs[n] = IntPtr.Zero;
                _valPtrs[n] = IntPtr.Zero;
                _keysHandle = GCHandle.Alloc(_keyPtrs, GCHandleType.Pinned);
                _valsHandle = GCHandle.Alloc(_valPtrs, GCHandleType.Pinned);
                Native.userMetadataKeys   = _keysHandle.AddrOfPinnedObject();
                Native.userMetadataValues = _valsHandle.AddrOfPinnedObject();
            }
        }

        public void Dispose()
        {
            if (_shuttersHandle.IsAllocated) _shuttersHandle.Free();
            if (_gainsHandle.IsAllocated)    _gainsHandle.Free();
            NativeMethods.FreeUtf8(_formatPtr);
            NativeMethods.FreeUtf8(_cropPtr);
            NativeMethods.FreeUtf8(_textPtr);
            if (_keyPtrs != null)
                foreach (var p in _keyPtrs) NativeMethods.FreeUtf8(p);
            if (_valPtrs != null)
                foreach (var p in _valPtrs) NativeMethods.FreeUtf8(p);
            if (_keysHandle.IsAllocated) _keysHandle.Free();
            if (_valsHandle.IsAllocated) _valsHandle.Free();
        }
    }
}
