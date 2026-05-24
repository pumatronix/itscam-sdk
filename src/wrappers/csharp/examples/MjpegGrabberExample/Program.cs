// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// Example: Simple MJPEG Frame-Rate Grabber
//
// Demonstrates the SDK's typed REST surface with partial serialization:
//
//   * GetProfilesAsync() -- read profile names and ids.
//   * UpdateProfileByIdAsync() -- disable trigger with a partial struct.
//   * SetStreamConfigAsync() -- configure MJPEG stream parameters.
//   * SetAnalyticsConfigAsync() / SetOcrConfigAsync() -- typed config updates.
//
// Steps performed (all via REST, which always requires authentication):
//
//   1. Login to the camera.
//   2. Fetch all image profiles, locate the "day" and "night" profiles
//      (by name, case-insensitive) and disable trigger on each using
//      a typed partial UpdateProfileByIdAsync call.
//   3. SetStreamConfigAsync to set mjpeg.main.useTriggerFrames = false
//      and mjpeg.main.framerate (default 30).
//   4. SetAnalyticsConfigAsync to disable voting.
//   5. SetOcrConfigAsync to disable OCR.
//   6. Open the MJPEG stream via the CGI client (auth is optional on the
//      camera's defaults), count frames for a configurable measurement
//      window, and print the measured frames-per-second to the console.
//
// Usage:
//     dotnet run -- <host> --user <user> --password <pass> \
//                    [--https] [--insecure] \
//                    [--duration <seconds>] [--framerate <fps>]
//
//   --user / --password   required - REST always needs credentials
//   --https               use HTTPS instead of plain HTTP
//   --insecure            skip TLS server certificate verification (dev only)
//   --duration            measurement window in seconds (default: 10)
//   --framerate           MJPEG stream framerate to configure (default: 30)

using System;
using System.Collections.Generic;
using System.Text.Json.Nodes;
using System.Threading.Tasks;
using Pumatronix.Itscam;
using Pumatronix.Itscam.RestTypes;

class Program
{
    static async Task<int> Main(string[] args)
    {
        string host     = null;
        string user     = null;
        string pass     = null;
        bool   useTls   = false;
        bool   insecure = false;
        int    duration  = 10;
        int    framerate = 30;

        for (int i = 0; i < args.Length; ++i)
        {
            switch (args[i])
            {
                case "--https":    useTls   = true; break;
                case "--insecure": insecure = true; break;
                case "--user" when i + 1 < args.Length:
                    user = args[++i]; break;
                case "--password" when i + 1 < args.Length:
                    pass = args[++i]; break;
                case "--duration" when i + 1 < args.Length:
                    duration = int.Parse(args[++i]); break;
                case "--framerate" when i + 1 < args.Length:
                    framerate = int.Parse(args[++i]); break;
                default:
                    if (host == null) host = args[i];
                    else
                    {
                        Console.Error.WriteLine($"Unexpected argument: {args[i]}");
                        return PrintUsage();
                    }
                    break;
            }
        }

        if (host == null || user == null || pass == null)
            return PrintUsage();

        string scheme = useTls ? "https" : "http";
        ushort port   = (ushort)(useTls ? 443 : 80);

        using var rest = new ItscamRestClient();
        rest.SetBaseUrl(host, port, scheme);
        if (useTls && insecure) rest.SetVerifyServerCertificate(false);

        Console.WriteLine($"Connecting to {scheme}://{host}:{port} ...");
        await rest.LoginAsync(user, pass);
        Console.WriteLine("REST login OK.");

        await DisableTriggerOnDayNightProfiles(rest);
        await ConfigureMjpegStreamSource(rest, framerate);
        await DisableAnalyticsAndOcr(rest);

        using var cgi = new ItscamCgiClient();
        cgi.SetBaseUrl(host, port, scheme);
        if (useTls && insecure) cgi.SetVerifyServerCertificate(false);

        Console.WriteLine($"\nMeasuring MJPEG frame rate over {duration} second(s)...");

        int    frames    = 0;
        object lockObj   = new object();
        var    startTime = DateTime.UtcNow;

        cgi.MjpegFrame += (_, frame) =>
        {
            lock (lockObj) { ++frames; }
        };

        cgi.StartMjpegStream();
        await Task.Delay(TimeSpan.FromSeconds(duration));
        cgi.StopMjpegStream();

        double elapsed = (DateTime.UtcNow - startTime).TotalSeconds;
        double fps     = elapsed > 0 ? frames / elapsed : 0;

        Console.WriteLine($"\nResult: {frames} frame(s) in {elapsed:F2}s  -->  {fps:F2} fps");
        return 0;
    }

    // ----------------------------------------------------------------
    // Helpers (typed read, partial PUT write)
    // ----------------------------------------------------------------

    static async Task DisableTriggerOnDayNightProfiles(ItscamRestClient rest)
    {
        Console.WriteLine("\n[REST] Fetching image profiles (typed read)...");
        List<ProfileConfig> profiles = await rest.GetProfilesAsync();
        Console.WriteLine($"  {profiles.Count} profile(s) returned.");

        var targeted = new List<ProfileConfig>();
        foreach (var p in profiles)
        {
            string name = p.Name ?? string.Empty;
            if (name.IndexOf("day",   StringComparison.OrdinalIgnoreCase) >= 0 ||
                name.IndexOf("night", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                targeted.Add(p);
            }
        }
        if (targeted.Count == 0)
        {
            Console.WriteLine("  Could not find day/night profiles by name; "
                + "applying to all profiles as fallback.");
            targeted.AddRange(profiles);
        }

        var patch = new ProfileConfig
        {
            Trigger = new Trigger { Enabled = false },
        };

        foreach (var profile in targeted)
        {
            string name = profile.Name ?? "(unnamed)";
            Console.WriteLine($"  Disabling trigger on profile '{name}' (id={profile.Id})...");

            await rest.UpdateProfileByIdAsync((int)profile.Id, patch);
            Console.WriteLine("    -> done.");
        }
    }

    static async Task ConfigureMjpegStreamSource(ItscamRestClient rest, int framerate)
    {
        Console.WriteLine($"\n[REST] Configuring MJPEG stream "
            + $"(useTriggerFrames=false, framerate={framerate})...");
        await rest.SetStreamConfigAsync(new StreamConfig
        {
            Mjpeg = new Mjpeg
            {
                Main = new MjpegMain
                {
                    UseTriggerFrames = false,
                    Framerate = framerate,
                },
            },
        });
        Console.WriteLine("  -> done.");
    }

    static async Task DisableAnalyticsAndOcr(ItscamRestClient rest)
    {
        Console.WriteLine("\n[REST] Disabling analytics (majority voting)...");
        await rest.SetAnalyticsConfigAsync(new AnalyticsConfig
        {
            Voting = new Voting { Enabled = false },
        });
        Console.WriteLine("  -> voting.enabled = false");

        Console.WriteLine("\n[REST] Disabling OCR...");
        await rest.SetOcrConfigAsync(new OcrConfig
        {
            Ocr = new OcrConfigOcr { Enabled = false },
        });
        Console.WriteLine("  -> ocr.enabled = false");
    }

    static int PrintUsage()
    {
        Console.Error.WriteLine(
            "Usage: dotnet run -- <host> --user <user> --password <pass>\n"
            + "                    [--https] [--insecure]\n"
            + "                    [--duration <seconds>]\n"
            + "                    [--framerate <fps>]\n"
            + "\n"
            + "  --user / --password   REST credentials (required)\n"
            + "  --https               use HTTPS instead of plain HTTP\n"
            + "  --insecure            skip TLS server certificate verification\n"
            + "  --duration            measurement window in seconds (default: 10)\n"
            + "  --framerate           MJPEG framerate to configure (default: 30)");
        return 1;
    }
}
