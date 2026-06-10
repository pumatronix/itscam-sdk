// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// REST-only example that programs two camera profiles plus the MJPEG
// stream:
//
//   - "Diurno"  : continuous trigger every 150 ms, single exposure,
//                 shutter 120-500 us, gain 0-12 dB, iris always on.
//
//   - "Noturno" : same trigger / shutter / gain / iris envelope, but
//                 with two exposures - first flash 100 %, second 5 %.
//
//   - MJPEG    : 15 fps, "use trigger frames" enabled (triggered mode).
//
// After programming the camera, the example opens an ItscamCgiClient,
// subscribes to mjpegvideo.cgi for a few seconds and writes the first
// frame to disk so the operator can confirm the stream is alive.
//
// The MJPEG `useTriggerFrames` flag is not exposed by the generated
// REST type (`MjpegMain`), so we send it via PatchJsonAsync alongside
// the typed StreamConfig PUT.
//
// Usage:
//     dotnet run -- <host> --user U --password P [--https] [--insecure] \
//                    [--day-profile NAME] [--night-profile NAME] \
//                    [--stream-seconds N]

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json.Nodes;
using System.Threading;
using System.Threading.Tasks;
using Pumatronix.Itscam;
using Pumatronix.Itscam.RestTypes;

class Program
{
    // Display-to-storage multipliers used by the camera webapp.
    //
    //   shutter is stored in microseconds (no scaling).
    //   gain    is stored as deciBels * 100  (12 dB -> 1200).
    private const long GainScale = 100;

    static async Task<int> Main(string[] args)
    {
        string? host = null;
        string? user = null;
        string? pass = null;
        string  dayProfile   = "Diurno";
        string  nightProfile = "Noturno";
        int     streamSeconds = 5;
        bool    useTls   = false;
        bool    insecure = false;

        for (int i = 0; i < args.Length; ++i)
        {
            string a = args[i];
            switch (a)
            {
                case "--https":    useTls   = true; break;
                case "--insecure": insecure = true; break;
                case "--user" when i + 1 < args.Length:
                    user = args[++i]; break;
                case "--password" when i + 1 < args.Length:
                    pass = args[++i]; break;
                case "--day-profile" when i + 1 < args.Length:
                    dayProfile = args[++i]; break;
                case "--night-profile" when i + 1 < args.Length:
                    nightProfile = args[++i]; break;
                case "--stream-seconds" when i + 1 < args.Length:
                    if (!int.TryParse(args[++i], out streamSeconds) ||
                        streamSeconds < 0)
                    {
                        Console.Error.WriteLine(
                            "--stream-seconds expects a non-negative integer");
                        return PrintUsage();
                    }
                    break;
                default:
                    if (host == null) host = a;
                    else
                    {
                        Console.Error.WriteLine($"Unexpected argument: {a}");
                        return PrintUsage();
                    }
                    break;
            }
        }

        if (host == null || string.IsNullOrEmpty(user)
                         || string.IsNullOrEmpty(pass))
        {
            return PrintUsage();
        }

        string scheme = useTls ? "https" : "http";
        ushort port   = (ushort)(useTls ? 443 : 80);

        using var rest = new ItscamRestClient();
        rest.SetBaseUrl(host, port, scheme);
        if (useTls && insecure) rest.SetVerifyServerCertificate(false);

        await rest.LoginAsync(user!, pass!);
        Console.WriteLine($"REST login OK on {scheme}://{host}:{port}");

        // ---- Discover the profiles by name ----------------------------
        var profiles = await rest.GetProfilesAsync();
        Console.WriteLine($"Found {profiles.Count} profile(s):");
        foreach (var p in profiles)
            Console.WriteLine($"  id={p.Id,-2} name=\"{p.Name}\"");

        await ConfigureDayProfileAsync(rest, profiles, dayProfile);
        await ConfigureNightProfileAsync(rest, profiles, nightProfile);
        await ConfigureMjpegStreamAsync(rest);

        if (streamSeconds > 0)
        {
            await ConsumeMjpegStreamAsync(
                host!, port, scheme, useTls && insecure,
                user!, pass!, streamSeconds);
        }

        Console.WriteLine("Done.");
        return 0;
    }

    // -------- Consume the freshly configured MJPEG stream ------------
    static async Task ConsumeMjpegStreamAsync(
        string host, ushort port, string scheme, bool insecure,
        string user, string pass, int seconds)
    {
        Console.WriteLine($"\nStreaming MJPEG for {seconds} second(s)...");

        using var cgi = new ItscamCgiClient();
        cgi.SetBaseUrl(host, port, scheme);
        if (insecure) cgi.SetVerifyServerCertificate(false);

        // CGI auth is normally disabled (configCgi.blockAPI=false).  We
        // still log in here so the same credentials work even on cameras
        // where the operator has opted CGI auth in.
        try { await cgi.LoginAsync(user, pass); }
        catch (ItscamException ex)
        {
            Console.WriteLine($"  (CGI login skipped: {ex.Message})");
        }

        int frames = 0;
        long bytes = 0;
        string outDir = Path.Combine(Environment.CurrentDirectory,
                                     "mjpeg-frames");
        Directory.CreateDirectory(outDir);

        cgi.MjpegFrame += (_, frame) =>
        {
            int n = Interlocked.Increment(ref frames);
            Interlocked.Add(ref bytes, frame.Data.LongLength);
            if (n == 1)
            {
                string path = Path.Combine(outDir, "frame-first.jpg");
                File.WriteAllBytes(path, frame.Data);
                Console.WriteLine(
                    $"  first frame: {frame.Data.Length} bytes "
                    + $"({frame.MimeType}) -> {path}");
            }
        };

        cgi.StartMjpegStream();
        try
        {
            await Task.Delay(TimeSpan.FromSeconds(seconds));
        }
        finally
        {
            cgi.StopMjpegStream();
        }

        double effectiveFps = frames / (double)seconds;
        Console.WriteLine(
            $"  received {frames} frame(s), {bytes:N0} bytes total, "
            + $"~{effectiveFps:F2} fps.");
    }

    // -------- Diurno: 1 exposure, continuous trigger ------------------
    static async Task ConfigureDayProfileAsync(
        ItscamRestClient rest, List<ProfileConfig> profiles, string name)
    {
        long id = FindProfileId(profiles, name);
        Console.WriteLine($"\nConfiguring \"{name}\" (id={id})...");

        var update = new ProfileConfig
        {
            Trigger = BuildContinuousTrigger(),
            Exposure = BuildExposureEnvelope(),
            MultipleExposures = new MultipleExposures
            {
                Enabled = false,
            },
        };

        await rest.UpdateProfileByIdAsync((int)id, update);
        Console.WriteLine($"  -> \"{name}\" updated (single exposure).");
    }

    // -------- Noturno: 2 exposures, flash 100% then 5% ----------------
    static async Task ConfigureNightProfileAsync(
        ItscamRestClient rest, List<ProfileConfig> profiles, string name)
    {
        long id = FindProfileId(profiles, name);
        Console.WriteLine($"\nConfiguring \"{name}\" (id={id})...");

        var update = new ProfileConfig
        {
            Trigger = BuildContinuousTrigger(),
            Exposure = BuildExposureEnvelope(),
            MultipleExposures = new MultipleExposures
            {
                Enabled = true,
                Settings = new List<MultipleExposuresConfig>
                {
                    BuildExposureSetting(flashPercent: 100),
                    BuildExposureSetting(flashPercent: 5),
                },
            },
        };

        await rest.UpdateProfileByIdAsync((int)id, update);
        Console.WriteLine($"  -> \"{name}\" updated (2 exposures, "
                          + "flash 100% / 5%).");
    }

    // -------- MJPEG stream: 15 fps + useTriggerFrames=true ------------
    static async Task ConfigureMjpegStreamAsync(ItscamRestClient rest)
    {
        Console.WriteLine("\nConfiguring MJPEG stream...");

        // Typed PUT for the fields covered by the generated schema.
        var streamUpdate = new StreamConfig
        {
            Mjpeg = new Mjpeg
            {
                Main = new MjpegMain
                {
                    Enabled   = true,
                    Framerate = 15.0,
                },
            },
        };
        await rest.SetStreamConfigAsync(streamUpdate);

        // `useTriggerFrames` is supported by the camera-daemon but is
        // not yet part of the typed MJPEG schema in the SDK.  Send the
        // additional field as a JSON patch on the same endpoint.
        var patch = new JsonObject
        {
            ["mjpeg"] = new JsonObject
            {
                ["main"] = new JsonObject
                {
                    ["useTriggerFrames"] = true,
                },
            },
        };
        await rest.PatchJsonAsync("/api/video/streams", patch);

        Console.WriteLine("  -> MJPEG: 15 fps, useTriggerFrames=true.");
    }

    // ---- Helpers -----------------------------------------------------

    static long FindProfileId(List<ProfileConfig> profiles, string name)
    {
        foreach (var p in profiles)
        {
            if (string.Equals(p.Name, name, StringComparison.Ordinal))
                return p.Id;
        }
        throw new ItscamInvalidParameterException(
            $"profile not found on camera: \"{name}\"");
    }

    static Trigger BuildContinuousTrigger() => new Trigger
    {
        Enabled         = true,
        Event           = "constant",   // free-running / continuous trigger
        MinimumInterval = 150,          // milliseconds between captures
    };

    // Shutter 120-500 us, gain 0-12 dB, iris always on (auto).
    static Exposure BuildExposureEnvelope() => new Exposure
    {
        Shutter = new ShutterClass
        {
            Automatic = true,
            MinValue  = 120,
            MaxValue  = 500,
        },
        Gain = new ShutterClass
        {
            Automatic = true,
            MinValue  = 0,
            MaxValue  = 12 * GainScale,
        },
        Iris = new ExposureIris
        {
            Automatic = true,
        },
    };

    // Per-exposure entry: keep shutter and gain at 100% of the profile
    // envelope; the two exposures only differ in flash power.
    static MultipleExposuresConfig BuildExposureSetting(long flashPercent)
        => new MultipleExposuresConfig
        {
            Shutter = new Shutter
            {
                PercentageOfCurrent = true,
                Value               = 100.0,
            },
            Gain = new SettingGain
            {
                PercentageOfCurrent = true,
                Value               = 100.0,
            },
            Flash = new Flash
            {
                Power = new List<Power>
                {
                    new Power { Out = 0, Percent = flashPercent },
                },
            },
        };

    static int PrintUsage()
    {
        Console.Error.WriteLine(
            "Usage: dotnet run -- <host> --user U --password P "
            + "[--https] [--insecure] "
            + "[--day-profile NAME] [--night-profile NAME] "
            + "[--stream-seconds N]\n"
            + "\n"
            + "  --https           use HTTPS instead of plain HTTP\n"
            + "  --insecure        skip TLS server certificate verification\n"
            + "  --user/--password REST credentials (always required)\n"
            + "  --day-profile     day profile name (default: Diurno)\n"
            + "  --night-profile   night profile name (default: Noturno)\n"
            + "  --stream-seconds  MJPEG capture duration in seconds "
            + "(default: 5, 0 disables)");
        return 1;
    }
}
