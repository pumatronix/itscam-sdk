# Primeira imagem com Go

Walkthrough do zero: criar um módulo Go, configurar cgo contra o
ITSCAM SDK e salvar a primeira imagem JPEG da câmera em disco. Caminho
principal usa o **`ItscamCgiClient`** (HTTP, anônimo por default) e há
uma seção opcional no final usando o **`ItscamClient`** (Cougar
TCP :60000).

```mermaid
flowchart TD
  prereq[Pre-requisitos] --> sdk[Build do SDK]
  sdk --> project[go mod init]
  project --> replace[Substituir modulo local]
  replace --> code[Escrever main.go]
  code --> run[go run]
  run --> verify[Verificar primeira-imagem.jpg]
  verify --> binary[Opcional: binary TCP :60000]
```

## 1. Pré-requisitos

| Item | Versão mínima | Verificar com |
| ---- | ------------- | ------------- |
| Go | 1.21+ | `go version` |
| Compilador C / C++17 | para cgo + `make lib` | `gcc --version`, `g++ --version` |
| GNU make | qualquer | `make --version` |
| Git | qualquer | `git --version` |
| Câmera ITSCAM | ITSCAM450 / ITSCAM600 alcançável na rede | `ping <ip-da-camera>` |

## 2. Buildar a shared library nativa

O wrapper Go usa cgo: dynamic linking precisa de `libitscam_sdk.so` em
runtime; static linking precisa de `libitscam_sdk.a` em build time.
Build dos dois com:

```bash
git clone https://github.com/pumatronix/itscam-sdk.git
cd itscam-sdk
make lib
```

Output em `src/core/build/linux/`.

## 3. Criar o projeto

A partir da raiz do checkout do SDK:

```bash
mkdir -p meu-app && cd meu-app
go mod init exemplo.com/meu-app
```

## 4. Apontar para o módulo Go local do SDK

O wrapper Go vive em
[`src/wrappers/go/`](../../src/wrappers/go/) sob o module path
`github.com/pumatronix/itscam-sdk-go`. Enquanto o módulo não estiver
publicado, use um `replace` directive para apontar para o checkout
local:

```bash
go mod edit -require=github.com/pumatronix/itscam-sdk-go@v0.0.0
go mod edit -replace=github.com/pumatronix/itscam-sdk-go=../src/wrappers/go
```

Confirme o `go.mod`:

```text
module exemplo.com/meu-app

go 1.21

require github.com/pumatronix/itscam-sdk-go v0.0.0

replace github.com/pumatronix/itscam-sdk-go => ../src/wrappers/go
```

## 5. Escrever o código mínimo

```go
// main.go
package main

import (
	"fmt"
	"log"
	"os"

	"github.com/pumatronix/itscam-sdk-go/itscam"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "uso: meu-app <ip-da-camera>")
		os.Exit(1)
	}
	host := os.Args[1]

	cgi, err := itscam.NewCgiClient()
	if err != nil {
		log.Fatal(err)
	}
	defer cgi.Close()

	if err := cgi.SetBaseUrl(host, 80, "http"); err != nil {
		log.Fatal(err)
	}
	// Para HTTPS:
	//   cgi.SetBaseUrl(host, 443, "https")
	// Para auth opcional (somente se configCgi.blockAPI=true):
	//   cgi.Login("admin", "1234", 10000)

	frame, err := cgi.GetLastFrame(10000)
	if err != nil {
		log.Fatalf("lastframe.cgi falhou: %v", err)
	}

	if err := os.WriteFile("primeira-imagem.jpg", frame.Data, 0644); err != nil {
		log.Fatal(err)
	}

	fmt.Printf("OK: %d bytes salvos em primeira-imagem.jpg (%s)\n",
		len(frame.Data), frame.MimeType)
}
```

`go mod tidy` para resolver dependências transitivas:

```bash
go mod tidy
```

## 6. Executar

Dynamic linking (mais simples para começar):

```bash
LD_LIBRARY_PATH=../src/core/build/linux \
    go run main.go 192.168.254.254
```

Static linking (binário sem dependência de runtime):

```bash
CGO_CFLAGS="-I../src/core" \
    go build -tags static -o meu-app main.go
./meu-app 192.168.254.254
```

Saída esperada:

```text
OK: 87421 bytes salvos em primeira-imagem.jpg (image/jpeg)
```

Verifique o arquivo:

```bash
file primeira-imagem.jpg
# primeira-imagem.jpg: JPEG image data, JFIF standard 1.01, ...
```

## 7. Troubleshooting

| Sintoma | Causa provável | Solução |
| ------- | -------------- | ------- |
| `error while loading shared libraries: libitscam_sdk.so.1` | shared library não encontrada | Exporte `LD_LIBRARY_PATH=../src/core/build/linux` ou builde com `-tags static`. |
| `# github.com/pumatronix/itscam-sdk-go/itscam ... fatal error: itscam_sdk_c.h: No such file or directory` | cgo não acha os headers | Static: passe `CGO_CFLAGS="-I../src/core"`. Dynamic: confirme o `replace` no `go.mod`. |
| `ErrConn` / `ErrTimeout` | IP errado ou porta 80 bloqueada | `curl -v http://<ip>/api/lastframe.cgi -o /dev/null` |
| `ErrAuth` em CGI | A câmera tem `configCgi.blockAPI=true` | Chame `cgi.Login("user", "pass", 10000)` antes do `GetLastFrame`. |
| Erro de SSL/TLS em HTTPS | CA bundle não configurado | `cgi.SetCaCertFile("/etc/ssl/certs/ca-bundle.pem")` ou, só em dev, `cgi.SetVerifyServerCertificate(false)`. |

## 8. Opcional: capture via `ItscamClient` (TCP :60000)

O CGI é o caminho mais simples. Se você precisa de **trigger em real
time** ou multi-exposure, use o binary client (porta 60000):

```go
// main.go (variante binary)
package main

import (
	"fmt"
	"log"
	"os"
	"time"

	"github.com/pumatronix/itscam-sdk-go/itscam"
)

func main() {
	host := os.Args[1]
	password := "1234"
	if len(os.Args) > 2 {
		password = os.Args[2]
	}

	camera, err := itscam.NewClient()
	if err != nil {
		log.Fatal(err)
	}
	defer camera.Close()

	if err := camera.Connect(host, 60000, 5*time.Second); err != nil {
		log.Fatal(err)
	}
	if err := camera.Authenticate(password, 10*time.Second); err != nil {
		log.Fatal(err)
	}
	if err := camera.SubscribeCaptures(
		itscam.DefaultCaptureSubscriptionConfig(), 5*time.Second); err != nil {
		log.Fatal(err)
	}

	results, err := camera.CaptureSnapshot(10 * time.Second)
	if err != nil || len(results) == 0 {
		log.Fatalf("captureSnapshot falhou: %v", err)
	}

	if err := os.WriteFile("primeira-imagem-binary.jpg",
		results[0].JpegData, 0644); err != nil {
		log.Fatal(err)
	}
	fmt.Printf("OK: %d bytes (binary)\n", len(results[0].JpegData))
}
```

Detalhe completo (auto-reconnect, exposure groups, eventos de trigger
contínuos) em [docs/api/binary-client.md](../api/binary-client.md) e
no example
[`capture_example.go`](../../src/wrappers/go/examples/capture_example.go).

## Próximos passos

- [Guia do wrapper Go](../wrappers/go.md) -- API completa, static vs
  dynamic linking, Wails GUI.
- [Examples Go](../../src/wrappers/go/examples/) -- CGI, REST, binary
  e GUI Wails.
- [HTTPS / TLS](../https-tls.md) -- configurar mbedTLS para produção.
- [Codegen REST](../codegen.md) -- regenerar `rest_types.go` para um
  firmware específico.
