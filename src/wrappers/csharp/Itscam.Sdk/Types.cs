// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using System;
using System.Collections.Generic;

namespace Pumatronix.Itscam
{
    /// <summary>
    /// Parameters for ItscamCgiClient.GetSnapshotAsync().  Mirrors the
    /// native ITSCAM_CgiSnapshotRequest struct; defaults match the
    /// camera's behaviour ("just take a snapshot with current settings").
    /// </summary>
    public sealed class SnapshotCgiRequest
    {
        /// <summary>Per-exposure shutter values (absolute).</summary>
        public IReadOnlyList<int> Shutters { get; set; } = Array.Empty<int>();

        /// <summary>Per-exposure gain values (absolute).</summary>
        public IReadOnlyList<int> Gains { get; set; } = Array.Empty<int>();

        /// <summary>JPEG quality (0-100).  -1 = camera default.</summary>
        public int Quality { get; set; } = -1;

        /// <summary>Combine multi-exposure frames into a single mosaic.</summary>
        public bool Mosaic { get; set; }

        /// <summary>Output format ("", "png").</summary>
        public string Format { get; set; } = string.Empty;

        /// <summary>Scenario id; -1 = unchanged.</summary>
        public int Scenario { get; set; } = -1;

        /// <summary>User crop "x0,y0,x1,y1".</summary>
        public string Crop { get; set; } = string.Empty;

        /// <summary>Text overlay ("tarja") to bake into the image.</summary>
        public string TextOverlay { get; set; } = string.Empty;

        /// <summary>
        /// Arbitrary user metadata.  Keys are prefixed with "User_" by
        /// the SDK if not already.
        /// </summary>
        public IReadOnlyDictionary<string, string> UserMetadata { get; set; }
            = new Dictionary<string, string>();
    }

    /// <summary>
    /// Single image returned by lastframe.cgi or one entry of a snapshot
    /// multipart response.
    /// </summary>
    public sealed class CgiImage
    {
        public string MimeType { get; }
        public byte[] Data { get; }

        public CgiImage(string mimeType, byte[] data)
        {
            MimeType = mimeType ?? string.Empty;
            Data     = data ?? Array.Empty<byte>();
        }

        /// <summary>Convenience: get a read-only view of the bytes.</summary>
        public ReadOnlySpan<byte> AsSpan() => Data;
    }

    /// <summary>
    /// Single MJPEG / trigger frame delivered through ItscamCgiClient.MjpegFrame.
    /// </summary>
    public sealed class CgiStreamFrame
    {
        public ulong Sequence { get; }
        public string MimeType { get; }
        public byte[] Data { get; }

        public CgiStreamFrame(ulong sequence, string mimeType, byte[] data)
        {
            Sequence = sequence;
            MimeType = mimeType ?? string.Empty;
            Data     = data ?? Array.Empty<byte>();
        }
    }

    /// <summary>Timestamp with millisecond precision from the binary client.</summary>
    public sealed class ItscamTimestamp
    {
        public int Year { get; set; }
        public int Month { get; set; }
        public int Day { get; set; }
        public int Hour { get; set; }
        public int Minute { get; set; }
        public int Second { get; set; }
        public int Millisecond { get; set; }
        public int TimezoneOffsetMinutes { get; set; }
    }

    /// <summary>Frame metadata from the binary TCP client.</summary>
    public sealed class FrameInfo
    {
        public ulong RequestId { get; set; }
        public ulong FrameCount { get; set; }
        public int MultiExpIndex { get; set; }
        public int MultiExpLength { get; set; }
        public int Shutter { get; set; }
        public float Gain { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }
        public ItscamTimestamp Timestamp { get; set; }
    }

    /// <summary>Capture delivered by the binary client (trigger or snapshot).</summary>
    public sealed class CaptureResult
    {
        public FrameInfo Frame { get; }
        public byte[] Jpeg { get; }
        public IReadOnlyList<string> Plates { get; }

        public CaptureResult(FrameInfo frame, byte[] jpeg,
                             IReadOnlyList<string> plates = null)
        {
            Frame  = frame ?? new FrameInfo();
            Jpeg   = jpeg ?? Array.Empty<byte>();
            Plates = plates ?? Array.Empty<string>();
        }
    }

    /// <summary>Camera profile entry from the binary client.</summary>
    public sealed class ProfileInfo
    {
        public uint Id { get; set; }
        public string Name { get; set; } = string.Empty;
        public string Description { get; set; } = string.Empty;
        public bool IsActive { get; set; }
    }

    /// <summary>Binary client connection state (matches native enum).</summary>
    public enum ConnectionState
    {
        Connected    = 0,
        Disconnected = 1,
        Reconnecting = 2,
        Reconnected  = 3,
    }

    /// <summary>SDK log level from the binary client.</summary>
    public enum LogLevel
    {
        Info  = 0,
        Error = 1,
    }

    /// <summary>Auto-reconnect settings for ItscamClient.ConnectAsync.</summary>
    public sealed class AutoReconnectConfig
    {
        public bool Enabled { get; set; } = true;
        public uint IntervalMs { get; set; } = 5000;
        /// <summary>0 = unlimited retries.</summary>
        public uint MaxRetries { get; set; } = 0;
    }

    /// <summary>Low-level event subscription flags.</summary>
    public sealed class EventSubscription
    {
        public bool Pipeline { get; set; }
        public bool TriggerMetadata { get; set; }
        public bool TriggerImage { get; set; }
        public bool SnapshotMetadata { get; set; }
        public bool SnapshotImage { get; set; }
        public bool PreviewMetadata { get; set; }
        public bool PreviewImage { get; set; }
        public bool Gpio { get; set; }
        public bool Serial1 { get; set; }
        public bool Serial2 { get; set; }
    }

    /// <summary>High-level capture subscription (trigger + snapshot JPEGs).</summary>
    public sealed class CaptureSubscriptionConfig
    {
        public bool IncludeTrigger { get; set; } = true;
        public bool IncludeSnapshot { get; set; } = true;
        public bool IncludeMetadata { get; set; } = true;
        public bool EmbedComments { get; set; } = true;
        public bool EmbedExif { get; set; } = true;
        public bool EmbedSignature { get; set; }
        /// <summary>-1 leaves quality unchanged on the camera.</summary>
        public int TriggerQuality { get; set; } = -1;
        public int SnapshotQuality { get; set; } = -1;

        public static CaptureSubscriptionConfig Default { get; } =
            new CaptureSubscriptionConfig();
    }

    /// <summary>Connection state change notification.</summary>
    public sealed class ConnectionStateEventArgs : EventArgs
    {
        public ConnectionState State { get; }
        public string Reason { get; }

        public ConnectionStateEventArgs(ConnectionState state, string reason)
        {
            State  = state;
            Reason = reason ?? string.Empty;
        }
    }

    /// <summary>SDK log message from the binary client worker thread.</summary>
    public sealed class LogEventArgs : EventArgs
    {
        public LogLevel Level { get; }
        public string Message { get; }

        public LogEventArgs(LogLevel level, string message)
        {
            Level   = level;
            Message = message ?? string.Empty;
        }
    }
}
