# API nativa C++

[Português (Brasil)](cpp.md) | [English (US)](cpp.en-US.md)

A API nativa fica em [`src/core/`](../../src/core/) e é o ponto de partida para os outros wrappers (C# / Python / Go) -- todos eles falam com o mesmo binário através da C ABI em [`src/core/c_api/`](../../src/core/c_api/). Se você está consumindo o SDK direto em um app C++17, este é o guia certo.

> **Referência completa de classes, structs e funções**: [referência Doxygen](/api-ref/cpp/index.html). Esta página cobre integração, build, link e padrões idiomáticos.

> Esta página cobre **integração e padrões idiomáticos** do código C++. A referência detalhada de cada client surface (com tabelas de endpoints, tipos de request/response, etc.) vive em `docs/api/`:
>
> - [Binary client (Cougar TCP :60000)](../api/binary-client.md)
> - [REST client (HTTP/HTTPS JSON)](../api/rest-client.md)
> - [CGI client (HTTP/HTTPS multipart)](../api/cgi-client.md)

## Onde está o código

| Local | O que tem |
| ----- | --------- |
| [`src/core/itscam_sdk.h`](../../src/core/itscam_sdk.h) | Header umbrella -- `#include` único para toda a superfície pública. |
| [`src/core/itscam_client.h`](../../src/core/itscam_client.h) | `ItscamClient` -- binary TCP (porta 60000). |
| [`src/core/itscam_rest_client.h`](../../src/core/itscam_rest_client.h) | `ItscamRestClient` -- HTTP/HTTPS JSON. |
| [`src/core/itscam_cgi_client.h`](../../src/core/itscam_cgi_client.h) | `ItscamCgiClient` -- HTTP/HTTPS para CGI endpoints. |
| [`src/core/itscam_types.h`](../../src/core/itscam_types.h) | `Result<T>`, `Future<T>`, `Error`, `LogLevel`, enums. |
| [`src/core/itscam_sdk_utils.h`](../../src/core/itscam_sdk_utils.h) | Helpers (versão, conversões). |
| [`src/core/3rdparty/`](../../src/core/3rdparty/) | Dependências vendoradas (cpp-httplib, nlohmann/json, mbedTLS). |
| [`src/examples/`](../../src/examples/) | Quatro programas C++ standalone. |

## Integração com o SDK pré-compilado (recomendado)

O pacote de distribuição (`itscam-sdk-<version>.tar.gz`) já inclui headers e shared libraries prontos para uso. Baixe na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>

g++ -std=c++17 \
    -I$SDK/linux-x64/cpp/include \
    -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L$SDK/linux-x64/cpp/lib -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN' \
    -o your_app

# Copie a .so para junto do binário:
cp $SDK/linux-x64/cpp/lib/libitscam_sdk.so* ./
```

| Arquivo | Plataforma |
| ------- | ---------- |
| `libitscam_sdk.so.1.0.0` (+ soname + symlink) | Linux shared |
| `itscam_sdk.dll` + `libitscam_sdk.a` | Windows |

Nada de dependência de sistema para TLS: o **mbedTLS 3.6 LTS** é statically linked. Veja [`docs/https-tls.md`](../https-tls.md).

## Build a partir do source (avançado)

Se você precisa compilar o SDK do zero (contribuidores, cross-compile, debug):

```bash
git clone https://github.com/pumatronix/itscam-sdk.git && cd itscam-sdk
make lib            # libitscam_sdk.{so,a} -> src/core/build/linux/
make windows        # itscam_sdk.dll (cross MinGW) -> src/core/build/windows/
make examples       # binários C++ em src/examples/build/
make docker-all     # tudo dentro da builder image (recomendado)
```

Para linkar contra o source tree, use os headers em `src/core/` e os artefatos em `src/core/build/linux/`:

```bash
g++ -std=c++17 -Isrc/core/ -Isrc/core/3rdparty/ -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L./src/core/build/linux/ -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN:./src/core/build/linux' \
    -o your_app
```

Veja [getting-started.md](../getting-started.md) para o walkthrough completo de build, link e Docker.

## Idiomas C++

| Padrão | API | Notas |
| ------ | --- | ----- |
| Result type | `Result<T>` em [`itscam_types.h`](../../src/core/itscam_types.h) | `bool` operator + `.value()` / `.error()`. Sempre cheque antes de usar. |
| Async | `Future<T>` | `.get()`, `.get(ms)`, `.isReady()`, `waitAll()`. |
| Errors | `Error::Code` enum | Mapeada para os tipos de erro de cada wrapper. Veja [error-handling.md](../error-handling.md). |
| Lifetime | Todos os clients são **move-only** (PIMPL) | Não copie -- `std::move` quando precisar transferir ownership. |
| Threading | Callbacks rodam na worker thread do SDK | **Não bloqueie**: enfileire o payload e retorne. |
| Logging | `client.setLogHandler([](LogLevel lvl, const std::string& msg) {...})` | Por client; default = silencioso. |
| HTTPS | `setBaseUrl(host, 443, "https")` + `setCaCertFile()` ou `setCaCertData()` | mbedTLS vendorado. |

### Result e Future

```cpp
#include "itscam_sdk.h"

itscam::ItscamCgiClient cgi;
cgi.setBaseUrl("192.168.254.254", 80);

auto last = cgi.getLastFrame();
if (!last) {
    std::cerr << "falhou: " << last.error().message << "\n";
    return 1;
}
const auto& img = last.value();   // CgiImage { mimeType, data }
```

```cpp
auto future = cgi.getSnapshotAsync(snapReq);
// ... outras coisas ...
if (auto images = future.get(std::chrono::seconds(15))) {
    // images.value() é std::vector<CgiImage>
}
```

## Uso rápido por client

### CGI (auth opcional)

```cpp
#include "itscam_sdk.h"
#include <fstream>

using namespace itscam;
ItscamCgiClient cgi;
cgi.setBaseUrl("192.168.254.254", 80);
// cgi.setBaseUrl("camera.example.com", 443, "https");
// cgi.setVerifyServerCertificate(false);   // só para demo / dev
// cgi.login("admin", "1234");              // somente quando blockAPI=true

auto last = cgi.getLastFrame().value();
std::ofstream("lastframe.jpg", std::ios::binary)
    .write(reinterpret_cast<const char*>(last.data.data()),
           static_cast<std::streamsize>(last.data.size()));

SnapshotCgiRequest req;
req.quality = 80;
auto images = cgi.getSnapshot(req).value();
// images.size() > 1 quando multi-exposure está ativo

cgi.startMjpegStream([](const CgiStreamFrame& f) {
    // Roda na worker thread do SDK. Não bloqueie.
});
std::this_thread::sleep_for(std::chrono::seconds(5));
cgi.stopMjpegStream();
```

### REST (auth obrigatória)

```cpp
ItscamRestClient rest;
rest.setBaseUrl("192.168.254.254", 80);
rest.login("admin", "1234");

auto profiles = rest.getProfiles().value();   // typed POCO

nlohmann::json patch = {{"trigger", {{"enabled", false}}}};
rest.patchJson("/api/image/profiles/0", patch);

auto raw = rest.httpGet("/api/equipment/misc/readonly/volatile").value();
```

Detalhe completo (typed helpers, partial PUT semantics e quando usar `httpGet` / `httpPut`) em [docs/api/rest-client.md](../api/rest-client.md).

### Binary (Cougar TCP :60000)

```cpp
ItscamClient camera;
camera.connect("192.168.254.254");
camera.authenticate("1234");
camera.subscribeCaptures();

auto result = camera.captureSnapshot();
if (result) {
    const auto& shots = result.value();   // std::vector<CaptureResult>
    // shots[i].jpeg  -- bytes JPEG
    // shots[i].info  -- frame metadata
}

camera.onTriggerImage([](const TriggerImage& t) {
    // Roda na worker thread do SDK. Não bloqueie.
});
```

Auto-reconnect, exposure groups e eventos de GPIO/serial estão em [docs/api/binary-client.md](../api/binary-client.md).

## Examples

Em [`src/examples/`](../../src/examples/):

| Programa                                                                                       | Demonstra                                          |
| ---------------------------------------------------------------------------------------------- | -------------------------------------------------- |
| [`itscam_sdk_example.cpp`](../../src/examples/itscam_sdk_example.cpp)                          | Binary client connect + capture + eventos.         |
| [`itscam_rest_example.cpp`](../../src/examples/itscam_rest_example.cpp)                        | REST login + leitura de configuration.             |
| [`itscam_cgi_example.cpp`](../../src/examples/itscam_cgi_example.cpp)                          | CGI lastframe + snapshot + MJPEG (auth opcional).  |
| [`itscam_trigger_recorder.cpp`](../../src/examples/itscam_trigger_recorder.cpp)                | Trigger burst recorder em disco (binary client).   |

Build com `make examples`; binários em `src/examples/build/`.

## Tutorial passo a passo

Para criar um projeto C++ do zero e salvar a primeira imagem em disco, veja [Primeira imagem com C++](../tutorials/first-image-cpp.md).
