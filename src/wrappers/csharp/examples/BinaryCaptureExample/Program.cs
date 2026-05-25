// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// Binary TCP client example (port 60000): connect, optionally authenticate,
// subscribe to captures, request a snapshot, and wait briefly for callbacks.
//
// Usage:
//     dotnet run -- <host> [password]

using System;
using System.Threading.Tasks;
using Pumatronix.Itscam;

class Program
{
    static async Task<int> Main(string[] args)
    {
        if (args.Length < 1 || args[0] == "--help" || args[0] == "-h")
            return PrintUsage();

        string host     = args[0];
        string? password = args.Length >= 2 ? args[1] : null;

        int snapshotCallbacks = 0;

        using var client = new ItscamClient();
        client.SnapshotImage += (_, result) =>
        {
            ++snapshotCallbacks;
            Console.WriteLine(
                $"  snapshot callback #{snapshotCallbacks}: "
                + $"{result.Jpeg.Length} bytes");
        };

        Console.WriteLine($"Connecting to {host}:60000 ...");
        await client.ConnectAsync(host, 60000, 10000,
            new AutoReconnectConfig { Enabled = true, IntervalMs = 3000 });

        if (!string.IsNullOrEmpty(password))
        {
            await client.AuthenticateAsync(password);
            Console.WriteLine("Authenticated.");
        }

        await client.SubscribeCapturesAsync();
        var results = await client.CaptureSnapshotAsync();
        Console.WriteLine($"capture_snapshot returned {results.Count} frame(s)");
        for (int i = 0; i < results.Count; ++i)
            Console.WriteLine(
                $"  frame {i + 1}: {results[i].Jpeg.Length} bytes");

        var profiles = await client.ListProfilesAsync();
        Console.WriteLine($"list_profiles returned {profiles.Count} profile(s)");
        foreach (var p in profiles)
        {
            Console.WriteLine(
                $"  id={p.Id} active={p.IsActive} name={p.Name}");
        }

        Console.WriteLine("Waiting briefly for trigger frames ...");
        await Task.Delay(2000);

        Console.WriteLine($"Done ({snapshotCallbacks} snapshot callback(s)).");
        return 0;
    }

    static int PrintUsage()
    {
        Console.Error.WriteLine(
            "Usage: dotnet run -- <host> [password]\n"
            + "\n"
            + "  host       Camera IP or hostname (required)\n"
            + "  password   Binary TCP auth password (optional)");
        return 1;
    }
}
