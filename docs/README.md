# ITSCAM SDK -- Índice da documentação

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

Referência organizada por chapters para o ITSCAM Client SDK. Comece pelo [Overview](overview.md) ou vá direto ao tema que você precisa.

## Fundamentos

- [Overview](overview.md) -- o que é o SDK, platforms suportadas e layout do repository.
- [Getting started](getting-started.md) -- baixe o SDK pré-compilado na [página de releases](https://github.com/pumatronix/itscam-sdk/releases), integre no seu app, rode examples e (opcional) build avançado a partir do source.
- [Error handling](error-handling.md) -- `Result<T>`, `Future<T>`, error codes e logging.
- [JPEG metadata (COM marker)](jpeg-metadata.md) -- extração de metadados de reconhecimento e classificação embutidos nas imagens JPEG.
- [HTTPS / TLS](https-tls.md) -- mbedTLS vendored, configuration e failure modes.

## Camera surfaces (C++)

- [`ItscamClient` (binary TCP)](api/binary-client.md)
- [`ItscamRestClient` (HTTP / JSON)](api/rest-client.md)
- [`ItscamCgiClient` (CGI / multipart)](api/cgi-client.md)

## Language wrappers

- [C++ (nativo)](wrappers/cpp.md)
- [C# / .NET](wrappers/csharp.md)
- [Python](wrappers/python.md)
- [Go](wrappers/go.md)
- [Java](wrappers/java.md)
- [Node.js](wrappers/nodejs.md)

## Tutoriais

Walkthroughs do zero -- criar um projeto, instalar a dependência do SDK e salvar a primeira imagem da câmera em disco:

- [Primeira imagem com C++](tutorials/first-image-cpp.md)
- [Primeira imagem com C# / .NET](tutorials/first-image-csharp.md)
- [Primeira imagem com Python](tutorials/first-image-python.md)
- [Primeira imagem com Go](tutorials/first-image-go.md)
- [Primeira imagem com Java](tutorials/first-image-java.md)
- [Primeira imagem com Node.js](tutorials/first-image-nodejs.md)

## Estendendo o SDK

- [Adicionar um novo wrapper](adding-a-new-wrapper.md) -- procedimento canônico para futuros bindings (Rust, Ruby, Swift, Kotlin, ...).

## Histórico

- [Migration from CougarClient](migration-cougar.md)

Cada chapter PT-BR tem uma versão em inglês com sufixo `*.en-US.md` (por exemplo, [`overview.en-US.md`](overview.en-US.md)). O switcher no topo de cada arquivo aponta para o par correspondente. O [índice em inglês](README.en-US.md) lista o mapping completo.

Para o ponto de entrada do projeto, com quick-link tables, veja [../README.md](../README.md).
