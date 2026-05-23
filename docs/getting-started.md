# Getting started

[Português (Brasil)](getting-started.md) | [English (US)](getting-started.en-US.md)

Caminho de cinco minutos: do checkout fresh até um example C++ rodando contra a câmera. O fluxo recomendado usa o **builder Docker** porque ele já inclui GCC, MinGW-w64, .NET 8 SDK, Go 1.25, Python 3, Wails e toda a toolchain de cross-compile.

## Pré-requisitos

- Docker Engine (caminho recomendado).
- Alternativamente, para builds nativos: compilador C++17 (GCC 7+, Clang 5+, MinGW-w64 para Windows) e GNU `make`.
- Opcional: `dotnet` (wrapper C#), `go` (wrapper Go), `python3` (wrapper Python) -- só precisam estar instalados no host se você **não** for usar o builder Docker.

Tudo o mais (cpp-httplib, nlohmann/json, mbedTLS) é vendored em [`src/core/3rdparty/`](../src/core/3rdparty), então não há packages de sistema obrigatórios.

## Build via Docker (recomendado)

```bash
git clone https://github.com/pumatronix/itscam-sdk.git
cd itscam-sdk

make docker-all        # tudo: Linux + Windows cross + wrappers
make docker-linux      # só libitscam_sdk.{so,a} para Linux
make docker-windows    # cross-compile itscam_sdk.dll com MinGW
make docker-shell      # shell interativo dentro do builder
make help              # lista todos os targets (docker-* e nativos)
```

Os artefatos caem em `src/core/build/<platform>/` e `src/examples/build/`, montados no host via volume.

## Build nativo (opcional)

Se você já tem GCC/Clang local e prefere não usar Docker:

```bash
make lib            # build libitscam_sdk.{so,a} para Linux
make windows        # cross-compile itscam_sdk.dll (MinGW)
make examples       # build dos quatro example binaries
make all            # tudo (Linux + Windows + wrappers)
```

## Rodar um example C++

```bash
# Binary client (rpath aponta para libitscam_sdk.so)
./src/examples/build/itscam_sdk_example 192.168.254.254

# REST client (login obrigatório)
./src/examples/build/itscam_rest_example 192.168.254.254 admin 1234

# CGI client (auth opcional; configCgi.blockAPI=false por default)
./src/examples/build/itscam_cgi_example 192.168.254.254
./src/examples/build/itscam_cgi_example 192.168.254.254 --https --insecure \
    --user admin --password 1234
```

## Linkar seu próprio app

### Contra a shared library (recomendado)

```bash
g++ -std=c++17 -Isrc/core/ -Isrc/core/3rdparty/ -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L./src/core/build/linux/ -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN:./src/core/build/linux' \
    -o your_app
```

### Binary self-contained estático

```bash
g++ -std=c++17 \
    -Isrc/core/ -Isrc/core/3rdparty/ \
    src/core/itscam_client.cpp \
    src/core/itscam_rest_client.cpp \
    src/core/itscam_cgi_client.cpp \
    src/core/impl/*.cpp \
    your_app.cpp \
    -o your_app -lpthread
```

(Para builds estáticos você também precisa compilar e linkar os sources do mbedTLS vendored. O `make lib` de top-level já cuida disso, então prefira o fluxo da shared library a menos que você tenha uma razão específica para evitar.)

## Dependências (vendored)

O repo é self-contained -- nada da tabela abaixo precisa estar instalado no host.

| Dependência     | Versão    | Local                                 |
| ---------------- | --------- | ------------------------------------- |
| cpp-httplib      | 0.31.0    | `src/core/3rdparty/httplib.h`         |
| nlohmann/json    | single h  | `src/core/3rdparty/nlohmann/json.hpp` |
| mbedTLS          | 3.6.2 LTS | `src/core/3rdparty/mbedtls/`          |

Veja [`docs/https-tls.md`](https-tls.md) para a configuration do mbedTLS e o procedimento de upgrade.
