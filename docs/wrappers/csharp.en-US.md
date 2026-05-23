# C# / .NET wrapper

[Português (Brasil)](csharp.md) | [English (US)](csharp.en-US.md)

The C# wrapper lives at [`src/wrappers/csharp/`](../../src/wrappers/csharp/) and targets **netstandard2.0**, which makes it consumable from:

> **Full class, method, and XML doc reference**: [DocFX for `Pumatronix.Itscam.Sdk`](/api-ref/csharp/index.html). This page covers installation, idiomatic patterns, and examples.


- .NET 6 / 7 / 8 / 9
- .NET Framework 4.6.1+
- Mono / Xamarin / Unity (any runtime that speaks netstandard2.0)

It wraps all three SDK surfaces:

| Class             | Native counterpart                            |
| ----------------- | --------------------------------------------- |
| `ItscamClient`    | `src/core/itscam_client.h`                    |
| `ItscamRestClient`| `src/core/itscam_rest_client.h`               |
| `ItscamCgiClient` | `src/core/itscam_cgi_client.h`                |

## Build

```bash
make csharp              # builds Itscam.Sdk.dll for the host platform
make csharp-pack         # builds native binaries + produces a NuGet
```

`make csharp-pack` produces `src/wrappers/csharp/nupkg/Pumatronix.Itscam.Sdk.<version>.nupkg` containing the managed assembly and a per-**host RID** native binary under `runtimes/<rid>/native/`. The RID describes the machine where the .NET application runs (Linux x64, Windows x64, etc.) -- **not** the ITSCAM camera, which is reached over the network via REST/CGI/ binary and does not host your app.

By default the pack ships the RIDs that the `Makefile` actually builds:

- `linux-x64` -- produced by `make lib`
- `win-x64`, `win-x86` -- produced by `make windows` (MinGW cross-compile)

Additional slots exist in [`Itscam.Sdk.csproj`](../../src/wrappers/csharp/Itscam.Sdk/Itscam.Sdk.csproj) and are packed *only if* you produce the binaries yourself:

- `linux-arm` -- requires `src/core/build/linux-arm/libitscam_sdk.so.*`
- `linux-arm64` -- requires `src/core/build/linux-arm64/libitscam_sdk.so.*`

The bundled toolchain does not yet compile those variants; see the "ARM toolchains may be wired in" comment in the [`Makefile`](../../Makefile) if you need to add them.

Detailed wrapper-specific notes (P/Invoke conventions, native binary layout, MSBuild target file) live in [`src/wrappers/csharp/README.md`](../../src/wrappers/csharp/README.md).

## Idiomatic surface

| Concern        | What you get                                                                 |
| -------------- | ---------------------------------------------------------------------------- |
| Async          | `Task` / `Task<T>` on every blocking call (work runs on the ThreadPool).     |
| Lifetime       | Every client implements `IDisposable`; use `using` or `Dispose()`.           |
| Streaming      | `event EventHandler<CgiStreamFrame> MjpegFrame` raised on the SDK worker.    |
| Errors         | `ItscamException` hierarchy (`ItscamTimeoutException`, `ItscamAuthException`, ...). |
| Strings        | UTF-8 marshalling helpers, safe on netstandard2.0 (no .NET 6+ APIs).         |

## CGI usage (auth optional)

```csharp
using Pumatronix.Itscam;

using var cgi = new ItscamCgiClient();
cgi.SetBaseUrl("192.168.254.254", 80);
// cgi.SetBaseUrl("camera.example.com", 443, "https");
// cgi.SetVerifyServerCertificate(false);   // demo / dev only

// Optional: only when the camera enables CGI auth.
// await cgi.LoginAsync("admin", "1234");

var last = await cgi.GetLastFrameAsync();
File.WriteAllBytes("lastframe.jpg", last.Data);

var snap = await cgi.GetSnapshotAsync(new SnapshotCgiRequest {
    Quality = 80, Mosaic = false,
});

cgi.MjpegFrame += (_, frame) => { /* runs on SDK worker */ };
cgi.StartMjpegStream();
await Task.Delay(5000);
cgi.StopMjpegStream();
```

## REST usage (auth required)

```csharp
using var rest = new ItscamRestClient();
rest.SetBaseUrl("192.168.254.254", 80);
await rest.LoginAsync("admin", "1234");

// Read-only inspection: typed POCO is convenient.
List<ProfileConfig> profiles = await rest.GetProfilesAsync();

// Configuration changes: partial PUT (send only what you change).
await rest.PatchJsonAsync("/api/image/profiles/0",
    new JsonObject { ["trigger"] = new JsonObject { ["enabled"] = false } });

// Raw JSON for endpoints not yet promoted to typed helpers:
string json = await rest.GetAsync("/api/equipment/misc/readonly/volatile");
```

`PatchJsonAsync` PUTs a partial JSON document; the daemon merges the supplied fields into the existing config. Do **not** GET a full profile and PUT it back -- `PUT /api/image/profiles/{id}` rejects full-document bodies with HTTP 500. See `docs/api/rest-client.md` for details.

## Example project

A complete end-to-end example lives at [`src/wrappers/csharp/examples/CaptureExample/`](../../src/wrappers/csharp/examples/CaptureExample/):

```bash
cd src/wrappers/csharp/examples/CaptureExample
dotnet run -- 192.168.254.254
dotnet run -- 192.168.254.254 --https --insecure --user admin --password 1234
```

CGI is always exercised; the REST section is skipped when no credentials are supplied (REST always requires auth).
