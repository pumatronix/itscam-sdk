// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using System;
using System.Collections.Generic;
using Pumatronix.Itscam.Native;

namespace Pumatronix.Itscam
{
    /// <summary>
    /// Routes native binary-client callbacks back to managed ItscamClient
    /// instances.  Static delegate fields prevent GC while the native worker
    /// is active.
    /// </summary>
    internal static class BinaryClientCallbacks
    {
        private static readonly object RegistryLock = new object();
        private static readonly Dictionary<IntPtr, ItscamClient> Registry
            = new Dictionary<IntPtr, ItscamClient>();

        private static readonly NativeMethods.NativeCaptureCallback TriggerThunk
            = OnTriggerImage;
        private static readonly NativeMethods.NativeCaptureCallback SnapshotThunk
            = OnSnapshotImage;
        private static readonly NativeMethods.NativeDisconnectCallback DisconnectThunk
            = OnDisconnect;
        private static readonly NativeMethods.NativeConnectionStateCallback
            ConnectionStateThunk = OnConnectionState;
        private static readonly NativeMethods.NativeLogCallback LogThunk = OnLog;

        internal static void Attach(ItscamClient client)
        {
            var handle = client.NativeHandle;
            lock (RegistryLock)
                Registry[handle] = client;

            NativeMethods.ItscamClient_onTriggerImage(
                handle, TriggerThunk, handle);
            NativeMethods.ItscamClient_onSnapshotImage(
                handle, SnapshotThunk, handle);
            NativeMethods.ItscamClient_onDisconnect(
                handle, DisconnectThunk, handle);
            NativeMethods.ItscamClient_onConnectionState(
                handle, ConnectionStateThunk, handle);
            NativeMethods.ItscamClient_onLog(handle, LogThunk, handle);
        }

        internal static void Detach(IntPtr handle)
        {
            NativeMethods.ItscamClient_onTriggerImage(handle, null, IntPtr.Zero);
            NativeMethods.ItscamClient_onSnapshotImage(handle, null, IntPtr.Zero);
            NativeMethods.ItscamClient_onDisconnect(handle, null, IntPtr.Zero);
            NativeMethods.ItscamClient_onConnectionState(handle, null, IntPtr.Zero);
            NativeMethods.ItscamClient_onLog(handle, null, IntPtr.Zero);

            lock (RegistryLock)
                Registry.Remove(handle);
        }

        private static ItscamClient Lookup(IntPtr userData)
        {
            lock (RegistryLock)
            {
                Registry.TryGetValue(userData, out var client);
                return client;
            }
        }

        private static void OnTriggerImage(IntPtr result, IntPtr userData)
        {
            var client = Lookup(userData);
            if (client == null) return;
            client.RaiseTriggerImage(
                NativeMethods.ToCaptureResult(result));
        }

        private static void OnSnapshotImage(IntPtr result, IntPtr userData)
        {
            var client = Lookup(userData);
            if (client == null) return;
            client.RaiseSnapshotImage(
                NativeMethods.ToCaptureResult(result));
        }

        private static void OnDisconnect(IntPtr reason, IntPtr userData)
        {
            var client = Lookup(userData);
            if (client == null) return;
            client.RaiseDisconnected(
                NativeMethods.PtrToStringUtf8(reason));
        }

        private static void OnConnectionState(int state, IntPtr reason,
                                              IntPtr userData)
        {
            var client = Lookup(userData);
            if (client == null) return;
            client.RaiseConnectionStateChanged(
                (ConnectionState)state,
                NativeMethods.PtrToStringUtf8(reason));
        }

        private static void OnLog(int level, IntPtr message, IntPtr userData)
        {
            var client = Lookup(userData);
            if (client == null) return;
            client.RaiseLog((LogLevel)level,
                NativeMethods.PtrToStringUtf8(message));
        }
    }
}
