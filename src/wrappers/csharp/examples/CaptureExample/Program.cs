// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// End-to-end example exercising the two ITSCAM SDK HTTP surfaces:
//
//   - ItscamCgiClient   (lastframe.cgi, snapshot.cgi, mjpegvideo.cgi)
//                       -- always exercised; credentials are optional
//                          because configCgi.blockAPI defaults to false
//                          on the camera.
//
//   - ItscamRestClient  (login + typed config read/write)
//                       -- only exercised when credentials are supplied,
//                          because the REST API always requires auth.
//                          Uses GetVolatileInfoAsync() for status and
//                          UpdateProfileByNameAsync() to push partial
//                          updates to the default Diurno (day) and
//                          Noturno (night) profiles.
//
// Usage:
//     dotnet run -- <host> [--https] [--insecure] \
//                    [--user U --password P] \
//                    [--day-profile NAME] [--night-profile NAME]
//
// The example reads 5 seconds of MJPEG frames and writes the first
// snapshot frame to ./snapshot-0.jpg.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Pumatronix.Itscam;
using Pumatronix.Itscam.RestTypes;

class Program
{
    static async Task<int> Main(string[] args)
    {
        string? host = null;
        string? user = null;
        string? pass = null;
        string  dayProfile   = "Diurno";
        string  nightProfile = "Noturno";
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

        if (host == null) return PrintUsage();

        string scheme = useTls ? "https" : "http";
        ushort port   = (ushort)(useTls ? 443 : 80);
        bool   haveCreds = !string.IsNullOrEmpty(user) &&
                           !string.IsNullOrEmpty(pass);

        // -------- REST client (auth-only) --------
        if (haveCreds)
        {
            using var rest = new ItscamRestClient();
            rest.SetBaseUrl(host, port, scheme);
            if (useTls && insecure) rest.SetVerifyServerCertificate(false);
            await rest.LoginAsync(user, pass);
            Console.WriteLine("REST login OK.");

            // -- Typed status: GetVolatileInfoAsync returns MiscVolatile --
            var volatileInfo = await rest.GetVolatileInfoAsync();
            Console.WriteLine("Volatile equipment info:");
            Console.WriteLine(
                $"  active profile : id={volatileInfo.Profile?.Id} "
                + $"name=\"{volatileInfo.Profile?.Name}\"");
            if (volatileInfo.Fps?.Mjpeg is double mjpegFps)
                Console.WriteLine($"  MJPEG fps      : {mjpegFps:F2}");
            if (volatileInfo.Ae?.Level is double aeLevel)
                Console.WriteLine($"  AE level       : {aeLevel:F2} "
                                  + $"({volatileInfo.Ae.CtrlMode})");
            if (volatileInfo.Isp != null)
                Console.WriteLine(
                    $"  ISP            : shutter={volatileInfo.Isp.Shutter} "
                    + $"gain={volatileInfo.Isp.Gain} "
                    + $"iris={volatileInfo.Isp.Iris}");
            if (volatileInfo.Lens != null)
                Console.WriteLine(
                    $"  lens           : focus={volatileInfo.Lens.Focus} "
                    + $"zoom={volatileInfo.Lens.Zoom}");

            // -- Typed config: list available profiles before updating --
            var profiles = await rest.GetProfilesAsync();
            Console.WriteLine($"Found {profiles.Count} profile(s) on camera:");
            foreach (var p in profiles)
                Console.WriteLine($"  id={p.Id,-2} name=\"{p.Name}\" "
                                  + $"active={p.Active}");

            // -- Typed config: push partial updates by name --
            // Construct a ProfileConfig with only the fields we want to
            // change; null fields are stripped from the PUT body.
            await ConfigureDayProfileAsync(rest, profiles, dayProfile);
            await ConfigureNightProfileAsync(rest, profiles, nightProfile);
        }
        else
        {
            Console.WriteLine(
                "No credentials supplied; skipping the REST section "
                + "(REST always requires authentication).");
        }

        // -------- CGI client (auth optional) --------
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

        Console.WriteLine("Fetching lastframe.cgi...");
        var last = await cgi.GetLastFrameAsync();
        await File.WriteAllBytesAsync("lastframe.jpg", last.Data);
        Console.WriteLine($"  -> wrote {last.Data.Length} bytes to lastframe.jpg");

        Console.WriteLine("Triggering snapshot.cgi (multi-exposure mosaic off)...");
        var snap = await cgi.GetSnapshotAsync(new SnapshotCgiRequest
        {
            Quality = 80,
            Mosaic  = false,
        });
        Console.WriteLine($"  -> received {snap.Count} image(s)");
        for (int i = 0; i < snap.Count; ++i)
        {
            string path = $"snapshot-{i}.jpg";
            await File.WriteAllBytesAsync(path, snap[i].Data);
            Console.WriteLine($"     {path}: {snap[i].Data.Length} bytes "
                              + $"({snap[i].MimeType})");
        }

        Console.WriteLine("Streaming MJPEG for 5 seconds...");
        int frames = 0;
        cgi.MjpegFrame += (_, frame) =>
        {
            ++frames;
            if (frames == 1)
                File.WriteAllBytes("mjpeg-first.jpg", frame.Data);
        };
        cgi.StartMjpegStream();
        await Task.Delay(5000);
        cgi.StopMjpegStream();
        Console.WriteLine($"  -> received {frames} MJPEG frame(s)");

        return 0;
    }

    static int PrintUsage()
    {
        Console.Error.WriteLine(
            "Usage: dotnet run -- <host> [--https] [--insecure] "
            + "[--user U --password P] "
            + "[--day-profile NAME] [--night-profile NAME]\n"
            + "\n"
            + "  --https           use HTTPS instead of plain HTTP\n"
            + "  --insecure        skip TLS server certificate verification\n"
            + "  --user/--password opt-in CGI/REST credentials (REST is\n"
            + "                    only exercised when both are supplied)\n"
            + "  --day-profile     day profile name to configure "
            + "(default: Diurno)\n"
            + "  --night-profile   night profile name to configure "
            + "(default: Noturno)");
        return 1;
    }

    // ---- Typed REST helpers --------------------------------------------

    // Day profile -- single exposure, HDR off, light overlay.  Suitable
    // for daytime captures with ambient light only (no flash).  Only the
    // listed fields are sent in the PUT body; everything else is left
    // untouched on the camera.
    static async Task ConfigureDayProfileAsync(
        ItscamRestClient rest,
        IReadOnlyList<ProfileConfig> existing,
        string name)
    {
        if (!existing.Any(p => p.Name == name))
        {
            Console.WriteLine(
                $"Day profile \"{name}\" not present on camera; skipping.");
            return;
        }

        var update = new ProfileConfig
        {
            MultipleExposures = new MultipleExposures { Enabled = false },
            Hdr               = new Hdr               { Enable  = false },
            Overlay           = new Overlay
            {
                Enable = true,
                Text   = "Diurno",
            },
        };

        var result = await rest.UpdateProfileByNameAsync(name, update);
        Console.WriteLine(
            $"Day profile \"{name}\" (id={result.Id}) updated: "
            + "single exposure, HDR off, overlay \"Diurno\".");
    }

    // Night profile -- two-step multi-exposure with the on-board flash
    // at different power levels, HDR enabled, overlay text "Noturno".
    // Step 1 is meant to expose for the scene; step 2 (lower flash)
    // captures the plate without saturating retro-reflective surfaces.
    static async Task ConfigureNightProfileAsync(
        ItscamRestClient rest,
        IReadOnlyList<ProfileConfig> existing,
        string name)
    {
        if (!existing.Any(p => p.Name == name))
        {
            Console.WriteLine(
                $"Night profile \"{name}\" not present on camera; skipping.");
            return;
        }

        var step1 = new MultipleExposuresConfig
        {
            Flash = new Flash
            {
                Power = new List<Power>
                {
                    new Power { Out = 1, Percent = 80 },
                },
            },
            Shutter = new Shutter { PercentageOfCurrent = true, Value = 100 },
            Gain    = new SettingGain
                      { PercentageOfCurrent = true, Value = 100 },
        };

        var step2 = new MultipleExposuresConfig
        {
            Flash = new Flash
            {
                Power = new List<Power>
                {
                    new Power { Out = 1, Percent = 40 },
                },
            },
            Shutter = new Shutter { PercentageOfCurrent = true, Value = 100 },
            Gain    = new SettingGain
                      { PercentageOfCurrent = true, Value = 100 },
        };

        var update = new ProfileConfig
        {
            MultipleExposures = new MultipleExposures
            {
                Enabled  = true,
                Settings = new List<MultipleExposuresConfig> { step1, step2 },
            },
            Hdr     = new Hdr     { Enable = true },
            Overlay = new Overlay
            {
                Enable = true,
                Text   = "Noturno",
            },
        };

        var result = await rest.UpdateProfileByNameAsync(name, update);
        Console.WriteLine(
            $"Night profile \"{name}\" (id={result.Id}) updated: "
            + "2 multi-exposure steps (flash 80%/40%), HDR on, "
            + "overlay \"Noturno\".");
    }
}
