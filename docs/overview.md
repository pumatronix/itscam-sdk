# Overview

The **ITSCAM Client SDK** is a cross-platform C++14 library that talks to
[Pumatronix](https://www.pumatronix.com) ITSCAM cameras over three
complementary transports.  Native bindings for Python, Go and C# / .NET
sit on top of a single C ABI, so a single build of the core library
serves every language.

## Platform support

| Platform | Architectures              | Toolchain         |
| -------- | -------------------------- | ----------------- |
| Linux    | x86_64, ARMv7, ARM64       | GCC / Clang       |
| Windows  | x86_64                     | MinGW-w64         |

Built artefacts:

| File                            | Purpose          |
| ------------------------------- | ---------------- |
| `libitscam_sdk.so.1.0.0`        | Linux shared lib |
| `libitscam_sdk.so.1`            | Linux soname     |
| `libitscam_sdk.so`              | Linker symlink   |
| `libitscam_sdk.a`               | Linux static lib |
| `itscam_sdk.dll`                | Windows DLL      |
| `libitscam_sdk_static.a`        | Windows static   |

mbedTLS is statically linked, so the resulting libraries have no
runtime dependency on OpenSSL or system mbedTLS.

## Camera surfaces

The SDK exposes three independent client classes; pick the one that
matches the protocol your application needs.

| Class             | Protocol                      | Use it for                                                                                         |
| ----------------- | ----------------------------- | -------------------------------------------------------------------------------------------------- |
| `ItscamClient`    | Binary TCP (Cougar) on 60000  | Real-time triggers, live previews, multi-exposure capture, GPIO/serial I/O, image-config tuning.   |
| `ItscamRestClient`| HTTP / HTTPS JSON on 80 / 443 | Equipment configuration, profiles, OCR, classifier, lanes, ITSCAM PRO server hooks (admin scope). |
| `ItscamCgiClient` | HTTP / HTTPS on 80 / 443      | `snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`, `trigger.cgi`, `reboot.cgi`.                    |

They share a common error / logging surface and can be used in the same
process side by side.

## Repository layout

```
itscam-sdk/
├── Makefile                # top-level build orchestrator
├── Dockerfile              # reproducible build environment
├── README.md               # navigation hub + quick-link matrix
├── AGENTS.md               # guidance for AI agents
├── docs/                   # chapter-style documentation (this folder)
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
    ├── examples/                        # C++ example applications
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

- [Getting started](getting-started.md) -- build the library and run an example.
- [API: binary client](api/binary-client.md) -- low-latency TCP control.
- [API: REST client](api/rest-client.md) -- equipment configuration.
- [API: CGI client](api/cgi-client.md) -- HTTP image endpoints.
- [HTTPS / TLS](https-tls.md) -- mbedTLS configuration.
- [Error handling](error-handling.md) -- `Result<T>`, `Future<T>`, error codes.
- Wrappers: [Python](wrappers/python.md) -- [Go](wrappers/go.md) -- [C# / .NET](wrappers/csharp.md).
- [Migration from CougarClient](migration-cougar.md).
