# ITSCAM SDK -- Documentation Index

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

Chapter-style reference for the ITSCAM Client SDK. Start with
[Overview](overview.md) or jump straight to the topic you need.

## Foundations

- [Overview](overview.md) -- what the SDK is, supported platforms, and repository layout.
- [Getting started](getting-started.md) -- build the SDK, run examples, and link your app.
- [Error handling](error-handling.md) -- `Result<T>`, `Future<T>`, error codes, and logging.
- [HTTPS / TLS](https-tls.md) -- vendored mbedTLS, configuration, and failure modes.

## Camera Surfaces (C++)

- [`ItscamClient` (binary TCP)](api/binary-client.md)
- [`ItscamRestClient` (HTTP / JSON)](api/rest-client.md)
- [`ItscamCgiClient` (CGI / multipart)](api/cgi-client.md)

## Language Wrappers

- [C# / .NET](wrappers/csharp.md)
- [Python](wrappers/python.md)
- [Go](wrappers/go.md)

## History

- [Migration from CougarClient](migration-cougar.md)

For the project entry point with quick-link tables, see
[../README.md](../README.md).
