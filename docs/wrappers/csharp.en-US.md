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

## Install

### From the pre-compiled SDK package (recommended)

The distribution package (`itscam-sdk-<version>.tar.gz`) includes a multi-RID NuGet ready to consume. Download from the [releases page](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>
cat > nuget.config <<EOF
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="itscam-sdk" value="$SDK/csharp" />
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
  </packageSources>
</configuration>
EOF
ITSCAM_VERSION=$(sed -n 's/.*"nugetVersion"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$SDK/VERSION.json")
dotnet add package Pumatronix.Itscam.Sdk --version "$ITSCAM_VERSION"
```

The NuGet already contains native binaries for linux-x64, linux-arm, linux-arm64, win-x64, and win-x86 (each one when the respective cross-toolchain was available at pack time). The MSBuild target file automatically copies the correct native binary to the build output.

### Build from source (advanced)

If you are developing inside the SDK source tree:

```bash
make csharp              # builds Itscam.Sdk.dll for the host platform
make csharp-pack         # builds native binaries + produces a NuGet
```

`make csharp-pack` produces `src/wrappers/csharp/nupkg/Pumatronix.Itscam.Sdk.<version>.nupkg` containing the managed assembly and a per-**host RID** native binary under `runtimes/<rid>/native/`. The RID describes the machine where the .NET application runs (Linux x64, Windows x64, etc.) -- **not** the ITSCAM camera, which is reached over the network via REST/CGI/binary and does not host your app.

`make csharp-pack` auto-detects the available cross-toolchains and runs `make lib-arm` / `make lib-arm64` / `make windows` before packing -- so the final NuGet includes:

- `linux-x64` -- produced by `make lib`
- `linux-arm` -- produced by `make lib-arm` (Arm GNU-A 8.3-2019.03 cross; ITSCAM450)
- `linux-arm64` -- produced by `make lib-arm64` (Arm GNU-A 8.3-2019.03 cross; ITSCAM600)
- `win-x64`, `win-x86` -- produced by `make windows` (MinGW cross-compile)

Any RID whose toolchain is not on `PATH` is silently omitted from the final package.

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

// Configuration changes: typed setter with partial serialization (preferred).
var patch = new ProfileConfig { Trigger = new TriggerConfig { Enabled = false } };
await rest.UpdateProfileByIdAsync(0, patch);

// Alternative: generic patchJson for untyped payloads.
// await rest.PatchJsonAsync("/api/image/profiles/0",
//     new JsonObject { ["trigger"] = new JsonObject { ["enabled"] = false } });

// Raw JSON for endpoints not yet promoted to typed helpers:
string json = await rest.GetAsync("/api/equipment/misc/readonly/volatile");
```

Typed setters use **partial serialization** -- only fields explicitly set on the struct are included in the PUT body. The daemon merges the supplied fields into the existing config. The generic `PatchJsonAsync` remains available for untyped payloads or endpoints without a typed helper. See `docs/api/rest-client.md` for details.

## Example project

A complete end-to-end example lives at [`src/wrappers/csharp/examples/CaptureExample/`](../../src/wrappers/csharp/examples/CaptureExample/):

```bash
cd src/wrappers/csharp/examples/CaptureExample
dotnet run -- 192.168.254.254
dotnet run -- 192.168.254.254 --https --insecure --user admin --password 1234
```

CGI is always exercised; the REST section is skipped when no credentials are supplied (REST always requires auth).

For an **equipment-configuration** focused scenario (program the Diurno/Noturno profiles with continuous trigger, tune the MJPEG stream, then consume its frames), see [`src/wrappers/csharp/examples/ProfileConfigExample/`](../../src/wrappers/csharp/examples/ProfileConfigExample/):

```bash
cd src/wrappers/csharp/examples/ProfileConfigExample
dotnet run -- 192.168.254.254 --user admin --password 1234 \
              --day-profile Diurno --night-profile Noturno \
              --stream-seconds 5
```
