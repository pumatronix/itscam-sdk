// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using System;
using Pumatronix.Itscam.Native;

namespace Pumatronix.Itscam
{
    /// <summary>
    /// Base exception thrown by all ITSCAM SDK operations on failure.
    /// </summary>
    public class ItscamException : Exception
    {
        /// <summary>Native error code returned by the SDK.</summary>
        public ItscamErrorCode ErrorCode { get; }

        public ItscamException(ItscamErrorCode code, string message)
            : base(message)
        {
            ErrorCode = code;
        }

        internal static void ThrowIfFailed(NativeErrorCode code, string context)
        {
            if (code == NativeErrorCode.Ok) return;
            string detail = NativeMethods.GetLastErrorMessage();
            // Avoid "PUT /path: PUT /path: message" when the native layer
            // already prefixes the verb and path.
            string ctxWithDetail = string.IsNullOrEmpty(detail)
                ? context
                : detail.StartsWith(context, StringComparison.Ordinal)
                    ? detail
                    : $"{context}: {detail}";

            var managed = (ItscamErrorCode)(int)code;
            throw managed switch
            {
                ItscamErrorCode.Timeout            => new ItscamTimeoutException(ctxWithDetail),
                ItscamErrorCode.ConnectionFailed   => new ItscamConnectionException(ctxWithDetail),
                ItscamErrorCode.NotAuthenticated   => new ItscamAuthenticationException(ctxWithDetail),
                ItscamErrorCode.InvalidParameter   => new ItscamInvalidParameterException(ctxWithDetail),
                ItscamErrorCode.ServerError        => new ItscamServerException(ctxWithDetail),
                ItscamErrorCode.Disconnected       => new ItscamConnectionException(ctxWithDetail + " (disconnected)"),
                _                                  => new ItscamException(managed, ctxWithDetail),
            };
        }
    }

    public sealed class ItscamTimeoutException : ItscamException
    {
        public ItscamTimeoutException(string context)
            : base(ItscamErrorCode.Timeout, $"ITSCAM operation timed out: {context}") {}
    }

    public sealed class ItscamConnectionException : ItscamException
    {
        public ItscamConnectionException(string context)
            : base(ItscamErrorCode.ConnectionFailed,
                   $"ITSCAM connection failed: {context}") {}
    }

    public sealed class ItscamAuthenticationException : ItscamException
    {
        public ItscamAuthenticationException(string context)
            : base(ItscamErrorCode.NotAuthenticated,
                   $"ITSCAM authentication failed: {context}") {}
    }

    public sealed class ItscamInvalidParameterException : ItscamException
    {
        public ItscamInvalidParameterException(string context)
            : base(ItscamErrorCode.InvalidParameter,
                   $"ITSCAM invalid parameter: {context}") {}
    }

    public sealed class ItscamServerException : ItscamException
    {
        public ItscamServerException(string context)
            : base(ItscamErrorCode.ServerError,
                   $"ITSCAM server error: {context}") {}
    }

    /// <summary>Mirror of native ITSCAM_ErrorCode.</summary>
    public enum ItscamErrorCode
    {
        Ok = 0,
        ConnectionFailed = 1,
        Timeout = 2,
        NotAuthenticated = 3,
        InvalidParameter = 4,
        ServerError = 5,
        Disconnected = 6,
        Unknown = 7,
        NullHandle = 8,
        AllocationFailed = 9,
    }
}
