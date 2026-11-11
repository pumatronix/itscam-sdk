# Usando o SDK pelo CMake

O repositório é um projeto CMake: uma aplicação compila o SDK a partir do código-fonte com o próprio toolchain, como subprojeto ou a partir de um pacote instalado, e linka um único target, `itscam::sdk`. English: [`cmake.en-US.md`](cmake.en-US.md).

Os artefatos do release continuam sendo gerados pelo `make` ([`compatibility.md`](compatibility.md)). Os dois sistemas de build compilam as mesmas fontes com as mesmas flags, e o `make verify` compila os dois.

## Como subprojeto

Com o SDK num subdiretório (um submódulo git, uma cópia):

```cmake
cmake_minimum_required(VERSION 3.10)
project(app CXX)

set(ITSCAM_SDK_WITH_CGI OFF)        # opções antes do add_subdirectory()
add_subdirectory(third_party/itscam-sdk)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE itscam::sdk)
```

Com FetchContent (CMake 3.14+):

```cmake
include(FetchContent)
FetchContent_Declare(itscam_sdk
    GIT_REPOSITORY https://github.com/pumatronix/itscam-sdk.git
    GIT_TAG v0.9.0)
FetchContent_MakeAvailable(itscam_sdk)
target_link_libraries(app PRIVATE itscam::sdk)
```

Como subprojeto, o SDK gera uma biblioteca estática por padrão, não instala nada e não escreve nada no próprio código-fonte. O header de versão é gerado na árvore de build, a partir do `git describe`.

## Como pacote instalado

```bash
mkdir -p build/cmake && cd build/cmake
cmake ../.. -DCMAKE_INSTALL_PREFIX=/opt/itscam-sdk
cmake --build . --target install
```

```cmake
find_package(itscam_sdk 0.9 CONFIG REQUIRED)   # CMAKE_PREFIX_PATH=/opt/itscam-sdk
target_link_libraries(app PRIVATE itscam::sdk)
```

Os headers vão para `include/itscam` (`#include "itscam_sdk.h"`, como no tarball do release), a biblioteca para `lib`, e os arquivos do pacote para `lib/cmake/itscam_sdk`. Enquanto a versão major for 0, `find_package(itscam_sdk 0.9)` aceita só 0.9.x: um release minor pode quebrar quem chama. O pacote define `itscam_sdk_SHARED`, `itscam_sdk_WITH_REST`, `itscam_sdk_WITH_CGI`, `itscam_sdk_WITH_TLS` e `itscam_sdk_WITH_C_API` com as opções com que foi compilado.

## Opções

| Opção | Padrão | Efeito |
| ----- | ------ | ------ |
| `ITSCAM_SDK_SHARED` | `BUILD_SHARED_LIBS` se definido; senão ON standalone e OFF como subprojeto | `libitscam_sdk` compartilhada ou estática. |
| `ITSCAM_SDK_WITH_REST` | ON | O cliente REST (`ItscamRestClient`). |
| `ITSCAM_SDK_WITH_CGI` | ON | O cliente CGI (`ItscamCgiClient`). |
| `ITSCAM_SDK_WITH_TLS` | ON | HTTPS para REST e CGI, com o mbedTLS embarcado linkado dentro. |
| `ITSCAM_SDK_WITH_C_API` | ON | A API C, de que os wrappers de linguagem precisam. |
| `ITSCAM_SDK_CXX_STANDARD` | 17 | Padrão com que as fontes do SDK compilam: 14 ou 17. Os headers exigem C++14. |
| `ITSCAM_SDK_STATIC_RUNTIME` | ON | Biblioteca compartilhada: linka libstdc++ e libgcc dentro, então ela precisa só da glibc. |
| `ITSCAM_SDK_INSTALL` | ON standalone, OFF como subprojeto | Regras de instalação e o pacote `itscam_sdk`. |
| `ITSCAM_SDK_BUILD_EXAMPLES` | OFF | Os exemplos C++ de `src/examples`. |
| `ITSCAM_SDK_BUILD_TESTS` | OFF | Os smoke tests C e C++, executados pelo `ctest` contra [`tests/standin`](../tests/standin). |

O cliente binário (`ItscamClient`) sempre é compilado. O build mínimo, só o cliente binário em C++14, é o que um daemon embarcado precisa:

```cmake
set(ITSCAM_SDK_CXX_STANDARD 14)
set(ITSCAM_SDK_WITH_REST OFF)
set(ITSCAM_SDK_WITH_CGI OFF)
set(ITSCAM_SDK_WITH_TLS OFF)
set(ITSCAM_SDK_WITH_C_API OFF)
add_subdirectory(itscam-sdk)
```

## Compilação cruzada

Passe o seu toolchain como de costume. [`cmake/toolchains`](../cmake/toolchains) tem arquivos para os compiladores da imagem builder, que também mostram o formato que um arquivo de toolchain precisa ter:

| Arquivo | Alvo |
| ------- | ---- |
| `arm-linux-gnueabihf.cmake` | Linux ARMv7 hard-float (ITSCAM450) |
| `aarch64-linux-gnu.cmake` | Linux ARM64 (ITSCAM600, ITSCAM450b) |
| `x86_64-w64-mingw32.cmake`, `i686-w64-mingw32.cmake` | Windows x64, x86 |

```bash
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/aarch64-linux-gnu.cmake
```

Qualquer toolchain Linux com glibc 2.23 ou mais nova funciona. Um toolchain sem `libstdc++.a` compila a biblioteca compartilhada com `-DITSCAM_SDK_STATIC_RUNTIME=OFF`, e aí precisa da libstdc++ no alvo.

No Windows, a DLL exporta só a API C, como a DLL do release. Aplicações C++ linkam a biblioteca estática (`ITSCAM_SDK_SHARED=OFF`).

## O que o `make verify` verifica

[`tools/verify/cmake-matrix.sh`](../tools/verify/cmake-matrix.sh) compila uma aplicação ([`tests/cmake/consumer`](../tests/cmake/consumer)) de cada um destes jeitos, com CMake 3.10:

- build standalone compartilhado com exemplos e smoke tests, `ctest`, instalação;
- subprojeto, estático, C++17;
- subprojeto, só o cliente binário, fontes e aplicação em C++14;
- subprojeto, compartilhado;
- `find_package` a partir da instalação;
- ARMv7 e ARM64 com os toolchains Arm GNU-A;
- Windows x64 (DLL, aplicação C) e x86 (estático, aplicação C++).
