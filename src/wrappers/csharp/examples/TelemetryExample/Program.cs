// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// System telemetry example.  Reads the state of the whole camera once, then
// follows the documents the camera pushes for a while.
//
// Usage:
//   dotnet run -- <host> <user> <password> [--seconds N] [--https] [--insecure]

using System;
using System.Linq;
using System.Threading.Tasks;
using Pumatronix.Itscam;
using Pumatronix.Itscam.RestTypes;

internal static class Program
{
    private static void PrintHealth(SystemTelemetry telemetry)
    {
        var health = telemetry.Health;
        if (health == null)
        {
            Console.WriteLine("health: unavailable");
            return;
        }
        Console.WriteLine($"health: {health.Status}");
        if (health.Issues == null) return;
        foreach (var issue in health.Issues)
            Console.WriteLine($"  [{issue.Severity}] {issue.Id}: {issue.Message}");
    }

    private static async Task<int> Main(string[] args)
    {
        if (args.Length < 3)
        {
            Console.Error.WriteLine("Usage: TelemetryExample <host> <user> <password> "
                                    + "[--seconds N] [--https] [--insecure]");
            return 1;
        }
        var flags = args.Skip(3).ToList();
        bool useHttps = flags.Contains("--https");
        bool insecure = flags.Contains("--insecure");
        int secondsAt = flags.IndexOf("--seconds");
        int seconds = secondsAt >= 0 ? int.Parse(flags[secondsAt + 1]) : 30;

        using var rest = new ItscamRestClient();
        rest.SetBaseUrl(args[0], (ushort)(useHttps ? 443 : 80),
                        useHttps ? "https" : "http");
        if (useHttps && insecure) rest.SetVerifyServerCertificate(false);
        await rest.LoginAsync(args[1], args[2]);

        // Polling: one read of the whole camera
        PrintHealth(await rest.GetSystemTelemetryAsync());

        // Stream: one document at once, then one every 5 s.  Handlers run on
        // the SDK's callback pool: do not block in them.
        rest.TelemetryReceived += (_, update) =>
        {
            Console.WriteLine($"\nupdate #{update.Sequence}");
            PrintHealth(update.GetTelemetry());
        };
        rest.StartTelemetryStream();
        await Task.Delay(TimeSpan.FromSeconds(seconds));
        rest.StopTelemetryStream();

        var latest = rest.LatestTelemetry();
        if (latest != null)
            Console.WriteLine($"\nlast document: #{latest.Sequence}");
        return 0;
    }
}
