// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using Pumatronix.Itscam.Native;

namespace Pumatronix.Itscam
{
    /// <summary>Settings shared by every client.</summary>
    public static class ItscamSdk
    {
        /// <summary>
        /// Set how many threads run client callbacks.  Callbacks of every
        /// client run on a pool of SDK threads shared by all clients, in
        /// order and one at a time per client.
        /// </summary>
        /// <param name="count">
        /// Maximum number of threads; 0 restores the default (the number of CPU cores, at most 4).
        /// </param>
        public static void SetCallbackThreads(uint count)
        {
            NativeMethods.SetCallbackThreads(count);
        }
    }
}
