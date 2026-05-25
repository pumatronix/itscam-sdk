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
//   - ItscamRestClient  (login + read config)
//                       -- only exercised when credentials are supplied,
//                          because the REST API always requires auth.
//
// Usage:
//     dotnet run -- <host> [--https] [--insecure] \
//                    [--user U --password P]
//
// The example reads 5 seconds of MJPEG frames and writes the first
// snapshot frame to ./snapshot-0.jpg.

using System;
using System.IO;
using System.Threading.Tasks;
using Pumatronix.Itscam;

class Program
{
    static async Task<int> Main(string[] args)
    {
        string? host = null;
        string? user = null;
        string? pass = null;
        bool   useTls   = false;
        bool   insecure = false;

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
            Console.WriteLine($"REST login OK; volatile info:");
            Console.WriteLine(
                await rest.GetAsync("/api/equipment/misc/readonly/volatile"));
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
            + "[--user U --password P]\n"
            + "\n"
            + "  --https           use HTTPS instead of plain HTTP\n"
            + "  --insecure        skip TLS server certificate verification\n"
            + "  --user/--password opt-in CGI/REST credentials (REST is\n"
            + "                    only exercised when both are supplied)");
        return 1;
    }
}
