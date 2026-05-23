// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// P/Invoke surface for the ITSCAM SDK native library (libitscam_sdk.so
// or itscam_sdk.dll).  This file mirrors the C API headers in
// core/c_api/*.h.  All strings are marshalled as UTF-8.

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Pumatronix.Itscam.Native
{
    /// <summary>
    /// Native error codes returned by every ITSCAM_*_* function.
    /// Mirrors ITSCAM_ErrorCode in itscam_sdk_c.h.
    /// </summary>
    internal enum NativeErrorCode
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

    /// <summary>P/Invoke wrappers.</summary>
    internal static class NativeMethods
    {
        /// <summary>
        /// Name of the native library.  .NET probes for
        /// libitscam_sdk.so on Linux and itscam_sdk.dll on Windows
        /// automatically.  When consumers install the NuGet package the
        /// runtime-specific binary is copied next to their executable
        /// by Itscam.Sdk.targets.
        /// </summary>
        internal const string Lib = "itscam_sdk";

        // ====================================================================
        //  Binary client (ITSCAM_Client)
        // ====================================================================

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_create",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ItscamClient_create();

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ItscamClient_destroy(IntPtr client);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_connect",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode ItscamClient_connect(
            IntPtr client,
            [MarshalAs(UnmanagedType.LPStr)] string host,
            ushort port,
            uint timeoutMs,
            IntPtr reconnect);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_disconnect",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ItscamClient_disconnect(IntPtr client);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_isConnected",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ItscamClient_isConnected(IntPtr client);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_authenticate",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode ItscamClient_authenticate(
            IntPtr client,
            [MarshalAs(UnmanagedType.LPStr)] string password,
            uint timeoutMs);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_subscribe",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_subscribe(
            IntPtr client,
            ref NativeEventSubscription events,
            uint timeoutMs);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_subscribeCaptures",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_subscribeCaptures(
            IntPtr client,
            ref NativeCaptureSubscriptionConfig config,
            uint timeoutMs);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_captureSnapshot",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_captureSnapshot(
            IntPtr client,
            ref NativeBinarySnapshotRequest request,
            uint timeoutMs,
            out IntPtr outResults);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_getLastFrame",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_getLastFrame(
            IntPtr client,
            int quality,
            uint timeoutMs,
            out IntPtr outJpeg);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_getActiveProfileId",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_getActiveProfileId(
            IntPtr client,
            uint timeoutMs,
            out uint outId);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_setActiveProfile",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_setActiveProfile(
            IntPtr client,
            uint profileId,
            uint timeoutMs);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_listProfiles",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_listProfiles(
            IntPtr client,
            uint timeoutMs,
            out IntPtr outProfiles);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_reboot",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode ItscamClient_reboot(
            IntPtr client,
            uint timeoutMs);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_onTriggerImage",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ItscamClient_onTriggerImage(
            IntPtr client,
            NativeCaptureCallback callback,
            IntPtr userData);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_onSnapshotImage",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ItscamClient_onSnapshotImage(
            IntPtr client,
            NativeCaptureCallback callback,
            IntPtr userData);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_onDisconnect",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ItscamClient_onDisconnect(
            IntPtr client,
            NativeDisconnectCallback callback,
            IntPtr userData);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_onConnectionState",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ItscamClient_onConnectionState(
            IntPtr client,
            NativeConnectionStateCallback callback,
            IntPtr userData);

        [DllImport(Lib, EntryPoint = "ITSCAM_Client_onLog",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ItscamClient_onLog(
            IntPtr client,
            NativeLogCallback callback,
            IntPtr userData);

        [DllImport(Lib, EntryPoint = "ITSCAM_CaptureResultArray_size",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr CaptureResultArray_size(IntPtr array);

        [DllImport(Lib, EntryPoint = "ITSCAM_CaptureResultArray_get",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr CaptureResultArray_get(
            IntPtr array, UIntPtr index);

        [DllImport(Lib, EntryPoint = "ITSCAM_CaptureResultArray_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void CaptureResultArray_destroy(IntPtr array);

        [DllImport(Lib, EntryPoint = "ITSCAM_CaptureResult_getInfo",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeFrameInfo CaptureResult_getInfo(
            IntPtr result);

        [DllImport(Lib, EntryPoint = "ITSCAM_CaptureResult_getJpeg",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr CaptureResult_getJpeg(
            IntPtr result, out UIntPtr outSize);

        [DllImport(Lib, EntryPoint = "ITSCAM_CaptureResult_getPlateCount",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr CaptureResult_getPlateCount(IntPtr result);

        [DllImport(Lib, EntryPoint = "ITSCAM_CaptureResult_getPlate",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr CaptureResult_getPlate(
            IntPtr result, UIntPtr index);

        [DllImport(Lib, EntryPoint = "ITSCAM_ByteArray_size",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr ByteArray_size(IntPtr array);

        [DllImport(Lib, EntryPoint = "ITSCAM_ByteArray_data",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ByteArray_data(IntPtr array);

        [DllImport(Lib, EntryPoint = "ITSCAM_ByteArray_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ByteArray_destroy(IntPtr array);

        [DllImport(Lib, EntryPoint = "ITSCAM_ProfileArray_size",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr ProfileArray_size(IntPtr array);

        [DllImport(Lib, EntryPoint = "ITSCAM_ProfileArray_get",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ProfileArray_get(
            IntPtr array, UIntPtr index, out NativeProfileInfo outInfo);

        [DllImport(Lib, EntryPoint = "ITSCAM_ProfileArray_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ProfileArray_destroy(IntPtr array);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate void NativeCaptureCallback(IntPtr result, IntPtr userData);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate void NativeDisconnectCallback(IntPtr reason, IntPtr userData);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate void NativeConnectionStateCallback(
            int state, IntPtr reason, IntPtr userData);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate void NativeLogCallback(
            int level, IntPtr message, IntPtr userData);

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeAutoReconnectConfig
        {
            public int Enabled;
            public uint IntervalMs;
            public uint MaxRetries;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeEventSubscription
        {
            public int Pipeline;
            public int TriggerMetadata;
            public int TriggerImage;
            public int SnapshotMetadata;
            public int SnapshotImage;
            public int PreviewMetadata;
            public int PreviewImage;
            public int Gpio;
            public int Serial1;
            public int Serial2;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeCaptureSubscriptionConfig
        {
            public int IncludeTrigger;
            public int IncludeSnapshot;
            public int IncludeMetadata;
            public int EmbedComments;
            public int EmbedExif;
            public int EmbedSignature;
            public int TriggerQuality;
            public int SnapshotQuality;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeBinarySnapshotRequest
        {
            public int Reserved;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeTimestamp
        {
            public ushort Year;
            public ushort Month;
            public ushort Day;
            public ushort Hour;
            public ushort Minute;
            public ushort Second;
            public ushort Millisecond;
            public int TimezoneOffset;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeFrameInfo
        {
            public ulong RequestId;
            public ulong FrameCount;
            public int MultiExpIndex;
            public int MultiExpLength;
            public int Shutter;
            public float Gain;
            public uint Width;
            public uint Height;
            public NativeTimestamp Timestamp;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeProfileInfo
        {
            public uint Id;
            public IntPtr Name;
            public IntPtr Description;
            public int IsActive;
        }

        internal static CaptureResult ToCaptureResult(IntPtr resultPtr)
        {
            if (resultPtr == IntPtr.Zero)
                return new CaptureResult(new FrameInfo(), Array.Empty<byte>());

            var info = CaptureResult_getInfo(resultPtr);
            var frame = new FrameInfo
            {
                RequestId = info.RequestId,
                FrameCount = info.FrameCount,
                MultiExpIndex = info.MultiExpIndex,
                MultiExpLength = info.MultiExpLength,
                Shutter = info.Shutter,
                Gain = info.Gain,
                Width = (int)info.Width,
                Height = (int)info.Height,
                Timestamp = new ItscamTimestamp
                {
                    Year = info.Timestamp.Year,
                    Month = info.Timestamp.Month,
                    Day = info.Timestamp.Day,
                    Hour = info.Timestamp.Hour,
                    Minute = info.Timestamp.Minute,
                    Second = info.Timestamp.Second,
                    Millisecond = info.Timestamp.Millisecond,
                    TimezoneOffsetMinutes = info.Timestamp.TimezoneOffset,
                },
            };

            IntPtr jpegPtr = CaptureResult_getJpeg(resultPtr, out UIntPtr jpegSize);
            byte[] jpeg = CopyBytes(jpegPtr, (int)jpegSize);

            int plateCount = (int)CaptureResult_getPlateCount(resultPtr);
            var plates = new string[plateCount];
            for (int i = 0; i < plateCount; ++i)
                plates[i] = PtrToStringUtf8(
                    CaptureResult_getPlate(resultPtr, (UIntPtr)i));

            return new CaptureResult(frame, jpeg, plates);
        }

        internal static IReadOnlyList<CaptureResult> TakeCaptureResults(IntPtr arrayPtr)
        {
            if (arrayPtr == IntPtr.Zero)
                return Array.Empty<CaptureResult>();

            try
            {
                int n = (int)CaptureResultArray_size(arrayPtr);
                var list = new List<CaptureResult>(n);
                for (int i = 0; i < n; ++i)
                {
                    var ptr = CaptureResultArray_get(arrayPtr, (UIntPtr)i);
                    list.Add(ToCaptureResult(ptr));
                }
                return list;
            }
            finally
            {
                CaptureResultArray_destroy(arrayPtr);
            }
        }

        internal static byte[] TakeByteArray(IntPtr arrayPtr)
        {
            if (arrayPtr == IntPtr.Zero)
                return Array.Empty<byte>();

            try
            {
                int n = (int)ByteArray_size(arrayPtr);
                if (n <= 0)
                    return Array.Empty<byte>();
                return CopyBytes(ByteArray_data(arrayPtr), n);
            }
            finally
            {
                ByteArray_destroy(arrayPtr);
            }
        }

        internal static IReadOnlyList<ProfileInfo> TakeProfiles(IntPtr arrayPtr)
        {
            if (arrayPtr == IntPtr.Zero)
                return Array.Empty<ProfileInfo>();

            try
            {
                int n = (int)ProfileArray_size(arrayPtr);
                var list = new List<ProfileInfo>(n);
                for (int i = 0; i < n; ++i)
                {
                    if (ProfileArray_get(arrayPtr, (UIntPtr)i,
                            out NativeProfileInfo native) == 0)
                        continue;

                    list.Add(new ProfileInfo
                    {
                        Id          = native.Id,
                        Name        = PtrToStringUtf8(native.Name),
                        Description = PtrToStringUtf8(native.Description),
                        IsActive    = native.IsActive != 0,
                    });
                }
                return list;
            }
            finally
            {
                ProfileArray_destroy(arrayPtr);
            }
        }

        // ====================================================================
        //  REST client (ITSCAM_RestClient)
        // ====================================================================

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_create",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Rest_create();

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Rest_destroy(IntPtr client);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_setBaseUrl",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Rest_setBaseUrl(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string host, ushort port,
            [MarshalAs(UnmanagedType.LPStr)] string scheme);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_setCaCertFile",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Rest_setCaCertFile(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_setCaCertData",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Rest_setCaCertData(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string pem);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_setVerifyServerCertificate",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Rest_setVerifyServerCertificate(IntPtr c,
            int verify);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_setClientCertificate",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Rest_setClientCertificate(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string certPem,
            [MarshalAs(UnmanagedType.LPStr)] string keyPem);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_login",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Rest_login(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string user,
            [MarshalAs(UnmanagedType.LPStr)] string pass,
            uint timeoutMs, out IntPtr outResponse);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_setAuthToken",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Rest_setAuthToken(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string token);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_clearAuthToken",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Rest_clearAuthToken(IntPtr c);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_httpGet",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Rest_httpGet(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string path, uint timeoutMs,
            out IntPtr outResponse);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_httpPut",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Rest_httpPut(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string path,
            [MarshalAs(UnmanagedType.LPStr)] string body, uint timeoutMs,
            out IntPtr outResponse);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_httpPost",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Rest_httpPost(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string path,
            [MarshalAs(UnmanagedType.LPStr)] string body, uint timeoutMs,
            out IntPtr outResponse);

        [DllImport(Lib, EntryPoint = "ITSCAM_RestClient_httpDelete",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Rest_httpDelete(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string path, uint timeoutMs,
            out IntPtr outResponse);

        // ====================================================================
        //  CGI client (ITSCAM_CgiClient)
        // ====================================================================

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_create",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr Cgi_create();

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Cgi_destroy(IntPtr c);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_setBaseUrl",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Cgi_setBaseUrl(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string host, ushort port,
            [MarshalAs(UnmanagedType.LPStr)] string scheme);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_setApiPrefix",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Cgi_setApiPrefix(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string prefix);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_setCaCertFile",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Cgi_setCaCertFile(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string path);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_setCaCertData",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Cgi_setCaCertData(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string pem);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_setVerifyServerCertificate",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Cgi_setVerifyServerCertificate(IntPtr c,
            int verify);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_setClientCertificate",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Cgi_setClientCertificate(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string certPem,
            [MarshalAs(UnmanagedType.LPStr)] string keyPem);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_login",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern NativeErrorCode Cgi_login(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string user,
            [MarshalAs(UnmanagedType.LPStr)] string pass, uint timeoutMs);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_setAuthToken",
                   CallingConvention = CallingConvention.Cdecl,
                   CharSet = CharSet.Ansi, BestFitMapping = false)]
        internal static extern void Cgi_setAuthToken(IntPtr c,
            [MarshalAs(UnmanagedType.LPStr)] string token);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_clearAuthToken",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Cgi_clearAuthToken(IntPtr c);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_getLastFrame",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode Cgi_getLastFrame(IntPtr c,
            uint timeoutMs, out IntPtr outImage);

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeSnapshotRequest
        {
            public IntPtr shutters;        // int32_t*
            public UIntPtr shuttersLen;
            public IntPtr gains;           // int32_t*
            public UIntPtr gainsLen;
            public int quality;
            public int mosaic;
            public IntPtr format;          // const char*
            public int scenario;
            public IntPtr crop;
            public IntPtr textOverlay;
            public IntPtr userMetadataKeys;   // const char**
            public IntPtr userMetadataValues; // const char**
        }

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_getSnapshot",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode Cgi_getSnapshot(IntPtr c,
            ref NativeSnapshotRequest request, uint timeoutMs,
            out IntPtr outImages);

        // MJPEG stream callback signature
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate void NativeStreamCallback(IntPtr framePtr,
                                                    IntPtr userData);

        [StructLayout(LayoutKind.Sequential)]
        internal struct NativeStreamFrame
        {
            public ulong sequence;
            public IntPtr mimeType;
            public IntPtr data;
            public UIntPtr dataLen;
        }

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_startMjpegStream",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode Cgi_startMjpegStream(IntPtr c,
            NativeStreamCallback callback, IntPtr userData, uint timeoutMs);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_stopMjpegStream",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Cgi_stopMjpegStream(IntPtr c);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_isMjpegStreamRunning",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern int Cgi_isMjpegStreamRunning(IntPtr c);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_forceTrigger",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode Cgi_forceTrigger(IntPtr c,
            uint timeoutMs, out IntPtr outResponse);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiClient_reboot",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern NativeErrorCode Cgi_reboot(IntPtr c,
            uint timeoutMs, out IntPtr outResponse);

        // ====================================================================
        //  Shared accessors
        // ====================================================================

        [DllImport(Lib, EntryPoint = "ITSCAM_String_data",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr String_data(IntPtr s);

        [DllImport(Lib, EntryPoint = "ITSCAM_String_size",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr String_size(IntPtr s);

        [DllImport(Lib, EntryPoint = "ITSCAM_String_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void String_destroy(IntPtr s);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiImage_mimeType",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr CgiImage_mimeType(IntPtr img);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiImage_data",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr CgiImage_data(IntPtr img);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiImage_size",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr CgiImage_size(IntPtr img);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiImage_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void CgiImage_destroy(IntPtr img);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiImageArray_size",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern UIntPtr CgiImageArray_size(IntPtr arr);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiImageArray_get",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr CgiImageArray_get(IntPtr arr,
            UIntPtr index);

        [DllImport(Lib, EntryPoint = "ITSCAM_CgiImageArray_destroy",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern void CgiImageArray_destroy(IntPtr arr);

        // ====================================================================
        //  Diagnostics
        // ====================================================================

        /// <summary>
        /// Thread-local last-error string from the native SDK.  Populated on
        /// every non-OK return; pointer is valid until the next SDK call on
        /// this thread.
        /// </summary>
        [DllImport(Lib, EntryPoint = "ITSCAM_getLastError",
                   CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr GetLastErrorPtr();

        /// <summary>
        /// Copy the thread-local last-error string into a managed
        /// <see cref="string"/>.  Safe to call after any failing SDK call.
        /// </summary>
        internal static string GetLastErrorMessage()
        {
            return PtrToStringUtf8(GetLastErrorPtr());
        }

        // ====================================================================
        //  Marshalling helpers
        // ====================================================================

        /// <summary>Pull a UTF-8 string out of a const char* native pointer.</summary>
        internal static string PtrToStringUtf8(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero) return string.Empty;
            int len = 0;
            while (Marshal.ReadByte(ptr, len) != 0) ++len;
            if (len == 0) return string.Empty;
            byte[] buffer = new byte[len];
            Marshal.Copy(ptr, buffer, 0, len);
            return System.Text.Encoding.UTF8.GetString(buffer);
        }

        /// <summary>
        /// Allocate a NUL-terminated UTF-8 buffer for the supplied string.
        /// Caller must release it with <see cref="FreeUtf8(IntPtr)"/>.
        /// </summary>
        internal static IntPtr AllocUtf8(string s)
        {
            if (s == null) s = string.Empty;
            byte[] utf8 = System.Text.Encoding.UTF8.GetBytes(s);
            IntPtr ptr = Marshal.AllocHGlobal(utf8.Length + 1);
            if (utf8.Length > 0) Marshal.Copy(utf8, 0, ptr, utf8.Length);
            Marshal.WriteByte(ptr, utf8.Length, 0);
            return ptr;
        }

        internal static void FreeUtf8(IntPtr ptr)
        {
            if (ptr != IntPtr.Zero) Marshal.FreeHGlobal(ptr);
        }

        internal static string TakeString(IntPtr stringHandle)
        {
            if (stringHandle == IntPtr.Zero) return string.Empty;
            try
            {
                IntPtr data = String_data(stringHandle);
                return PtrToStringUtf8(data);
            }
            finally
            {
                String_destroy(stringHandle);
            }
        }

        internal static byte[] CopyBytes(IntPtr ptr, int length)
        {
            if (ptr == IntPtr.Zero || length <= 0) return Array.Empty<byte>();
            byte[] buffer = new byte[length];
            Marshal.Copy(ptr, buffer, 0, length);
            return buffer;
        }
    }
}
