// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using System;
using System.Threading;
using System.Threading.Tasks;

namespace Pumatronix.Itscam
{
    /// <summary>
    /// A client's native handle, shared by its calls.  Dispose may run while
    /// an async call still uses the handle: the handle is then destroyed
    /// once the last such call ends.
    /// </summary>
    internal sealed class ClientHandle
    {
        private readonly object _lock = new object();
        private readonly string _owner;
        private readonly Action<IntPtr> _destroy;
        private IntPtr _handle;
        private int _users;
        private bool _closed;

        internal ClientHandle(IntPtr handle, string owner,
                              Action<IntPtr> destroy)
        {
            _handle = handle;
            _owner = owner;
            _destroy = destroy;
        }

        /// <summary>Whether the client is disposed.</summary>
        internal bool IsClosed
        {
            get { lock (_lock) return _closed; }
        }

        /// <summary>Runs <paramref name="call"/> with the handle.</summary>
        /// <exception cref="ObjectDisposedException">The client is disposed.</exception>
        internal void Use(Action<IntPtr> call)
        {
            var h = Acquire();
            try { call(h); }
            finally { Release(); }
        }

        internal T Use<T>(Func<IntPtr, T> call)
        {
            var h = Acquire();
            try { return call(h); }
            finally { Release(); }
        }

        /// <summary>Like <see cref="Use"/>, but does nothing once disposed.</summary>
        internal void UseIfOpen(Action<IntPtr> call)
        {
            if (!TryAcquire(out var h)) return;
            try { call(h); }
            finally { Release(); }
        }

        /// <summary>
        /// Runs <paramref name="call"/> on the thread pool.  The call holds
        /// the handle from now on: disposing the client before it runs does
        /// not cancel it.
        /// </summary>
        /// <exception cref="ObjectDisposedException">The client is disposed.</exception>
        internal Task UseAsync(Action<IntPtr> call)
        {
            var h = Acquire();
            return Task.Run(() =>
            {
                try { call(h); }
                finally { Release(); }
            });
        }

        internal Task<T> UseAsync<T>(Func<IntPtr, T> call)
        {
            var h = Acquire();
            return Task.Run(() =>
            {
                try { return call(h); }
                finally { Release(); }
            });
        }

        /// <summary>
        /// Marks the client disposed and runs <paramref name="beforeDestroy"/>
        /// with the handle, once.  The handle is destroyed here if no call
        /// holds it, otherwise when the last one ends.
        /// </summary>
        /// <returns>False if the client was already disposed.</returns>
        internal bool Close(Action<IntPtr> beforeDestroy)
        {
            IntPtr h;
            lock (_lock)
            {
                if (_closed) return false;
                _closed = true;
                ++_users;
                h = _handle;
            }
            try
            {
                beforeDestroy?.Invoke(h);
            }
            finally
            {
                var last = Drop();
                if (last != IntPtr.Zero) _destroy(last);
            }
            return true;
        }

        private IntPtr Acquire()
        {
            if (!TryAcquire(out var h))
                throw new ObjectDisposedException(_owner);
            return h;
        }

        private bool TryAcquire(out IntPtr handle)
        {
            lock (_lock)
            {
                handle = IntPtr.Zero;
                if (_closed) return false;
                ++_users;
                handle = _handle;
                return true;
            }
        }

        // The last call to end after Dispose destroys the handle on a thread
        // of its own: an event handler may be waiting for that call, or for
        // an async call queued on the thread pool, and destroying waits for
        // the handlers.
        private void Release()
        {
            var last = Drop();
            if (last == IntPtr.Zero) return;
            new Thread(() => _destroy(last))
            {
                IsBackground = true,
                Name = "itscam-sdk-destroy",
            }.Start();
        }

        private IntPtr Drop()
        {
            lock (_lock)
            {
                if (--_users > 0 || !_closed) return IntPtr.Zero;
                var h = _handle;
                _handle = IntPtr.Zero;
                return h;
            }
        }
    }
}
