# Wrapper Go

[Português (Brasil)](go.md) | [English (US)](go.en-US.md)

O wrapper Go fica em
[`src/wrappers/go/`](../../src/wrappers/go/) e usa **cgo** sobre a C
API do SDK. A build tag `static` alterna entre dynamic linking
(default; depende de `libitscam_sdk.so` em runtime) e static linking
completo (puxa o static archive mais pthread / stdc++ / m).

## Build e execução

```bash
make lib                                  # build libitscam_sdk.so / .a
make go-example                           # example do binary client
make go-rest-example                      # example REST
make go-cgi-example                       # example CGI

# Dynamic linking em runtime:
LD_LIBRARY_PATH=$PWD/src/core/build/linux \
    ./src/wrappers/go/examples/cgi_snapshot_example 192.168.254.254
```

Static linking:

```bash
cd src/wrappers/go/examples
CGO_CFLAGS="-I$PWD/../../../core" \
go build -tags static -o cgi_snapshot_example cgi_snapshot_example.go
./cgi_snapshot_example 192.168.254.254
```

Os directives `#cgo` em
[`src/wrappers/go/itscam/*.go`](../../src/wrappers/go/itscam/) já
sabem onde ficam os static archives
(`../../../core/build/<platform>/`) relativos ao package, então não
são necessárias flags extras além de `CGO_CFLAGS` para os headers.

## Superfície

```go
import "github.com/pumatronix/itscam-sdk-go/itscam"

// Três clients, todos espelhando 1:1 a superfície C++:
client, _ := itscam.NewClient()
rest,   _ := itscam.NewRestClient()
cgi,    _ := itscam.NewCgiClient()
```

Errors são retornados como valores `error` do Go. O tipo subjacente
carrega o `Error::Code` do SDK, então o caller pode mapear modos de
falha específicos:

```go
if err := cgi.Login("admin", "1234", 10000); err != nil {
    if errors.Is(err, itscam.ErrTimeout) { /* ... */ }
}
```

## Uso do CGI (auth opcional)

```go
cgi, _ := itscam.NewCgiClient()
defer cgi.Close()

_ = cgi.SetBaseUrl("192.168.254.254", 80, "http")
// cgi.SetBaseUrl("camera.example.com", 443, "https")
// cgi.SetVerifyServerCertificate(false)
// cgi.Login("admin", "1234", 10000)   // somente quando blockAPI=true

last, _ := cgi.GetLastFrame(10000)
_ = os.WriteFile("lastframe.jpg", last.Data, 0644)

req := itscam.NewSnapshotCgiRequest()
req.Quality = 80
imgs, _ := cgi.GetSnapshot(req, 15000)

_ = cgi.StartMjpegStream(func(f itscam.CgiStreamFrame) {
    // Roda na worker thread do SDK; não bloqueie.
}, 10000)
time.Sleep(5 * time.Second)
cgi.StopMjpegStream()
```

## Uso do REST (auth obrigatória)

Duas superfícies que coexistem:

* **Typed helpers** (preferencial) -- `rest.GetOcrConfig`,
  `rest.SetOcrConfig`, `rest.GetProfiles` etc. retornam structs Go
  geradas a partir do documento OpenAPI da câmera. Veja
  [`docs/codegen.md`](../codegen.md) para o workflow de regeneração
  (maintainer e downstream).
* **Generic verbs** (escape hatch) -- `rest.Get`, `rest.Put`,
  `rest.Post`, `rest.Delete` retornam o body JSON cru como string.

* **Partial PUT** -- `rest.PatchJSON(path, patch)` envia somente os
  campos que mudaram. Obrigatório para image profiles e recomendado
  para a maioria das configuration updates. Veja
  [`docs/api/rest-client.md`](../api/rest-client.md).

```go
rest, _ := itscam.NewRestClient()
defer rest.Close()

_ = rest.SetBaseUrl("192.168.254.254", 80, "http")
_, _ = rest.Login("admin", "1234", 10000)

// Read-only: typed struct é conveniente.
profiles, _ := rest.GetProfiles(10000)

// Write: partial PUT (não faça round-trip do profile completo).
_, _ = rest.PatchJSON("/api/image/profiles/0", map[string]interface{}{
    "trigger": map[string]interface{}{"enabled": false},
}, 10000)

// Generic verbs:
body, _ := rest.Get("/api/equipment/misc/readonly/constants", 10000)
fmt.Println(body)
```

As structs geradas ficam em
[`src/wrappers/go/itscam/rest_types.go`](../../src/wrappers/go/itscam/rest_types.go);
campos opcionais são modelados como ponteiros, então `nil` faz
round-trip limpo via `encoding/json`.

## Examples

Em [`src/wrappers/go/examples/`](../../src/wrappers/go/examples/):

| Programa                      | Demonstra                                             |
| ----------------------------- | ----------------------------------------------------- |
| `capture_example.go`          | Binary client connect + capture loop.                 |
| `rest_example.go`             | REST login + leitura de configuration.                |
| `cgi_snapshot_example.go`     | CGI lastframe + snapshot + MJPEG (auth opcional).     |
| `gui/`                        | Desktop viewer com Wails; veja `gui/README.md`.       |

## Example Wails GUI

O example em `gui/` é uma aplicação Wails estática que exercita o
SDK a partir de uma UI WebView. Build com:

```bash
make go-gui                # Linux
make go-gui-windows        # cross-compile para Windows
make docker-go-gui         # dentro da builder image do SDK
```

Veja
[`src/wrappers/go/examples/gui/README.md`](../../src/wrappers/go/examples/gui/README.md)
para pré-requisitos (Go 1.21+, Wails CLI, WebKit2GTK).

## Tutorial passo a passo

Para um walkthrough do zero (`go mod init`, configurar cgo e salvar
a primeira imagem em disco), veja
[Primeira imagem com Go](../tutorials/first-image-go.md).
