# AGENTS.md -- guidance for AI agents integrating the SDK

This file is the briefing AI coding agents should read **before** they write or modify code in this repository or in any application that embeds the ITSCAM SDK. It's intentionally short and scannable; the deep references live under [`docs/`](docs/).

> Human contributors: AGENTS.md is *adjacent to* the README, not a replacement for it. Start at [`README.md`](README.md).

---

## 1. What this SDK is

The **ITSCAM Client SDK** is a C++17 library that talks to Pumatronix ITSCAM cameras. A single C ABI in [`src/core/c_api/`](src/core/c_api/) is wrapped by:

- C++ (the native API)
- C# / .NET via P/Invoke ([`src/wrappers/csharp/`](src/wrappers/csharp/))
- Python via ctypes ([`src/wrappers/python/`](src/wrappers/python/))
- Go via cgo ([`src/wrappers/go/`](src/wrappers/go/))

All four bindings expose the **same three client classes** with identical surface area. When the user asks for a feature, implement it once in the C++ core, extend the C API, and propagate it to every wrapper that ships it.

## 2. The three client surfaces -- pick the right one

| Class             | Protocol                  | Use it for                                                                                                                | Don't use it for                       |
| ----------------- | ------------------------- | ------------------------------------------------------------------------------------------------------------------------- | -------------------------------------- |
| `ItscamClient`    | Binary TCP on **60000**   | Real-time triggers, GPIO/serial I/O, exposure-group accumulation, very low-latency capture.                               | Reading or writing daemon configuration.|
| `ItscamRestClient`| HTTP / HTTPS JSON         | Equipment configuration (profiles, OCR, classifier, lanes, ITSCAM PRO server hooks).  Anything `setX`/`getX` admin-level. | Continuous image streaming.            |
| `ItscamCgiClient` | HTTP / HTTPS multipart    | `snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`, `trigger.cgi?force=1`, `reboot.cgi`.                                   | Programmatic config -- use REST.        |

These clients are independent and can coexist in the same process.

References: [`docs/api/binary-client.md`](docs/api/binary-client.md), [`docs/api/rest-client.md`](docs/api/rest-client.md), [`docs/api/cgi-client.md`](docs/api/cgi-client.md).

## 3. Auth model -- get this right

- **REST always requires authentication.** Call `login(user, pass)` or `setAuthToken(jwt)` before any other REST method. A missing or expired token surfaces as `Error::NotAuthenticated` (HTTP 401).
- **CGI authentication is optional and normally disabled.** The camera's webapp gates CGI auth behind the `configCgi.blockAPI` flag, which defaults to `false`. **Do not** sprinkle `cgi.login(...)` calls unless the user explicitly opted in. Examples accept `--user` / `--password` flags only; without them they run anonymously and print a banner saying so.
- **Binary client** uses its own simple `authenticate(password)` challenge over the TCP channel.

If you find yourself adding `login()` calls to CGI code paths "just in case", stop and re-read this section.

## 4. Sync / async / streaming -- patterns

| Pattern               | API                                                                                          | Notes                                                          |
| --------------------- | -------------------------------------------------------------------------------------------- | -------------------------------------------------------------- |
| One-shot, blocking    | `Result<T> foo(...)`                                                                         | Default; pair with a timeout argument.                          |
| One-shot, async       | `Future<T> fooAsync(...)`                                                                    | `.get()`, `.get(ms)`, `.isReady()`, `waitAll()`.                |
| Continuous events     | `camera.onXxx(callback)`                                                                     | Runs on the SDK's worker thread.  **Do not block** in callbacks.|
| Continuous streams    | `startXxxStream(callback); stopXxxStream();`                                                 | Same threading rule.  Always pair start/stop.                   |
| Multi-exposure groups | `subscribeCaptures()` + `onTriggerExposureGroup` / `onSnapshotExposureGroup`                 | Per-RID accumulation; partial groups delivered after a timeout. |

In **C# / .NET** every blocking call has a `Task` / `Task<T>` counterpart; stream callbacks are exposed as `event EventHandler<...>`. In **Python** the same calls are synchronous Python methods and streaming uses Python callables; in **Go** they're plain functions returning `error`.

## 5. Errors -- one taxonomy, many wrappers

Every native method returns `Result<T>` carrying an `Error::Code` enum. Each wrapper maps that enum to its idiomatic error type ([details](docs/error-handling.md)):

| Native              | C#                              | Python                  | Go                          |
| ------------------- | ------------------------------- | ----------------------- | --------------------------- |
| `Timeout`           | `ItscamTimeoutException`        | `ItscamTimeoutError`    | `errors.Is(err, ErrTimeout)`|
| `NotAuthenticated`  | `ItscamAuthException`           | `ItscamAuthError`       | `errors.Is(err, ErrAuth)`   |
| `ConnectionFailed`  | `ItscamConnectionException`     | `ItscamConnectionError` | `errors.Is(err, ErrConn)`   |
| `InvalidParameter`  | `ItscamInvalidParameterException`| `ItscamError` subclass  | `errors.Is(err, ErrParam)`  |
| `ServerError`       | `ItscamServerException`         | `ItscamError` subclass  | `errors.Is(err, ErrServer)` |

When mapping new native error codes, **extend all four wrappers in the same change**.

## 6. Threading & lifetime

- Every client is move-only in C++ (PIMPL). In C# / Python / Go they are reference types; **always Dispose / `close` / `cancel` callbacks** before releasing the handle so worker threads stop cleanly.
- Callbacks fire on the SDK worker thread. Hand off heavy work (decoding, disk I/O, UI updates) via a queue / channel / dispatcher.
- The binary client has built-in **auto-reconnect**; subscribe to `onConnectionStateChanged` to see `Reconnecting` / `Reconnected` transitions and re-apply your own state on `Reconnected` if you keep per-session caches.

## 7. HTTPS

mbedTLS 3.6.2 LTS is vendored under [`src/core/3rdparty/mbedtls/`](src/core/3rdparty/mbedtls/) and statically linked. Switch to TLS by passing the `scheme` arg to `setBaseUrl`:

```cpp
client.setBaseUrl("camera.example.com", 443, "https");
client.setCaCertFile("/etc/itscam/ca.pem");   // or setCaCertData(pem)
// client.setVerifyServerCertificate(false);  // dev only
```

The same setters exist verbatim in every wrapper (`SetCaCertFile` / `set_ca_cert_file` / `SetCaCertFile`). Never disable verification in production code paths you ship to customers.

Reference: [`docs/https-tls.md`](docs/https-tls.md).

## 8. Build commands you'll actually run

```bash
make version                # regenerate version metadata (git tag + sha + date)
make lib                  # build libitscam_sdk.{so,a}
make examples             # build the four C++ examples
make csharp               # build Itscam.Sdk.dll
make csharp-pack          # produce a multi-RID NuGet
make go-cgi-example       # build the Go CGI example (needs `go` in PATH)
make docker-all           # everything inside the reproducible builder image
make sdk-dist             # consumer tar.gz (cpp/c/csharp/python/go)
make docker-sdk-dist      # same, inside Docker
```

After any C / C++ change run `make -C src/core linux` (fast incremental build) and `make examples` to confirm the linker is happy. After any wrapper change rebuild that wrapper's example.

The Python wrapper picks up the freshly built `.so` via `LD_LIBRARY_PATH`; the C# and Go wrappers via the relative paths inside their project / cgo directives.

## 9. Repository layout (compressed)

```
itscam-sdk/
  Makefile                   # top-level orchestrator
  README.md                  # navigation hub + quick-link matrix
  AGENTS.md                  # this file
  docs/                      # chapter-style documentation
  src/
    core/                    # C/C++ core library + c_api/ + 3rdparty/
    examples/                # 4 standalone C++ programs
    wrappers/
      python/                # ctypes
      go/                    # cgo + Wails GUI
      csharp/                # netstandard2.0 + NuGet packaging
```

Full breakdown in [`docs/overview.md`](docs/overview.md).

## 10. Anti-patterns -- don't do these

- **Don't add `cgi.login(...)` to library code or non-opt-in examples.** CGI auth is off by default; treating it as required breaks every default install.
- **Don't block inside callbacks.** Frame / event / connection-state callbacks all run on the SDK's worker thread. Push the payload to a queue and return immediately.
- **Don't `printf` from inside the SDK to communicate with users.** Use the per-client `setLogHandler(...)` hook so applications stay in control of stdout/stderr.
- **Don't introduce a new system dependency for HTTPS.** mbedTLS is vendored deliberately; the artefacts ship with zero TLS runtime deps. If you have a strong reason to switch backends, propose it before changing the build.
- **Don't hardcode paths under the old top-level `core/` / `examples/` / `wrappers/` directories.** Everything moved to [`src/`](src/) -- write `src/core/...`, `src/wrappers/...`, etc.
- **Don't add a wrapper feature without adding it to all wrappers** (or explicitly documenting why one is omitted). Parity is a contract.
- **Don't ignore `Result<T>` / exception returns.** Wrap with a check even in examples -- the SDK reports nuanced failure modes that are silently lost otherwise.
- **Don't hand-edit the generated REST type files.** Anything under `src/core/itscam_rest_types.hpp`, `src/wrappers/csharp/Itscam.Sdk/RestTypes/`, `src/wrappers/python/itscam/rest_types.py`, or `src/wrappers/go/itscam/rest_types.go` is rebuilt from `tools/codegen/spec/default.yaml` -- regenerate with `make codegen` instead. See [`docs/codegen.md`](docs/codegen.md).
- **Don't round-trip typed REST configs for partial updates.** The ITSCAM daemon merges PUT bodies; send only changed fields via `patchJson()` / `PatchJsonAsync` / `patch_json` / `PatchJSON`. Full-document PUT on `PUT /api/image/profiles/{id}` returns HTTP 500. Typed getters are fine for read-only inspection.

## 11. When the user asks for a new feature

1. Start in [`src/core/`](src/core/) -- add the typed method to the relevant C++ client (`ItscamClient` / `ItscamRestClient` / `ItscamCgiClient`) and its PIMPL implementation.
2. Extend the matching C API in [`src/core/c_api/`](src/core/c_api/) so wrappers can reach the feature.
3. Mirror the surface in each wrapper:
   - C#: [`src/wrappers/csharp/Itscam.Sdk/`](src/wrappers/csharp/Itscam.Sdk/) + `Native/NativeMethods.cs`.
   - Python: [`src/wrappers/python/itscam/`](src/wrappers/python/itscam/) + `bindings.py`.
   - Go: [`src/wrappers/go/itscam/`](src/wrappers/go/itscam/).
4. Add (or extend) an example under [`src/examples/`](src/examples/) and each wrapper's `examples/` folder.
5. Update the relevant chapter in [`docs/`](docs/) and add a row to the quick-link table in [`README.md`](README.md) if it's a new use-case.

**REST schema changes are special.** The REST configuration types (`OcrConfig`, `ProfileConfig`, ...) are **auto-generated** from the bundled OpenAPI snapshot at `tools/codegen/spec/default.yaml`. Do not hand-edit the generated files (`src/core/itscam_rest_types.hpp`, `src/wrappers/csharp/Itscam.Sdk/RestTypes/RestTypes.g.cs`, `src/wrappers/python/itscam/rest_types.py`, `src/wrappers/go/itscam/rest_types.go`). Workflow:

- If you touched anything under `tools/codegen/spec/`, run `make codegen` (or `make docker-codegen` on hosts without Node) and commit the regenerated files alongside the spec change.
- Always run `make codegen-check` before committing. CI runs `make docker-codegen-check` and will fail the build if the committed outputs drift from the snapshot.
- Downstream users regenerate against their own firmware with `make codegen SPEC=/path/to/their.yaml` -- the bundled snapshot is not touched. Keep the post-processor idempotent so this works.
- Use the generic `httpGet` / `httpPut` / `httpPost` / `httpDelete` escape hatch (and its Python / Go / C# equivalents) for one-off endpoints, partial updates, or fields outside the snapshot. Do **not** hand-roll typed structs to "extend" the generated ones.

See [`docs/codegen.md`](docs/codegen.md) for the end-to-end maintainer and downstream-user workflows.

That's the canonical flow -- do not invent a private path that short-circuits any of these steps without explicit operator approval.

---

If something here contradicts a more specific chapter in `docs/`, the chapter wins. When in doubt, follow the most recent commit on `main` and ask before changing public surface area.
