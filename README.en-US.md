![ITSCAM Client SDK](./docs/images/itscam-sdk-repo-logo.jpg)

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

**Documentation:** [pumatronix.github.io/itscam-sdk](https://pumatronix.github.io/itscam-sdk/)

A cross-platform C++17 library for integrating [Pumatronix](https://www.pumatronix.com) ITSCAM cameras (ITSCAM450 / ITSCAM600), with idiomatic bindings for **C# / .NET**, **Python**, and **Go**. HTTPS support and the mbedTLS backend are bundled with the SDK, so no system TLS dependency is required beyond a C++ compiler.

The SDK exposes three independent client classes across C++, C#, Python, and Go. Choose the client by protocol and task. They can run side by side in the same process. The binary client uses the **Cougar** protocol on TCP port **60000**. REST and CGI use HTTP/HTTPS on ports **80/443**.

| Client | Transport | Use it for | Do not use it for |
| ------ | --------- | ---------- | ----------------- |
| **`ItscamClient`** | **Cougar** binary TCP **:60000** | Real-time triggers and snapshots, GPIO/serial, multi-exposure groups, **image-pipeline** control (profiles, trigger/exposure, JPEG, overlays/crops), low latency, and auto-reconnect. | **Equipment** administration, including networking, timezone, FTP, licenses, OCR/lanes/analytics, and other settings owned by the webapp/daemon. Use REST. |
| **`ItscamRestClient`** | HTTP/HTTPS JSON | **Equipment** and daemon administration through the webapp backend: networking, timezone, OCR, classifier, lanes, analytics, ITSCAM PRO, typed config helpers, and partial JSON updates. | Real-time Cougar event streams, low-latency pipeline tuning, or continuous image streaming. Use the binary client for pipeline work, and log in before any REST call. |
| **`ItscamCgiClient`** | HTTP/HTTPS multipart | `snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`, forced triggers, and simple image endpoints. Credentials are optional. Anonymous access is the default. | Configuration. Use Cougar for pipeline settings or REST for equipment settings. |

## Quick Links

Choose a row by **what you want to do** and a column by **the language you ship in**. Every cell links to a runnable example.

| Use case | C++ | C# / .NET | Python | Go |
| -------- | --- | --------- | ------ | -- |
| Binary capture / triggers (real time, TCP 60000) | [`itscam_sdk_example.cpp`](src/examples/itscam_sdk_example.cpp) | [`BinaryCaptureExample/Program.cs`](src/wrappers/csharp/examples/BinaryCaptureExample/Program.cs) | [`capture_example.py`](src/wrappers/python/examples/capture_example.py) | [`capture_example.go`](src/wrappers/go/examples/capture_example.go) |
| Trigger-burst recorder to disk | [`itscam_trigger_recorder.cpp`](src/examples/itscam_trigger_recorder.cpp) | -- | -- | -- |
| Equipment / REST configuration (login required) | [`itscam_rest_example.cpp`](src/examples/itscam_rest_example.cpp) | [`CaptureExample/Program.cs`](src/wrappers/csharp/examples/CaptureExample/Program.cs) | [`rest_example.py`](src/wrappers/python/examples/rest_example.py) | [`rest_example.go`](src/wrappers/go/examples/rest_example.go) |
| CGI snapshot / lastframe / MJPEG (auth optional) | [`itscam_cgi_example.cpp`](src/examples/itscam_cgi_example.cpp) | [`CaptureExample/Program.cs`](src/wrappers/csharp/examples/CaptureExample/Program.cs) | [`cgi_snapshot_example.py`](src/wrappers/python/examples/cgi_snapshot_example.py) | [`cgi_snapshot_example.go`](src/wrappers/go/examples/cgi_snapshot_example.go) |
| HTTPS / TLS (REST + CGI, mbedTLS) | [`itscam_rest_example.cpp`](src/examples/itscam_rest_example.cpp) (`--https`) | [`CaptureExample/Program.cs`](src/wrappers/csharp/examples/CaptureExample/Program.cs) (`--https`) | [`cgi_snapshot_example.py`](src/wrappers/python/examples/cgi_snapshot_example.py) (`--https`) | [`cgi_snapshot_example.go`](src/wrappers/go/examples/cgi_snapshot_example.go) (`--https`) |
| Desktop GUI viewer (Wails) | -- | -- | -- | [`gui/`](src/wrappers/go/examples/gui/) |

Reference docs by surface:

| Surface | API guide | Wrapper guide |
| ------- | --------- | ------------- |
| Binary client (Cougar TCP 60000) | [docs/api/binary-client.md](docs/api/binary-client.md) | [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md) -- [C#](docs/wrappers/csharp.md) |
| REST client (HTTP/HTTPS JSON) | [docs/api/rest-client.md](docs/api/rest-client.md) | [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md) -- [C#](docs/wrappers/csharp.md) |
| CGI client (HTTP/HTTPS multipart) | [docs/api/cgi-client.md](docs/api/cgi-client.md) | [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md) -- [C#](docs/wrappers/csharp.md) |

## Getting Started

The SDK is distributed as a pre-compiled package (`itscam-sdk-<version>.tar.gz`) containing headers, shared libraries, a NuGet, Python wheel, and Go module for linux-x64, win-x64, and win-x86. Extract the package and integrate directly into your project -- no compilation needed:

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>
```

| Language | Quick integration |
| -------- | ----------------- |
| **C++ / C** | `g++ -I$SDK/linux-x64/cpp/include ... -L$SDK/linux-x64/cpp/lib -litscam_sdk` |
| **C# / .NET** | `dotnet add package Pumatronix.Itscam.Sdk --source $SDK/csharp` |
| **Python** | `pip install $SDK/linux-x64/python/itscam-*.whl` |
| **Go** | `go mod edit -replace=...=$SDK/linux-x64/go/itscam-sdk-go` |

Full integration guide by language in [`docs/getting-started.md`](docs/getting-started.md).

## Building from Source (advanced)

If you need to build the SDK from scratch (contributors, cross-compile, debug), use the Docker builder:

```bash
git clone https://github.com/pumatronix/itscam-sdk.git && cd itscam-sdk
make docker-all     # everything: Linux + Windows cross + wrappers (recommended)
make docker-linux   # libitscam_sdk.{so,a} for Linux only
make docker-shell   # interactive shell with the full toolchain
make sdk-dist       # produce the itscam-sdk-<version>.tar.gz package
make help           # list every target (docker-* and native)
```

Optional native build on the host (when GCC/Clang is already installed):

```bash
make lib            # build libitscam_sdk.{so,a} for Linux
make examples       # build the four C++ example binaries
make all            # build everything: Linux + Windows cross + wrappers
```

All native code lives under [`src/`](src/). See [`docs/overview.md`](docs/overview.md) for the full repository layout and [`docs/getting-started.md`](docs/getting-started.md) for build and integration details.

## Using AI Agents

This repository ships [`AGENTS.md`](AGENTS.md), a short, scannable briefing for coding agents (Cursor, Copilot, Claude Code, and similar tools). Read it **before** asking for SDK changes or integration work in apps that embed the library.

**In this repository**

- Tools that support [AGENTS.md](https://agents.md/) load the file automatically from the workspace root.
- In chat, reference `@AGENTS.md` or point the agent at the [Quick Links](#quick-links) table and the example for your language.
- Ask for reproducible builds with `make docker-all` or `make docker-linux` instead of installing toolchains by hand.

**In your app that consumes the SDK**

- Add [`AGENTS.md`](AGENTS.md) to the agent context (reference, `@`-mention, or project rule) to avoid picking the wrong client or auth model.
- State explicitly: language (C++ / C# / Python / Go), client (`ItscamClient`, `ItscamRestClient`, or `ItscamCgiClient`), and whether the task is real-time capture, REST configuration, or a CGI snapshot.

**Rules agents should follow** (full detail in [`AGENTS.md`](AGENTS.md)):

| Topic | Quick rule |
| ----- | ---------- |
| Three clients | Cougar **:60000** for pipeline/capture; REST for equipment; CGI for HTTP image endpoints. |
| Auth | REST always requires `login`; CGI is anonymous by default — do not add `cgi.login()` without opt-in. |
| New features | C++ core → C API → every wrapper; keep C# / Python / Go parity. |
| REST types | Generated from `tools/codegen/spec/default.yaml` — run `make codegen`, do not hand-edit. |
| Partial updates | Use `patchJson()` / `PatchJSON`; full profile PUT returns HTTP 500. |

**Documentation assistant**

The [docs site](docs-site/) (GitHub Pages) can embed an assistant powered by [Cloudflare AI Search](https://developers.cloudflare.com/ai-search/), with a corpus synced from `docs/`, examples, and `AGENTS.md`. See [`docs-site/README.md`](docs-site/README.md) for local setup and deployment.

## Documentation Map

This README is intentionally concise. Chapter-style documentation lives under [`docs/`](docs/):

- [Documentation index](docs/README.md) -- the main entry point for the docs.
- [Overview](docs/overview.md) -- what the SDK includes and where to find it.
- [Getting started](docs/getting-started.md) -- build the SDK, run examples, and link your app.
- [HTTPS / TLS](docs/https-tls.md) -- vendored mbedTLS, configuration, and troubleshooting.
- [Error handling](docs/error-handling.md) -- `Result<T>`, `Future<T>`, error codes, and logging.
- [JPEG metadata (COM marker)](docs/jpeg-metadata.en-US.md) -- extracting plate-recognition and classification metadata embedded in JPEG images.
- [Typed REST helpers & codegen](docs/codegen.md) -- bundled OpenAPI snapshot and regeneration workflows.
- API reference: [Binary client](docs/api/binary-client.md) -- [REST client](docs/api/rest-client.md) -- [CGI client](docs/api/cgi-client.md).
- Wrappers: [C# / .NET](docs/wrappers/csharp.md) -- [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md).
- [Migration from CougarClient](docs/migration-cougar.md).
- [`AGENTS.md`](AGENTS.md) — briefing for coding agents; see [Using AI Agents](#using-ai-agents).
- **[Documentation website](docs-site/)** -- VitePress site for GitHub Pages with an optional [Cloudflare AI Search](https://developers.cloudflare.com/ai-search/) assistant. See [`docs-site/README.md`](docs-site/README.md) for setup.

## Highlights

- **Three client surfaces, one library.** Cougar binary for real-time capture and pipeline control. REST for equipment administration. CGI for HTTP image endpoints.
- **HTTPS out of the box.** mbedTLS 3.6 LTS is vendored under [`src/core/3rdparty/mbedtls/`](src/core/3rdparty/mbedtls/) and statically linked into `libitscam_sdk`.
- **Auth-aware examples.** REST always requires login. CGI is anonymous by default (`configCgi.blockAPI = false`), and credentials are opt-in (`--user / --password`) in every language wrapper.
- **Feature parity** across the C#, Python, and Go wrappers through the C API in [`src/core/c_api/`](src/core/c_api/).

## Platform Support

| Platform | Architectures | Toolchain |
| -------- | ------------- | --------- |
| Linux | x86_64, ARMv7, ARM64 | GCC / Clang |
| Windows | x86_64 | MinGW-w64 |

## License and Contact

Copyright (c) 2026 Pumatronix Equipamentos Eletrônicos. Proprietary software. Contact Pumatronix for licensing terms.
