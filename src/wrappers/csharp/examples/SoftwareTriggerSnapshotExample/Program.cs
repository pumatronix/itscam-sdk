// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// Example: Software-Triggered Snapshot Loop
//
// Demonstrates the SDK's REST surfaces (when credentials are supplied):
//
//   * GetProfilesAsync() (typed) -- read-only inspection of profile names
//     and ids.
//   * PatchJsonAsync() (partial PUT) -- the correct way to change camera
//     configuration.  The ITSCAM daemon merges only the supplied fields;
//     sending back a full GET response body is rejected with HTTP 500 on
//     PUT /api/image/profiles/{id}.
//
// When --user and --password are supplied the example also:
//
//   1. Logs in to the REST API (REST always requires auth).
//   2. Patches day/night profiles: trigger off, 2 exposure steps.
//   3. Enables Jidosha OCR and the vehicle classifier.
//
// The snapshot loop always runs via snapshot.cgi (ItscamCgiClient).
// CGI authentication is optional on the camera's defaults
// (configCgi.blockAPI = false); pass --user/--password only when your
// camera has CGI auth enabled.
//
// Usage:
//     dotnet run -- <host> [--https] [--insecure] \
//                    [--user <user> --password <pass>] \
//                    [--count <n>] [--interval <ms>]
//
//   --https               use HTTPS instead of plain HTTP
//   --insecure            skip TLS server certificate verification (dev only)
//   --user / --password   opt-in credentials; REST config runs only when
//                         both are supplied
//   --count               number of snapshot rounds (default: run until Ctrl-C)
//   --interval            delay between rounds in milliseconds (default: 0)

using System;
using System.Collections.Generic;
using System.Text.Json.Nodes;
using System.Threading;
using System.Threading.Tasks;
using Pumatronix.Itscam;
using Pumatronix.Itscam.RestTypes;

class Program
{
    const int TargetExposureCount = 2;

    static async Task<int> Main(string[] args)
    {
        string host     = null;
        string user     = null;
        string pass     = null;
        bool   useTls   = false;
        bool   insecure = false;
        int    count    = -1;
        int    interval = 0;

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
                case "--count" when i + 1 < args.Length:
                    count = int.Parse(args[++i]); break;
                case "--interval" when i + 1 < args.Length:
                    interval = int.Parse(args[++i]); break;
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

        if (host == null)
            return PrintUsage();

        string scheme = useTls ? "https" : "http";
        ushort port   = (ushort)(useTls ? 443 : 80);
        bool   haveCreds = !string.IsNullOrEmpty(user) &&
                           !string.IsNullOrEmpty(pass);

        if (haveCreds)
        {
            using var rest = new ItscamRestClient();
            rest.SetBaseUrl(host, port, scheme);
            if (useTls && insecure) rest.SetVerifyServerCertificate(false);

            Console.WriteLine($"Connecting to {scheme}://{host}:{port} (REST)...");
            await rest.LoginAsync(user, pass);
            Console.WriteLine("REST login OK.");

            await ConfigureDayNightProfiles(rest);
            await EnableJidoshaAndClassifier(rest);
        }
        else
        {
            Console.WriteLine(
                "No credentials supplied; skipping REST configuration "
                + "(REST always requires authentication).");
        }

        using var cgi = new ItscamCgiClient();
        cgi.SetBaseUrl(host, port, scheme);
        if (useTls && insecure) cgi.SetVerifyServerCertificate(false);
        if (haveCreds)
        {
            await cgi.LoginAsync(user, pass);
            Console.WriteLine("CGI client logged in.");
        }
        else
        {
            Console.WriteLine(
                "CGI: anonymous mode (configCgi.blockAPI=false).");
        }

        Console.WriteLine(count < 0
            ? "\nStarting snapshot loop (press Ctrl-C to stop)...\n"
            : $"\nStarting snapshot loop ({count} round(s))...\n");

        using var cts = new CancellationTokenSource();
        Console.CancelKeyPress += (_, e) => { e.Cancel = true; cts.Cancel(); };

        int round = 0;
        while (!cts.IsCancellationRequested && (count < 0 || round < count))
        {
            ++round;
            Console.WriteLine($"--- Round {round} ---");

            var images = await cgi.GetSnapshotAsync(new SnapshotCgiRequest
            {
                Mosaic = false,
            });

            if (images == null || images.Count == 0)
            {
                Console.WriteLine("  (no images returned)");
            }
            else
            {
                Console.WriteLine($"  Received {images.Count} exposure(s):");
                for (int i = 0; i < images.Count; ++i)
                {
                    var img = images[i];
                    Console.WriteLine(
                        $"    Exposure {i + 1}/{images.Count}: "
                        + $"mime={img.MimeType}  size={img.Data.Length} bytes");
                }
            }

            if (interval > 0 && !cts.IsCancellationRequested)
                await Task.Delay(interval, cts.Token).ContinueWith(_ => { });
        }

        Console.WriteLine("\nDone.");
        return 0;
    }

    // ----------------------------------------------------------------
    // Helpers (typed read, partial PUT write)
    // ----------------------------------------------------------------

    static async Task ConfigureDayNightProfiles(ItscamRestClient rest)
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

        var steps = new JsonArray();
        for (int i = 0; i < TargetExposureCount; ++i)
            steps.Add(DefaultExposureStep());

        var patch = new JsonObject
        {
            ["trigger"] = new JsonObject { ["enabled"] = false },
            ["multipleExposures"] = new JsonObject
            {
                ["enabled"]  = true,
                ["settings"] = steps,
            },
        };

        foreach (var profile in targeted)
        {
            string name = profile.Name ?? "(unnamed)";
            Console.WriteLine($"  Configuring profile '{name}' (id={profile.Id})...");

            await rest.PatchJsonAsync(
                $"/api/image/profiles/{profile.Id}", patch);
            Console.WriteLine(
                $"    -> trigger disabled, {TargetExposureCount} exposures configured.");
        }
    }

    static JsonObject DefaultExposureStep() => new JsonObject
    {
        ["shutter"] = new JsonObject
        {
            ["percentageOfCurrent"] = true,
            ["value"]               = 100,
        },
        ["gain"] = new JsonObject
        {
            ["percentageOfCurrent"] = true,
            ["value"]               = 100,
        },
    };

    static async Task EnableJidoshaAndClassifier(ItscamRestClient rest)
    {
        Console.WriteLine("\n[REST] Enabling Jidosha OCR engine...");
        await rest.PatchJsonAsync("/api/equipment/ocr", new JsonObject
        {
            ["ocr"] = new JsonObject { ["enabled"] = true },
        });
        Console.WriteLine("  -> ocr.enabled = true");

        Console.WriteLine("\n[REST] Enabling vehicle Classifier...");
        await rest.PatchJsonAsync("/api/equipment/classifier", new JsonObject
        {
            ["classifier"] = new JsonObject { ["enabled"] = true },
        });
        Console.WriteLine("  -> classifier.enabled = true");
    }

    static int PrintUsage()
    {
        Console.Error.WriteLine(
            "Usage: SoftwareTriggerSnapshotExample <host> [--https] [--insecure]\n"
            + "                    [--user <user> --password <pass>]\n"
            + "                    [--count <n>] [--interval <ms>]\n"
            + "\n"
            + "  --https               use HTTPS instead of plain HTTP\n"
            + "  --insecure            skip TLS server certificate verification\n"
            + "  --user / --password   opt-in credentials; REST profile/OCR/classifier\n"
            + "                        configuration runs only when both are supplied\n"
            + "  --count               number of snapshot rounds (default: unlimited)\n"
            + "  --interval            delay between rounds in ms (default: 0)\n"
            + "\n"
            + "The snapshot loop uses snapshot.cgi and runs without credentials\n"
            + "by default (configCgi.blockAPI=false on the camera).");
        return 1;
    }
}
