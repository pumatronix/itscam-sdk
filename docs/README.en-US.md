# ITSCAM SDK -- Documentation Index

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

Chapter-style reference for the ITSCAM Client SDK. Start with [Overview](overview.en-US.md) or jump straight to the topic you need.

## Foundations

- [Overview](overview.en-US.md) -- what the SDK is, supported platforms, and repository layout.
- [Getting started](getting-started.en-US.md) -- download the pre-compiled SDK from the [releases page](https://github.com/pumatronix/itscam-sdk/releases), integrate into your app, run examples, and (optionally) advanced build from source.
- [Error handling](error-handling.en-US.md) -- `Result<T>`, `Future<T>`, error codes, and logging.
- [JPEG metadata (COM marker)](jpeg-metadata.en-US.md) -- extracting plate-recognition and classification metadata embedded in JPEG images.
- [JPEG Metadata Visualizer](metadata-visualizer.en-US.md) -- local Pumatronix JPEG/multipart upload, exposure carousel, searchable/sortable tag table, and bounding boxes.
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
- [Java](wrappers/java.en-US.md)
- [Node.js](wrappers/nodejs.en-US.md)

## Tutorials

Step-by-step walkthroughs -- create a project, install the SDK dependency and save the first camera image to disk:

- [First image with C++](tutorials/first-image-cpp.md)
- [First image with C# / .NET](tutorials/first-image-csharp.md)
- [First image with Python](tutorials/first-image-python.md)
- [First image with Go](tutorials/first-image-go.md)
- [First image with Java](tutorials/first-image-java.md)
- [First image with Node.js](tutorials/first-image-nodejs.md)

## Extending the SDK

- [Adding a new wrapper](adding-a-new-wrapper.en-US.md) -- canonical procedure for future language bindings (Rust, Ruby, Swift, Kotlin, ...).

## History

- [Migration from CougarClient](migration-cougar.en-US.md)

PT-BR is the primary language for every page; the `*.en-US.md` companions track the current translation pass. Tutorials are PT-BR-only for now.

For the project entry point with quick-link tables, see [../README.en-US.md](../README.en-US.md).
