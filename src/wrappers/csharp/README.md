# Itscam.Sdk (.NET wrapper)

Idiomatic C# / .NET wrapper for the ITSCAM Client SDK.  Targets
**.NET Standard 2.0**, which makes the package usable from:

- .NET 6 / 7 / 8 / 9
- .NET Framework 4.6.1+
- Mono / Xamarin / Unity (any runtime that speaks netstandard2.0)

The wrapper supports the full SDK surface:

| Client                | Wraps                           | Use case |
| --------------------- | ------------------------------- | ----------------------------------- |
| `ItscamClient`        | `src/core/itscam_client.h`      | binary protocol on TCP 60000        |
| `ItscamRestClient`    | `src/core/itscam_rest_client.h` | JSON REST API on the webapp backend |
| `ItscamCgiClient`     | `src/core/itscam_cgi_client.h`  | snapshot.cgi / mjpegvideo.cgi / etc |

## Layout

```
src/wrappers/csharp/
  Itscam.Sdk/                    # main library project (netstandard2.0)
    Itscam.Sdk.csproj
    Native/NativeMethods.cs      # P/Invoke surface
    ItscamClient.cs
    ItscamRestClient.cs
    ItscamCgiClient.cs
    Types.cs                     # SnapshotCgiRequest, CgiImage, ...
    Exceptions.cs                # ItscamException hierarchy
    build/Itscam.Sdk.targets     # native-binary copy logic
  examples/CaptureExample/       # net6.0 console example
  Itscam.Sdk.sln
  README.md
```

## Build

```bash
cd src/wrappers/csharp
dotnet build -c Release             # build everything
dotnet build -c Release Itscam.Sdk  # build only the library
dotnet pack  -c Release Itscam.Sdk  # produce Itscam.Sdk.<ver>.nupkg
```

`dotnet pack` embeds the native binaries from `src/core/build/<rid>/`
into `runtimes/<rid>/native/` inside the NuGet, so consumers get a
single package with binaries for every supported platform.  The
top-level `Makefile` provides a `csharp-pack` target that builds the
native artifacts first and then runs `dotnet pack`.

## HTTPS

Use the `--https` flag in the example or set `scheme = "https"` in your
own code:

```csharp
using var cgi = new ItscamCgiClient();
cgi.SetBaseUrl("camera.example.com", 443, "https");
cgi.SetCaCertFile("/etc/ssl/certs/ca-bundle.pem"); // or SetCaCertData
await cgi.LoginAsync("admin", "secret");
var image = await cgi.GetLastFrameAsync();
File.WriteAllBytes("lastframe.jpg", image.Data);
```

## Async surface

All blocking operations are exposed as `Task` / `Task<T>` (offloaded to
the `ThreadPool` so the calling context never blocks).  MJPEG frames
are delivered through the `MjpegFrame` event on the SDK's native worker
thread -- forward to your UI thread before touching UI state.

## Disposing

Every client is `IDisposable`.  Call `Dispose()` (or `using`) to free
the unmanaged handle promptly; a finalizer is provided as a safety net.

## Runtime identifiers

The package ships native binaries for:

- `linux-x64`
- `linux-arm`    (32-bit ARMv7, ITSCAM450)
- `linux-arm64`  (ITSCAM600)
- `win-x64`
- `win-x86`

Add new RIDs by extending the `<ItemGroup>` in `Itscam.Sdk.csproj` and
producing the matching build under `src/core/build/<rid>/`.
