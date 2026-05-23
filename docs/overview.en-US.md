# Overview

[Português (Brasil)](overview.md) | [English (US)](overview.en-US.md)

The **ITSCAM Client SDK** is a cross-platform C++17 library that talks to [Pumatronix](https://www.pumatronix.com) ITSCAM cameras over three complementary transports (binary TCP Cougar, REST, and CGI). Native bindings for Python, Go and C# / .NET sit on top of a single C ABI, so a single build of the core library serves every language.

> The short description of the three client classes and the supported-platform tables live in the [`README.en-US.md`](https://github.com/pumatronix/itscam-sdk/blob/main/README.en-US.md) (also the home of the [docs site](https://pumatronix.github.io/itscam-sdk/README.en-US)). This page focuses on the repository layout and where to go next.

## Build artefacts

| File                            | Platform         |
| ------------------------------- | ---------------- |
| `libitscam_sdk.so.1.0.0`        | Linux shared lib |
| `libitscam_sdk.so.1`            | Linux soname     |
| `libitscam_sdk.so`              | Linker symlink   |
| `libitscam_sdk.a`               | Linux static lib |
| `itscam_sdk.dll`                | Windows DLL      |
| `libitscam_sdk_static.a`        | Windows static   |

mbedTLS is statically linked, so the resulting libraries have no runtime dependency on OpenSSL or system mbedTLS.

## Repository layout

```
itscam-sdk/
├── Makefile                # top-level build orchestrator
├── Dockerfile              # reproducible build environment
├── README.md               # navigation hub + quick-link matrix
├── AGENTS.md               # guidance for AI agents
├── docs/                   # chapter-style documentation (this folder)
├── docs-site/              # VitePress site published on GitHub Pages
└── src/                    # ALL source code lives here
    ├── core/               # C/C++ core library
    │   ├── itscam_client.{h,cpp}        # binary TCP client
    │   ├── itscam_rest_client.{h,cpp}   # JSON REST client
    │   ├── itscam_cgi_client.{h,cpp}    # CGI client
    │   ├── itscam_sdk_utils.h
    │   ├── impl/                        # platform + shared HTTP transport
    │   ├── c_api/                       # C API for FFI wrappers
    │   └── 3rdparty/
    │       ├── httplib.h
    │       ├── nlohmann/json.hpp
    │       └── mbedtls/                 # vendored mbedTLS 3.6 LTS
    ├── examples/                        # standalone C++ programs
    │   ├── itscam_sdk_example.cpp
    │   ├── itscam_rest_example.cpp
    │   ├── itscam_cgi_example.cpp
    │   └── itscam_trigger_recorder.cpp
    └── wrappers/
        ├── python/                      # ctypes binding
        ├── go/                          # cgo binding (+ Wails GUI example)
        └── csharp/                      # netstandard2.0 + NuGet packaging
```

## Where to go next

- [Getting started](getting-started.en-US.md) -- build the library and run an example.
- [API: binary client](api/binary-client.md) -- low-latency TCP control.
- [API: REST client](api/rest-client.md) -- equipment configuration.
- [API: CGI client](api/cgi-client.md) -- HTTP image endpoints.
- [HTTPS / TLS](https-tls.md) -- mbedTLS configuration.
- [Error handling](error-handling.md) -- `Result<T>`, `Future<T>`, error codes.
- Wrappers: [Python](wrappers/python.en-US.md) -- [Go](wrappers/go.en-US.md) -- [C# / .NET](wrappers/csharp.en-US.md).
- [Migration from CougarClient](migration-cougar.md).
