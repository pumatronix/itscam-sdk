# ITSCAM SDK -- Índice da documentação

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

Referência organizada por chapters para o ITSCAM Client SDK. Comece pelo
[Overview](overview.md) ou vá direto ao tema que você precisa.

## Fundamentos

- [Overview](overview.md) -- o que é o SDK, platforms suportadas e layout do repository.
- [Getting started](getting-started.md) -- build, execução de examples e link do seu app.
- [Error handling](error-handling.md) -- `Result<T>`, `Future<T>`, error codes e logging.
- [HTTPS / TLS](https-tls.md) -- mbedTLS vendored, configuration e failure modes.

## Camera surfaces (C++)

- [`ItscamClient` (binary TCP)](api/binary-client.md)
- [`ItscamRestClient` (HTTP / JSON)](api/rest-client.md)
- [`ItscamCgiClient` (CGI / multipart)](api/cgi-client.md)

## Language wrappers

- [C# / .NET](wrappers/csharp.md)
- [Python](wrappers/python.md)
- [Go](wrappers/go.md)

## Histórico

- [Migration from CougarClient](migration-cougar.md)

Para o ponto de entrada do projeto, com quick-link tables, veja
[../README.md](../README.md).
