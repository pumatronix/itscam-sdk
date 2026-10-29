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
        private readonly ClientHandle _native;

        // Hold the delegate to prevent garbage collection while the
        // native stream worker is calling back into managed code.  It lives
        // as long as the client: a handler may stop the stream or dispose
        // the client while it runs.
        private readonly NativeMethods.NativeStreamCallback _streamDelegate;

        /// <summary>
        /// Raised when a new MJPEG frame arrives.  Handlers run on the
        /// native stream worker thread; do not block.  A handler may stop
        /// or restart the stream, or dispose the client.
        /// </summary>
        public event EventHandler<CgiStreamFrame> MjpegFrame;

        public ItscamCgiClient()
        {
            _streamDelegate = OnNativeStreamFrame;
            var handle = NativeMethods.Cgi_create();
            if (handle == IntPtr.Zero)
                throw new ItscamException(ItscamErrorCode.AllocationFailed,
                    "Failed to allocate native CGI client.");
            // The destroy may be left to the last call in progress, after
            // this client is collected: it keeps the stream delegate until
            // no frame callback can run
            var streamDelegate = _streamDelegate;
            _native = new ClientHandle(handle, nameof(ItscamCgiClient), h =>
            {
                NativeMethods.Cgi_destroy(h);
                GC.KeepAlive(streamDelegate);
            });
        }

        // ====================================================================
        //  Configuration
        // ====================================================================

        public void SetBaseUrl(string host, ushort port = 80,
                               string scheme = "http")
        {
            if (host == null) throw new ArgumentNullException(nameof(host));
            var rc = _native.Use(h => NativeMethods.Cgi_setBaseUrl(h, host,
                port, scheme ?? "http"));
            ItscamException.ThrowIfFailed(rc, "Cgi_setBaseUrl");
        }

        public void SetApiPrefix(string prefix)
        {
            _native.Use(h => NativeMethods.Cgi_setApiPrefix(h, prefix ?? "/api"));
        }

        public void SetCaCertFile(string pemPath) {
            _native.Use(h => NativeMethods.Cgi_setCaCertFile(h,
                pemPath ?? string.Empty));
        }

        public void SetCaCertData(string pem) {
            _native.Use(h => NativeMethods.Cgi_setCaCertData(h,
                pem ?? string.Empty));
        }

        public void SetVerifyServerCertificate(bool verify) {
            _native.Use(h => NativeMethods.Cgi_setVerifyServerCertificate(h,
                verify ? 1 : 0));
        }

        public void SetClientCertificate(string certPem, string keyPem) {
            _native.Use(h => NativeMethods.Cgi_setClientCertificate(h,
                certPem ?? string.Empty, keyPem ?? string.Empty));
        }

        // ====================================================================
        //  Authentication
        // ====================================================================

        public Task LoginAsync(string username, string password,
                               uint timeoutMs = 10000)
        {
            if (username == null) throw new ArgumentNullException(nameof(username));
            if (password == null) throw new ArgumentNullException(nameof(password));
            return _native.UseAsync(h =>
            {
                var rc = NativeMethods.Cgi_login(h, username, password,
                                                  timeoutMs);
                ItscamException.ThrowIfFailed(rc, "Cgi.login");
            });
        }

        public void SetAuthToken(string token) {
            _native.Use(h => NativeMethods.Cgi_setAuthToken(h,
                token ?? string.Empty));
        }

        public void ClearAuthToken() {
            _native.Use(h => NativeMethods.Cgi_clearAuthToken(h));
        }

        // ====================================================================
        //  /api/lastframe.cgi
        // ====================================================================

        public Task<CgiImage> GetLastFrameAsync(uint timeoutMs = 10000)
        {
            return _native.UseAsync(h =>
            {
                var rc = NativeMethods.Cgi_getLastFrame(h, timeoutMs,
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
            if (request == null) throw new ArgumentNullException(nameof(request));
            return _native.UseAsync(h =>
            {
                using var pinned = SnapshotRequestPinner.Pin(request);
                var nativeReq = pinned.Native;
                var rc = NativeMethods.Cgi_getSnapshot(h, ref nativeReq,
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
            _native.Use(h =>
            {
                if (NativeMethods.Cgi_isMjpegStreamRunning(h) != 0)
                    throw new InvalidOperationException(
                        "MJPEG stream already running.");

                var rc = NativeMethods.Cgi_startMjpegStream(h,
                    _streamDelegate, IntPtr.Zero, timeoutMs);
                ItscamException.ThrowIfFailed(rc, "Cgi.startMjpegStream");
            });
        }

        /// <summary>
        /// Stop the stream.  From another thread, this waits for the
        /// handler in progress; from a handler, it returns at once.
        /// </summary>
        public void StopMjpegStream()
        {
            _native.UseIfOpen(h => NativeMethods.Cgi_stopMjpegStream(h));
        }

        public bool IsMjpegStreamRunning
        {
            get
            {
                return _native.Use(h =>
                    NativeMethods.Cgi_isMjpegStreamRunning(h) != 0);
            }
        }

        private void OnNativeStreamFrame(IntPtr framePtr, IntPtr userData)
        {
            // A start racing Dispose may run the stream until the deferred
            // destroy: no handler runs once disposed
            if (framePtr == IntPtr.Zero || _native.IsClosed) return;
            var native = Marshal.PtrToStructure<NativeMethods.NativeStreamFrame>(framePtr);
            byte[] data = NativeMethods.CopyBytes(native.data,
                (int)native.dataLen);
            var frame = new CgiStreamFrame(native.sequence,
                NativeMethods.PtrToStringUtf8(native.mimeType), data);
            MjpegFrame?.Invoke(this, frame);
            // The handler may have disposed the client: keep the delegate
            // until the native call returns
            GC.KeepAlive(this);
        }

        // ====================================================================
        //  /api/trigger.cgi force
        //  /api/reboot.cgi
        // ====================================================================

        public Task<string> ForceTriggerAsync(uint timeoutMs = 10000)
        {
            return _native.UseAsync(h =>
            {
                var rc = NativeMethods.Cgi_forceTrigger(h, timeoutMs,
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
            return _native.UseAsync(h =>
            {
                var rc = NativeMethods.Cgi_reboot(h, timeoutMs,
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

        /// <summary>
        /// Stop the stream and release the client.  Calls still running
        /// end normally, and the native client is released after the last
        /// one.
        /// </summary>
        public void Dispose()
        {
            // Null if the constructor failed
            if (_native == null
                || !_native.Close(h => NativeMethods.Cgi_stopMjpegStream(h)))
                return;
            GC.SuppressFinalize(this);
        }

        ~ItscamCgiClient() { Dispose(); }

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
