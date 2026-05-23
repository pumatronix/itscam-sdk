# ITSCAM SDK -- Documentation Index

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

Chapter-style reference for the ITSCAM Client SDK. Start with [Overview](overview.en-US.md) or jump straight to the topic you need.

## Foundations

- [Overview](overview.en-US.md) -- what the SDK is, supported platforms, and repository layout.
- [Getting started](getting-started.en-US.md) -- build the SDK, run examples, and link your app.
- [Error handling](error-handling.en-US.md) -- `Result<T>`, `Future<T>`, error codes, and logging.
- [HTTPS / TLS](https-tls.en-US.md) -- vendored mbedTLS, configuration, and failure modes.
- [Typed REST helpers and codegen](codegen.en-US.md) -- bundled OpenAPI snapshot and regeneration workflows.

## Camera Surfaces (C++)

- [`ItscamClient` (binary TCP)](api/binary-client.en-US.md)
- [`ItscamRestClient` (HTTP / JSON)](api/rest-client.en-US.md)
- [`ItscamCgiClient` (CGI / multipart)](api/cgi-client.en-US.md)

## Language Wrappers

- [Native C++](wrappers/cpp.en-US.md)
- [C# / .NET](wrappers/csharp.en-US.md)
- [Python](wrappers/python.en-US.md)
- [Go](wrappers/go.en-US.md)

## Tutorials

Step-by-step walkthroughs -- create a project, install the SDK dependency and save the first camera image to disk:

- [First image with C++](tutorials/first-image-cpp.md)
- [First image with C# / .NET](tutorials/first-image-csharp.md)
- [First image with Python](tutorials/first-image-python.md)
- [First image with Go](tutorials/first-image-go.md)

## History

- [Migration from CougarClient](migration-cougar.en-US.md)

PT-BR is the primary language for every page; the `*.en-US.md` companions track the current translation pass. Tutorials are PT-BR-only for now.

For the project entry point with quick-link tables, see [../README.en-US.md](../README.en-US.md).
