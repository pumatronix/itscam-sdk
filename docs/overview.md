# Overview

[Português (Brasil)](overview.md) | [English (US)](overview.en-US.md)

O **ITSCAM Client SDK** é uma library C++17 cross-platform que conversa com câmeras [Pumatronix](https://www.pumatronix.com) ITSCAM por três transportes complementares (binary TCP Cougar, REST e CGI). Bindings nativos para Python, Go e C# / .NET ficam em cima de uma única C ABI, então um único build do core serve todas as linguagens.

> A descrição rápida das três classes de client e as tabelas de platforms suportadas vivem no [`README.md`](https://github.com/pumatronix/itscam-sdk/blob/main/README.md) (também é a home page do [docs site](https://pumatronix.github.io/itscam-sdk/)). Esta página foca no layout do repository e em onde ir a partir daqui.

## Pacote pré-compilado

O SDK é distribuído como `itscam-sdk-<version>.tar.gz`, contendo binários pré-compilados para linux-x64, win-x64 e win-x86, mais NuGet (.NET), Python wheel e módulo Go. Esse é o caminho recomendado para integração -- veja [Getting started](getting-started.md).

## Build artefacts (a partir do source)

Ao compilar o SDK do zero (`make lib` / `make docker-all`), os artefatos ficam em `src/core/build/<platform>/`:

| Arquivo                         | Plataforma       |
| ------------------------------- | ---------------- |
| `libitscam_sdk.so.1.0.0`        | Linux shared lib |
| `libitscam_sdk.so.1`            | Linux soname     |
| `libitscam_sdk.so`              | Linker symlink   |
| `libitscam_sdk.a`               | Linux static lib |
| `itscam_sdk.dll`                | Windows DLL      |
| `libitscam_sdk_static.a`        | Windows static   |

mbedTLS é statically linked, então as bibliotecas resultantes não têm dependência de runtime de OpenSSL ou mbedTLS de sistema.

## Repository layout

```
itscam-sdk/
├── Makefile                # build orchestrator de top-level
├── Dockerfile              # ambiente de build reproduzível
├── README.md               # navigation hub + quick-link matrix
├── AGENTS.md               # briefing para AI agents
├── docs/                   # documentação chapter-style (esta pasta)
├── docs-site/              # site VitePress publicado no GitHub Pages
└── src/                    # TODO o código-fonte vive aqui
    ├── core/               # core C/C++ library
    │   ├── itscam_client.{h,cpp}        # binary TCP client
    │   ├── itscam_rest_client.{h,cpp}   # JSON REST client
    │   ├── itscam_cgi_client.{h,cpp}    # CGI client
    │   ├── itscam_sdk_utils.h
    │   ├── impl/                        # platform + transport HTTP shared
    │   ├── c_api/                       # C API para wrappers FFI
    │   └── 3rdparty/
    │       ├── httplib.h
    │       ├── nlohmann/json.hpp
    │       └── mbedtls/                 # mbedTLS 3.6 LTS vendored
    ├── examples/                        # programas C++ standalone
    │   ├── itscam_sdk_example.cpp
    │   ├── itscam_rest_example.cpp
    │   ├── itscam_cgi_example.cpp
    │   └── itscam_trigger_recorder.cpp
    └── wrappers/
        ├── python/                      # binding ctypes
        ├── go/                          # binding cgo (+ exemplo Wails GUI)
        └── csharp/                      # netstandard2.0 + packaging NuGet
```

## Onde ir a partir daqui

- [Getting started](getting-started.md) -- buildar a library e rodar um example.
- [API: binary client](api/binary-client.md) -- controle TCP de baixa latência.
- [API: REST client](api/rest-client.md) -- equipment configuration.
- [API: CGI client](api/cgi-client.md) -- HTTP image endpoints.
- [HTTPS / TLS](https-tls.md) -- configuration do mbedTLS.
- [Error handling](error-handling.md) -- `Result<T>`, `Future<T>`, error codes.
- Wrappers: [Python](wrappers/python.md) -- [Go](wrappers/go.md) -- [C# / .NET](wrappers/csharp.md).
- [Migration from CougarClient](migration-cougar.md).
