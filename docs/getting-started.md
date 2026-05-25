# Getting started

[Português (Brasil)](getting-started.md) | [English (US)](getting-started.en-US.md)

Caminho de cinco minutos: do pacote SDK pré-compilado até um app rodando contra a câmera. Para quem precisa compilar o SDK do zero (contribuidores, customizações de plataforma), há uma [seção avançada](#build-do-sdk-a-partir-do-source) no final.

## Pré-requisitos

- Uma câmera ITSCAM (ITSCAM450 / ITSCAM600) alcançável na rede.
- O pacote de distribuição `itscam-sdk-<version>.tar.gz` (baixe na [página de releases](https://github.com/pumatronix/itscam-sdk/releases)).
- Para C++ / C: compilador C++17 (GCC 7+, Clang 5+) e `make`.
- Para C# / .NET: .NET SDK 6.0+.
- Para Python: Python 3.7+.
- Para Go: Go 1.21+ com cgo habilitado.

## Layout do pacote pré-compilado

Ao extrair o arquivo `itscam-sdk-<version>.tar.gz`, você encontra a seguinte estrutura:

```
itscam-sdk-<version>/
├── VERSION.json
├── README-sdk.md                    # layout do pacote + instalação por linguagem (PT-BR)
├── README-sdk.en-US.md              # same, in English (US)
├── README-repo.md                   # hub de navegação do repositório GitHub (PT-BR)
├── README.en-US.md                  # mesmo hub, em inglês
├── AGENTS.md                        # briefing para AI coding agents
├── docs/                            # guias chapter-style (getting started, API, wrappers, ...)
├── csharp/                          # NuGet multi-RID (linux-x64 + win-x64 + win-x86)
│   └── Pumatronix.Itscam.Sdk.<version>.nupkg
├── linux-x64/
│   ├── cpp/
│   │   ├── include/                 # Headers públicos (.h, .hpp)
│   │   └── lib/                     # libitscam_sdk.so + symlinks
│   ├── c/
│   │   ├── include/c_api/           # Headers da C API
│   │   └── lib/                     # mesmos .so
│   ├── python/
│   │   └── itscam-<version>.whl     # wheel com native lib embutida
│   └── go/
│       └── itscam-sdk-go/           # módulo Go com native lib em native/
├── win-x64/                         # mesma sub-estrutura (.dll + .a)
└── win-x86/                         # idem, 32-bit
```

## Extrair o SDK

```bash
tar xzf itscam-sdk-<version>.tar.gz
cd itscam-sdk-<version>
```

Nos exemplos abaixo, `$SDK` aponta para o diretório raiz extraído (ex: `/opt/itscam-sdk-1.0.0`).

## Integração por linguagem

### C++ / C

```bash
# Compile e linke contra a shared library pré-compilada:
g++ -std=c++17 \
    -I$SDK/linux-x64/cpp/include \
    -I$SDK/linux-x64/cpp/include/3rdparty \
    -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L$SDK/linux-x64/cpp/lib -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN' \
    -o your_app

# Copie a shared library para junto do binário (ou configure rpath / LD_LIBRARY_PATH):
cp $SDK/linux-x64/cpp/lib/libitscam_sdk.so* ./
```

Header umbrella: `#include "itscam_sdk.h"` traz toda a superfície pública.

Para a C API (FFI de outras linguagens): use os headers em `$SDK/linux-x64/c/include/c_api/`.

### C# / .NET

O pacote NuGet já inclui native binaries para todas as plataformas suportadas:

```bash
# Crie um projeto e adicione o NuGet a partir do diretório do SDK.
# Use nuget.config para combinar o feed local com nuget.org (deps transitivas):
dotnet new console -n MeuApp -o meu-app && cd meu-app
cat > nuget.config <<EOF
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="itscam-sdk" value="$SDK/csharp" />
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
  </packageSources>
</configuration>
EOF
ITSCAM_VERSION=$(sed -n 's/.*"nugetVersion"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$SDK/VERSION.json")
dotnet add package Pumatronix.Itscam.Sdk --version "$ITSCAM_VERSION"
```

O MSBuild target file do NuGet copia o native binary correto para o output de build automaticamente.

### Python

```bash
pip install $SDK/linux-x64/python/itscam-*.whl
python -c "import itscam; print(itscam.get_version())"
```

A wheel já embute `libitscam_sdk.so` -- nenhuma configuração de `LD_LIBRARY_PATH` é necessária.

### Go

O módulo Go dentro do SDK já inclui a native lib e os headers com cgo directives pré-configurados:

```bash
# No go.mod do seu projeto, aponte para o módulo local do SDK:
go mod edit -require=github.com/pumatronix/itscam-sdk-go@v0.0.0
go mod edit -replace=github.com/pumatronix/itscam-sdk-go=$SDK/linux-x64/go/itscam-sdk-go

# Dynamic linking (mais simples):
LD_LIBRARY_PATH=$SDK/linux-x64/go/itscam-sdk-go/native \
    go run main.go 192.168.254.254

# Static linking (binário sem dependência runtime):
CGO_CFLAGS="-I$SDK/linux-x64/go/itscam-sdk-go/include" \
    go build -tags static -o meu-app main.go
```

## Rodar um example rápido

Os tutoriais passo a passo criam um projeto do zero e salvam a primeira imagem em disco:

- [C++](tutorials/first-image-cpp.md)
- [C# / .NET](tutorials/first-image-csharp.md)
- [Python](tutorials/first-image-python.md)
- [Go](tutorials/first-image-go.md)

## Dependências de runtime

O SDK não tem dependências externas de runtime. O **mbedTLS 3.6 LTS** é statically linked nas bibliotecas nativas, então HTTPS funciona out of the box sem OpenSSL ou outra lib TLS de sistema.

| Dependência      | Versão    | Status                                  |
| ---------------- | --------- | --------------------------------------- |
| cpp-httplib      | 0.31.0    | Embutida no binário (build-time only)   |
| nlohmann/json    | single h  | Embutida no binário (build-time only)   |
| mbedTLS          | 3.6.2 LTS | Statically linked na `.so` / `.dll`     |

Veja [`docs/https-tls.md`](https-tls.md) para configuração do TLS em produção.

---

## Build do SDK a partir do source (avançado)

Se você precisa **compilar o SDK do zero** -- por exemplo, para contribuir com o projeto, cross-compilar para uma plataforma diferente ou depurar o core nativo -- siga as instruções abaixo.

### Pré-requisitos de build

- Docker Engine (caminho recomendado).
- Alternativamente, para builds nativos: compilador C++17 (GCC 7+, Clang 5+, MinGW-w64 para Windows) e GNU `make`.
- Opcional: `dotnet` (wrapper C#), `go` (wrapper Go), `python3` (wrapper Python) -- só precisam estar instalados no host se você **não** for usar o builder Docker.

Todas as dependências C++ (cpp-httplib, nlohmann/json, mbedTLS) são vendored em [`src/core/3rdparty/`](../src/core/3rdparty), então não há packages de sistema obrigatórios.

### Build via Docker (recomendado)

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

### Build nativo

Se você já tem GCC/Clang local e prefere não usar Docker:

```bash
make lib            # build libitscam_sdk.{so,a} para Linux
make windows        # cross-compile itscam_sdk.dll (MinGW)
make examples       # build dos quatro example binaries
make all            # tudo (Linux + Windows + wrappers)
```

### Gerar o pacote de distribuição

Após buildar tudo, empacote o SDK pré-compilado:

```bash
make sdk-dist           # gera dist/itscam-sdk-<version>.tar.gz
make docker-sdk-dist    # idem, dentro do Docker
```

### Rodar examples do source tree

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

### Linkar contra o source tree

Se você está desenvolvendo dentro do checkout do repositório, pode linkar diretamente contra os artefatos de build:

#### Contra a shared library

```bash
g++ -std=c++17 -Isrc/core/ -Isrc/core/3rdparty/ -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L./src/core/build/linux/ -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN:./src/core/build/linux' \
    -o your_app
```

#### Binary self-contained estático

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
