// C# wrapper smoke test, against tests/standin/server.js.
//
// Copyright (c) 2026 Pumatronix
//
// Same checks as the other languages (tests/README.md).  Environment:
// ITSCAM_TEST_HOST (127.0.0.1), ITSCAM_TEST_PORT (18080); libitscam_sdk is
// found through LD_LIBRARY_PATH.

using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Pumatronix.Itscam;

internal static class Program
{
    private static int _failures;

    // The wrapper has no call for the native library's version: ask the
    // library itself, under the name the wrapper loads it by
    [DllImport("itscam_sdk", EntryPoint = "ITSCAM_getVersion")]
    private static extern IntPtr NativeVersion();

    private static void Check(bool ok, string name, string detail = "")
    {
        Console.WriteLine($"{(ok ? "PASS" : "FAIL")} {name}{(detail.Length > 0 ? ": " + detail : "")}");
        if (!ok) _failures++;
    }

    private static string Env(string name, string fallback)
    {
        var v = Environment.GetEnvironmentVariable(name);
        return string.IsNullOrEmpty(v) ? fallback : v;
    }

    private static async Task<int> Main()
    {
        var host = Env("ITSCAM_TEST_HOST", "127.0.0.1");
        var port = ushort.Parse(Env("ITSCAM_TEST_PORT", "18080"));
        Console.WriteLine($"C# smoke ({Environment.Version}) against {host}:{port}");

        var wrapperVersion = typeof(ItscamRestClient).Assembly
            .GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion ?? "";
        Check(wrapperVersion.Length > 0, "wrapper version", wrapperVersion);

        // ITSCAM_TEST_EXPECT_VERSION: the library under test, so that a stale
        // copy found first fails instead of passing
        var version = Marshal.PtrToStringAnsi(NativeVersion()) ?? "";
        var expected = Env("ITSCAM_TEST_EXPECT_VERSION", "");
        Check(version.Length > 0 && (expected.Length == 0 || version == expected), "version",
              expected.Length == 0 || version == expected ? version : $"{version}, expected {expected}");

        using (var rest = new ItscamRestClient())
        {
            rest.SetBaseUrl(host, port, "http");
            try
            {
                await rest.LoginAsync("admin", "1234");
                Check(true, "rest login");
            }
            catch (ItscamException e) { Check(false, "rest login", e.Message); }

            try
            {
                var profiles = await rest.GetProfilesAsync();
                var ok = profiles.Count == 1 && profiles[0].Id == 3 &&
                         profiles[0].Name == "Câmera São João";
                Check(ok, "rest typed get",
                      profiles.Count > 0 ? $"id {profiles[0].Id}, name '{profiles[0].Name}'" : "no profile");
            }
            catch (ItscamException e) { Check(false, "rest typed get", e.Message); }

            try
            {
                await rest.GetAsync("/api/does-not-exist");
                Check(false, "rest 404 maps to ItscamInvalidParameterException", "succeeded");
            }
            catch (ItscamException e)
            {
                Check(e is ItscamInvalidParameterException && e.ErrorCode == ItscamErrorCode.InvalidParameter,
                      "rest 404 maps to ItscamInvalidParameterException", $"{e.GetType().Name}: {e.Message}");
            }
        }

        using (var cgi = new ItscamCgiClient())
        {
            cgi.SetBaseUrl(host, port, "http");
            try
            {
                var images = await cgi.GetSnapshotAsync(new SnapshotCgiRequest());
                var plates = JpegUtils.ExtractPlateRecognitions(JpegUtils.ParseCommentTags(images[0].Data));
                Check(images.Count == 1 && plates.Count == 1 && plates[0].Plate == "ABC1D23",
                      "cgi snapshot + jpeg plate", plates.Count > 0 ? plates[0].Plate : "no plate");
            }
            catch (ItscamException e) { Check(false, "cgi snapshot + jpeg plate", e.Message); }
        }

        using (var rest = new ItscamRestClient())
        {
            rest.SetBaseUrl(host, port, "http");
            await rest.LoginAsync("admin", "1234");
            try
            {
                var health = (await rest.GetSystemTelemetryAsync()).Health;
                var ok = health != null && health.Status == "warning" && health.Issues != null &&
                         health.Issues.Count == 1 && health.Issues[0].Id == "storageFull";
                Check(ok, "telemetry read", $"health {health?.Status}");
            }
            catch (ItscamException e) { Check(false, "telemetry read", e.Message); }

            // The stand-in pushes every 200 ms
            var received = 0;
            rest.TelemetryReceived += (_, update) => Interlocked.Increment(ref received);
            try
            {
                rest.StartTelemetryStream();
                for (var i = 0; i < 100 && Volatile.Read(ref received) < 2; i++) await Task.Delay(50);
                rest.StopTelemetryStream();
                var latest = rest.LatestTelemetry();
                Check(received >= 2 && latest != null && latest.Sequence >= 2,
                      "telemetry stream", $"{received} documents");
            }
            catch (ItscamException e) { Check(false, "telemetry stream", e.Message); }
        }

        // The stand-in's certificate is self-signed: nobody trusts it
        var tlsPort = ushort.Parse(Env("ITSCAM_TEST_TLS_PORT", "18443"));
        using (var rest = new ItscamRestClient())
        {
            rest.SetBaseUrl(host, tlsPort, "https");
            try
            {
                await rest.LoginAsync("admin", "1234");
                Check(false, "https refuses an untrusted certificate", "accepted");
            }
            catch (ItscamException e) { Check(true, "https refuses an untrusted certificate", e.Message); }
        }
        using (var rest = new ItscamRestClient())
        {
            rest.SetBaseUrl(host, tlsPort, "https");
            rest.SetCaCertFile(Env("ITSCAM_TEST_CA", "tests/standin/tls/cert.pem"));
            try
            {
                await rest.LoginAsync("admin", "1234");
                Check((await rest.GetProfilesAsync()).Count == 1, "https with the certificate trusted");
            }
            catch (ItscamException e) { Check(false, "https with the certificate trusted", e.Message); }
        }

        // Nothing listens on port 1: fails fast and leaves no thread behind
        using (var client = new ItscamClient())
        {
            try
            {
                await client.ConnectAsync(host, 1, 2000);
                Check(false, "binary connect to a closed port fails", "connected");
            }
            catch (ItscamException e) { Check(true, "binary connect to a closed port fails", e.Message); }
        }

        Console.WriteLine(_failures > 0 ? "C# smoke: FAILED" : "C# smoke: OK");
        return _failures > 0 ? 1 : 0;
    }
}
