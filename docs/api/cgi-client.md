# `ItscamCgiClient` -- CGI / multipart API

[Português (Brasil)](cgi-client.md) | [English (US)](cgi-client.en-US.md)

O CGI client mira nos legacy CGI endpoints do camera-daemon que o
webapp backend faz proxy em `/api/*.cgi`. É o caminho recomendado para
obter imagens via HTTP / HTTPS sem assumir o binary client.

Header: [`src/core/itscam_cgi_client.h`](../../src/core/itscam_cgi_client.h).
Example C++: [`src/examples/itscam_cgi_example.cpp`](../../src/examples/itscam_cgi_example.cpp).

> **Referência completa de cada método** (assinatura, parâmetros,
> response types): veja a
> [referência Doxygen do C++](/api-ref/cpp/classitscam_1_1ItscamCgiClient.html).
> Esta página foca em padrões de uso (anonymous vs autenticado,
> snapshot vs streaming, multi-exposure).

## Quick start

```cpp
#include "itscam_sdk.h"
#include <fstream>

int main() {
    using namespace itscam;
    ItscamCgiClient cgi;
    cgi.setBaseUrl("192.168.254.254", 80);

    // Autenticação é opcional na configuration default da câmera -- veja abaixo.

    auto last = cgi.getLastFrame();
    std::ofstream("lastframe.jpg", std::ios::binary)
        .write(reinterpret_cast<const char*>(last.value().data.data()),
               static_cast<std::streamsize>(last.value().data.size()));

    SnapshotCgiRequest req; req.quality = 80;
    auto images = cgi.getSnapshot(req).value();
    // images.size() > 1 quando multi-exposure está ativo (um por step,
    // a menos que mosaic = true).

    cgi.startMjpegStream([](const CgiStreamFrame& f) {
        // Chamado na worker thread do SDK. Não bloqueie.
        // f.data é o payload JPEG codificado.
    });
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cgi.stopMjpegStream();
}
```

## Autenticação é opcional

O CGI proxy do camera daemon é gateado pelo flag Redis
`configCgi.blockAPI` no webapp backend. Ele **vem como `false` por
default** em toda câmera enviada, o que significa que requests
anonymous são aceitas. Só chame os credential helpers quando o
operador habilitou explicitamente:

```cpp
cgi.login("admin", "1234");                // POST /api/auth, armazena JWT
cgi.setAuthToken("eyJ...");                // JWT pré-existente
cgi.setBasicAuth("admin", "1234");         // HTTP Basic
cgi.clearAuthToken();
cgi.clearBasicAuth();
```

Os quatro example programs (C++, Python, Go, C#) rodam anonymously por
default e aceitam flags opt-in `--user` / `--password`.

## Endpoints

| Método                                            | Endpoint                | Notas                                                          |
| ------------------------------------------------- | ----------------------- | -------------------------------------------------------------- |
| `getLastFrame(timeoutMs = 10000)`                 | `GET /api/lastframe.cgi`| Devolve o JPEG de preview mais recente.                         |
| `getSnapshot(SnapshotCgiRequest&, timeoutMs)`     | `GET /api/snapshot.cgi` | Trigger + capture. Multipart em multi-exposure.                 |
| `getSnapshotAsync(SnapshotCgiRequest&, timeoutMs)`| `GET /api/snapshot.cgi` | Igual, mas devolve um `Future`.                                 |
| `startMjpegStream(callback, timeoutMs)`           | `GET /api/mjpegvideo.cgi`| Stream contínuo `multipart/x-mixed-replace`.                    |
| `stopMjpegStream()`                               | (cancela worker)        | Junta a worker thread.                                          |
| `isMjpegStreamRunning()`                          | --                      |                                                                |
| `forceTrigger(timeoutMs = 10000)`                 | `GET /api/trigger.cgi?force=1` | Trigger one-shot.                                       |
| `reboot(timeoutMs = 10000)`                       | `GET /api/reboot.cgi`   | Reinicia o processo do camera-daemon.                           |
| `httpGetRaw(path, headers, timeoutMs)`            | `GET <path>`            | Escape hatch genérico que devolve `Result<CgiResponse>`.        |

> `trigger.cgi` é um streaming protocol não-standard (responses HTTP
> back-to-back numa única socket) que o cpp-httplib não consegue
> parsear continuamente. Use `ItscamClient::onTriggerImage` para
> trigger events em tempo real.

## `SnapshotCgiRequest`

Espelha os URL parameters aceitos pelo plugin `snapshot.cgi` do daemon:

```cpp
struct SnapshotCgiRequest {
    std::vector<int> shutters;                       // -> s=
    std::vector<int> gains;                          // -> g=
    int              quality   = -1;                 // -> q=, -1 = default do server
    bool             mosaic    = false;              // -> m=1
    std::string      format;                         // -> fmt= ("", "png")
    int              scenario  = -1;                 // -> c=
    std::string      crop;                           // -> r= "x0,y0,x1,y1"
    std::string      textOverlay;                    // -> t=
    std::map<std::string, std::string> userMetadata; // -> User_<key>=<value>
};
```

O return type é **sempre** `std::vector<CgiImage>`:

| Response do server                           | Tamanho do vector                        |
| -------------------------------------------- | ---------------------------------------- |
| `image/jpeg` (exposure única)                | 1                                        |
| `image/png` (exposure única, `fmt=png`)      | 1                                        |
| `image/jpeg` mosaic (mosaic = true)          | 1                                        |
| `multipart/related; boundary=snapshot`       | N (um entry por exposure step)           |

`CgiImage` carrega `mimeType`, `data` (bytes) e um map `headers` para
que applications possam recuperar `Content-Type`, `X-Frame-Index`,
etc. de cada part.

## Streaming MJPEG

`startMjpegStream` sobe uma worker thread que decodifica o body
`multipart/x-mixed-replace; boundary=MjpegBoundary` frame por frame e
invoca o callback para cada `CgiStreamFrame`. Pare com
`stopMjpegStream()`, que cancela a request em andamento e junta a
worker.

```cpp
cgi.startMjpegStream(
    [](const CgiStreamFrame& f) {
        // f.data       -- bytes JPEG
        // f.mimeType   -- e.g. "image/jpeg"
        // f.headers    -- headers de cada part
        // f.sequence   -- contador de frame monotonicamente crescente
    },
    /*timeoutMs=*/ 10000);
```

O callback roda na worker thread do SDK; jogue trabalho pesado
(decoding, disk I/O) para a sua própria queue.

## Logging

```cpp
cgi.setLogHandler([](LogLevel lvl, const std::string& msg) {
    std::cout << "[CGI:" << (lvl == LogLevel::Error ? "ERR " : "INFO")
              << "] " << msg << '\n';
});
```

Veja [`docs/error-handling.md`](../error-handling.md) para o modelo
compartilhado `Result<T>` / `Error` e
[`docs/https-tls.md`](../https-tls.md) para a configuration de HTTPS.
