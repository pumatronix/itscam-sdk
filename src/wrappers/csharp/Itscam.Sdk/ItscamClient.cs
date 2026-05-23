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
    /// Idiomatic .NET wrapper for the ITSCAM binary client (TCP protocol
    /// on port 60000).  Use this client for low-latency streaming events
    /// (trigger metadata, captures) and on-demand snapshots delivered
    /// directly from the camera-daemon.
    ///
    /// For HTTP/JSON config use <see cref="ItscamRestClient"/>; for
    /// legacy CGI endpoints (snapshot.cgi, lastframe.cgi, mjpegvideo.cgi)
    /// use <see cref="ItscamCgiClient"/>.
    /// </summary>
    public sealed class ItscamClient : IDisposable
    {
        private IntPtr _handle;
        private bool _disposed;

        internal IntPtr NativeHandle
        {
            get
            {
                ThrowIfDisposed();
                return _handle;
            }
        }

        /// <summary>Raised when a trigger capture arrives.  Do not block.</summary>
        public event EventHandler<CaptureResult> TriggerImage;

        /// <summary>Raised when a snapshot capture arrives.  Do not block.</summary>
        public event EventHandler<CaptureResult> SnapshotImage;

        /// <summary>Raised when the TCP connection drops.</summary>
        public event EventHandler<string> Disconnected;

        /// <summary>Raised on connect / disconnect / auto-reconnect.</summary>
        public event EventHandler<ConnectionStateEventArgs> ConnectionStateChanged;

        /// <summary>Raised for SDK log messages from the worker thread.</summary>
        public event EventHandler<LogEventArgs> Log;

        public ItscamClient()
        {
            _handle = NativeMethods.ItscamClient_create();
            if (_handle == IntPtr.Zero)
                throw new ItscamException(ItscamErrorCode.AllocationFailed,
                    "Failed to allocate native ITSCAM client.");
            BinaryClientCallbacks.Attach(this);
        }

        /// <summary>True iff a TCP connection is currently established.</summary>
        public bool IsConnected
        {
            get
            {
                ThrowIfDisposed();
                return NativeMethods.ItscamClient_isConnected(_handle) != 0;
            }
        }

        /// <summary>
        /// Connect to a camera over the binary protocol.  Default port is
        /// 60000.  Pass <paramref name="reconnect"/> to enable auto-reconnect.
        /// </summary>
        public Task ConnectAsync(string host, ushort port = 60000,
                                 uint timeoutMs = 5000,
                                 AutoReconnectConfig reconnect = null)
        {
            ThrowIfDisposed();
            if (host == null) throw new ArgumentNullException(nameof(host));

            return Task.Run(() =>
            {
                NativeErrorCode rc;
                if (reconnect == null)
                {
                    rc = NativeMethods.ItscamClient_connect(
                        _handle, host, port, timeoutMs, IntPtr.Zero);
                }
                else
                {
                    var cfg = new NativeMethods.NativeAutoReconnectConfig
                    {
                        Enabled    = reconnect.Enabled ? 1 : 0,
                        IntervalMs = reconnect.IntervalMs,
                        MaxRetries = reconnect.MaxRetries,
                    };
                    var h = GCHandle.Alloc(cfg, GCHandleType.Pinned);
                    try
                    {
                        rc = NativeMethods.ItscamClient_connect(
                            _handle, host, port, timeoutMs,
                            h.AddrOfPinnedObject());
                    }
                    finally
                    {
                        h.Free();
                    }
                }

                ItscamException.ThrowIfFailed(rc,
                    $"connect to {host}:{port}");
            });
        }

        public void Disconnect()
        {
            if (_disposed) return;
            NativeMethods.ItscamClient_disconnect(_handle);
        }

        public Task AuthenticateAsync(string password, uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            if (password == null) throw new ArgumentNullException(nameof(password));
            return Task.Run(() =>
            {
                var rc = NativeMethods.ItscamClient_authenticate(
                    _handle, password, timeoutMs);
                ItscamException.ThrowIfFailed(rc, "authenticate");
            });
        }

        public Task SubscribeAsync(EventSubscription events,
                                   uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            if (events == null) throw new ArgumentNullException(nameof(events));
            return Task.Run(() =>
            {
                var native = ToNative(events);
                var rc = NativeMethods.ItscamClient_subscribe(
                    _handle, ref native, timeoutMs);
                ItscamException.ThrowIfFailed(rc, "subscribe");
            });
        }

        public Task SubscribeCapturesAsync(
            CaptureSubscriptionConfig config = null,
            uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            if (config == null) config = CaptureSubscriptionConfig.Default;
            return Task.Run(() =>
            {
                var native = ToNative(config);
                var rc = NativeMethods.ItscamClient_subscribeCaptures(
                    _handle, ref native, timeoutMs);
                ItscamException.ThrowIfFailed(rc, "subscribeCaptures");
            });
        }

        /// <summary>
        /// Request a snapshot and return all resulting frames synchronously.
        /// Snapshot callbacks may also fire if subscribed.
        /// </summary>
        public Task<IReadOnlyList<CaptureResult>> CaptureSnapshotAsync(
            uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var request = new NativeMethods.NativeBinarySnapshotRequest();
                var rc = NativeMethods.ItscamClient_captureSnapshot(
                    _handle, ref request, timeoutMs, out var arrayPtr);
                ItscamException.ThrowIfFailed(rc, "captureSnapshot");
                return NativeMethods.TakeCaptureResults(arrayPtr);
            });
        }

        /// <summary>Fetch the last preview frame as JPEG bytes.</summary>
        public Task<byte[]> GetLastFrameAsync(int quality = -1,
                                              uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.ItscamClient_getLastFrame(
                    _handle, quality, timeoutMs, out var arrayPtr);
                ItscamException.ThrowIfFailed(rc, "getLastFrame");
                return NativeMethods.TakeByteArray(arrayPtr);
            });
        }

        public Task<uint> GetActiveProfileIdAsync(uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.ItscamClient_getActiveProfileId(
                    _handle, timeoutMs, out var id);
                ItscamException.ThrowIfFailed(rc, "getActiveProfileId");
                return id;
            });
        }

        public Task SetActiveProfileAsync(uint profileId, uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.ItscamClient_setActiveProfile(
                    _handle, profileId, timeoutMs);
                ItscamException.ThrowIfFailed(rc, "setActiveProfile");
            });
        }

        public Task<IReadOnlyList<ProfileInfo>> ListProfilesAsync(
            uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.ItscamClient_listProfiles(
                    _handle, timeoutMs, out var arrayPtr);
                ItscamException.ThrowIfFailed(rc, "listProfiles");
                return NativeMethods.TakeProfiles(arrayPtr);
            });
        }

        public Task RebootAsync(uint timeoutMs = 5000)
        {
            ThrowIfDisposed();
            return Task.Run(() =>
            {
                var rc = NativeMethods.ItscamClient_reboot(_handle, timeoutMs);
                ItscamException.ThrowIfFailed(rc, "reboot");
            });
        }

        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;

            if (_handle != IntPtr.Zero)
            {
                BinaryClientCallbacks.Detach(_handle);
                NativeMethods.ItscamClient_disconnect(_handle);
                NativeMethods.ItscamClient_destroy(_handle);
                _handle = IntPtr.Zero;
            }

            GC.SuppressFinalize(this);
        }

        ~ItscamClient() { Dispose(); }

        internal void RaiseTriggerImage(CaptureResult result)
        {
            TriggerImage?.Invoke(this, result);
        }

        internal void RaiseSnapshotImage(CaptureResult result)
        {
            SnapshotImage?.Invoke(this, result);
        }

        internal void RaiseDisconnected(string reason)
        {
            Disconnected?.Invoke(this, reason);
        }

        internal void RaiseConnectionStateChanged(ConnectionState state,
                                                    string reason)
        {
            ConnectionStateChanged?.Invoke(
                this, new ConnectionStateEventArgs(state, reason));
        }

        internal void RaiseLog(LogLevel level, string message)
        {
            Log?.Invoke(this, new LogEventArgs(level, message));
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(ItscamClient));
        }

        private static NativeMethods.NativeEventSubscription ToNative(
            EventSubscription e)
        {
            return new NativeMethods.NativeEventSubscription
            {
                Pipeline         = e.Pipeline ? 1 : 0,
                TriggerMetadata  = e.TriggerMetadata ? 1 : 0,
                TriggerImage     = e.TriggerImage ? 1 : 0,
                SnapshotMetadata = e.SnapshotMetadata ? 1 : 0,
                SnapshotImage    = e.SnapshotImage ? 1 : 0,
                PreviewMetadata  = e.PreviewMetadata ? 1 : 0,
                PreviewImage     = e.PreviewImage ? 1 : 0,
                Gpio             = e.Gpio ? 1 : 0,
                Serial1          = e.Serial1 ? 1 : 0,
                Serial2          = e.Serial2 ? 1 : 0,
            };
        }

        private static NativeMethods.NativeCaptureSubscriptionConfig ToNative(
            CaptureSubscriptionConfig c)
        {
            return new NativeMethods.NativeCaptureSubscriptionConfig
            {
                IncludeTrigger  = c.IncludeTrigger ? 1 : 0,
                IncludeSnapshot = c.IncludeSnapshot ? 1 : 0,
                IncludeMetadata = c.IncludeMetadata ? 1 : 0,
                EmbedComments   = c.EmbedComments ? 1 : 0,
                EmbedExif       = c.EmbedExif ? 1 : 0,
                EmbedSignature  = c.EmbedSignature ? 1 : 0,
                TriggerQuality  = c.TriggerQuality,
                SnapshotQuality = c.SnapshotQuality,
            };
        }
    }
}
