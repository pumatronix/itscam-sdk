![ITSCAM Client SDK](./docs/images/itscam-sdk-repo-logo.jpg)

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

**Documentação:** [pumatronix.github.io/itscam-sdk](https://pumatronix.github.io/itscam-sdk/)

Biblioteca cross-platform para integrar câmeras ITSCAM da [Pumatronix](https://www.pumatronix.com) (ITSCAM450 / ITSCAM600). O SDK já inclui suporte a HTTPS e o backend mbedTLS, sem dependências externas além de um compilador C++.

O SDK expõe três classes de cliente independentes em C++, C#, Python, Go, Java e Node.js. Escolha o client pelo protocolo e pelo tipo de tarefa. Eles podem rodar lado a lado no mesmo processo. O binary client usa o protocolo **Cougar** na porta TCP **60000**. REST e CGI usam HTTP/HTTPS nas portas **80/443**.

| Client | Transport | Use para | Não use para |
| ------ | --------- | -------- | ------------ |
| **`ItscamClient`** | **Cougar** binário TCP **:60000** | Triggers e snapshots em real time, GPIO/serial, multi-exposure groups, controle de **image-pipeline** (profiles, trigger/exposure, JPEG, overlays/crops), baixa latência e auto-reconnect. | Administração do **equipmento**, como networking, timezone, FTP, licenses, OCR/lanes/analytics e outras configurações do webapp/daemon. Use REST. |
| **`ItscamRestClient`** | HTTP/HTTPS JSON | Administração do **equipmento** e daemon via webapp backend: networking, timezone, OCR, classifier, lanes, analytics, ITSCAM PRO, typed config helpers e partial JSON updates. | Event stream Cougar em real time, ajustes de pipeline de baixa latência ou image streaming contínuo. Use o binary client para pipeline e faça login antes de qualquer chamada REST. |
| **`ItscamCgiClient`** | HTTP/HTTPS multipart | `snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`, forced triggers e endpoints de imagem simples. Credentials são opcionais. O acesso é anonymous por default. | Configuration. Use Cougar para pipeline settings ou REST para equipment. |

## Links rápidos

### Tutoriais de primeiros passos

Crie um projeto do zero e salve a primeira imagem em disco - escolha a linguagem do seu projeto:

| Linguagem | Tutorial |
| --------- | -------- |
| C++ | [Primeira imagem em C++](docs/tutorials/first-image-cpp.md) |
| C# / .NET | [Primeira imagem em C#](docs/tutorials/first-image-csharp.md) |
| Python | [Primeira imagem em Python](docs/tutorials/first-image-python.md) |
| Go | [Primeira imagem em Go](docs/tutorials/first-image-go.md) |
| Java | [Primeira imagem em Java](docs/tutorials/first-image-java.md) |
| Node.js | [Primeira imagem em Node.js](docs/tutorials/first-image-nodejs.md) |

### Casos de uso

Escolha uma linha pelo **que você quer fazer** e uma coluna pela **linguagem do seu projeto**. Cada célula aponta para um example pronto para executar.

| Use case | C++ | C# / .NET | Python | Go | Java | Node.js |
| -------- | --- | --------- | ------ | -- | ---- | ------- |
| Binary capture / triggers (real time, TCP 60000) | [`itscam_sdk_example.cpp`](src/examples/itscam_sdk_example.cpp) | [`BinaryCaptureExample/Program.cs`](src/wrappers/csharp/examples/BinaryCaptureExample/Program.cs) | [`capture_example.py`](src/wrappers/python/examples/capture_example.py) | [`capture_example.go`](src/wrappers/go/examples/capture_example.go) | [`CaptureExample.java`](src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/CaptureExample.java) | [`capture-example.js`](src/wrappers/nodejs/examples/capture-example.js) |
| Trigger-burst recorder to disk | [`itscam_trigger_recorder.cpp`](src/examples/itscam_trigger_recorder.cpp) | -- | -- | -- | -- | -- |
| Equipment / REST configuration (login required) | [`itscam_rest_example.cpp`](src/examples/itscam_rest_example.cpp) | [`CaptureExample/Program.cs`](src/wrappers/csharp/examples/CaptureExample/Program.cs) | [`rest_example.py`](src/wrappers/python/examples/rest_example.py) | [`rest_example.go`](src/wrappers/go/examples/rest_example.go) | [`RestExample.java`](src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/RestExample.java) | [`rest-example.js`](src/wrappers/nodejs/examples/rest-example.js) |
| CGI snapshot / lastframe / MJPEG (auth optional) | [`itscam_cgi_example.cpp`](src/examples/itscam_cgi_example.cpp) | [`CaptureExample/Program.cs`](src/wrappers/csharp/examples/CaptureExample/Program.cs) | [`cgi_snapshot_example.py`](src/wrappers/python/examples/cgi_snapshot_example.py) | [`cgi_snapshot_example.go`](src/wrappers/go/examples/cgi_snapshot_example.go) | [`CgiSnapshotExample.java`](src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/CgiSnapshotExample.java) | [`cgi-snapshot-example.js`](src/wrappers/nodejs/examples/cgi-snapshot-example.js) |
| Snapshot ↔ Freeflow swap (CGI + REST + RAC + voting) | [`itscam_snapshot_to_freeflow.cpp`](src/examples/itscam_snapshot_to_freeflow.cpp) | -- | [`snapshot_to_freeflow_example.py`](src/wrappers/python/examples/snapshot_to_freeflow_example.py) | -- | -- | -- |
| HTTPS / TLS (REST + CGI, mbedTLS) | [`itscam_rest_example.cpp`](src/examples/itscam_rest_example.cpp) (`--https`) | [`CaptureExample/Program.cs`](src/wrappers/csharp/examples/CaptureExample/Program.cs) (`--https`) | [`cgi_snapshot_example.py`](src/wrappers/python/examples/cgi_snapshot_example.py) (`--https`) | [`cgi_snapshot_example.go`](src/wrappers/go/examples/cgi_snapshot_example.go) (`--https`) | [`RestExample.java`](src/wrappers/java/examples/src/main/java/com/pumatronix/itscam/examples/RestExample.java) (`--https`) | [`rest-example.js`](src/wrappers/nodejs/examples/rest-example.js) (`--https`) |
| Desktop GUI viewer (Wails) | -- | -- | -- | [`gui/`](src/wrappers/go/examples/gui/) | -- | -- |

Documentação da API por tipo de cliente (Surface):

| Surface | API guide | Wrapper guide |
| ------- | --------- | ------------- |
| Binary client (Cougar TCP 60000) | [docs/api/binary-client.md](docs/api/binary-client.md) | [C++](docs/wrappers/cpp.md) -- [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md) -- [C#](docs/wrappers/csharp.md) -- [Java](docs/wrappers/java.md) -- [Node.js](docs/wrappers/nodejs.md) |
| REST client (HTTP/HTTPS JSON) | [docs/api/rest-client.md](docs/api/rest-client.md) | [C++](docs/wrappers/cpp.md) -- [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md) -- [C#](docs/wrappers/csharp.md) -- [Java](docs/wrappers/java.md) -- [Node.js](docs/wrappers/nodejs.md) |
| CGI client (HTTP/HTTPS multipart) | [docs/api/cgi-client.md](docs/api/cgi-client.md) | [C++](docs/wrappers/cpp.md) -- [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md) -- [C#](docs/wrappers/csharp.md) -- [Java](docs/wrappers/java.md) -- [Node.js](docs/wrappers/nodejs.md) |

## Começar a usar

Escolha um dos caminhos abaixo.

### Opção 1 - Pacote pré-compilado (integração)

O SDK é distribuído como um pacote pré-compilado (`itscam-sdk-<version>.tar.gz`) que contém headers, shared libraries, NuGet, Python wheel e módulo Go para linux-x64, win-x64 e win-x86. Baixe a versão desejada na [página de releases](https://github.com/pumatronix/itscam-sdk/releases), extraia o pacote e integre direto no seu projeto - não é necessário compilar nada:

```bash
tar xzf itscam-sdk-<version>.tar.gz
cd itscam-sdk-<version>
export SDK=$PWD
```

| Linguagem | Integração rápida |
| --------- | ----------------- |
| **C++ / C** | `g++ -I$SDK/linux-x64/cpp/include ... -L$SDK/linux-x64/cpp/lib -litscam_sdk` |
| **C# / .NET** | `nuget.config` + `dotnet add package ... --version $(… VERSION.json nugetVersion)` |
| **Python** | `pip install $SDK/linux-x64/python/itscam-*.whl` |
| **Go** | `go mod edit -replace=...=$SDK/linux-x64/go/itscam-sdk-go` |
| **Java** | `mvn install:install-file ... -DgeneratePom=true` (veja [wrapper Java](docs/wrappers/java.md)) |
| **Node.js** | `npm install $SDK/linux-x64/nodejs/pumatronix-itscam-sdk-*.tgz` |

Comece por `README-sdk.md` dentro do tarball (layout e install por linguagem). Guia completo em [`docs/getting-started.md`](docs/getting-started.md).

### Opção 2 - Clonar o repositório e compilar

Para contribuidores, debug ou quando você precisa do source completo:

```bash
git clone https://github.com/pumatronix/itscam-sdk.git
cd itscam-sdk
make docker-all     # recomendado: Linux + Windows cross + wrappers
```

Build nativo local (sem Docker), se você já tem GCC/Clang:

```bash
make lib            # libitscam_sdk.{so,a} para Linux
make examples       # build dos quatro C++ example binaries
```

Rodar um example contra a câmera:

```bash
./src/examples/build/itscam_sdk_example 192.168.254.254
./src/examples/build/itscam_rest_example 192.168.254.254 admin 1234
./src/examples/build/itscam_cgi_example 192.168.254.254
```

| Linguagem | Próximo passo (source tree) |
| --------- | -------------------------- |
| **C++ / C** | Link contra `src/core/build/linux/` - veja [linkar no source tree](docs/getting-started.md#linkar-contra-o-source-tree) |
| **C# / .NET** | `make csharp` e examples em [`src/wrappers/csharp/examples/`](src/wrappers/csharp/examples/) |
| **Python** | `make lib` + scripts em [`src/wrappers/python/examples/`](src/wrappers/python/examples/) |
| **Go** | `make go-cgi-example` ou [`src/wrappers/go/examples/`](src/wrappers/go/examples/) |
| **Java** | `make java` e examples em [`src/wrappers/java/examples/`](src/wrappers/java/examples/) |
| **Node.js** | `make nodejs` e scripts em [`src/wrappers/nodejs/examples/`](src/wrappers/nodejs/examples/) |

Outros targets úteis: `make docker-shell` (shell interativo), `make sdk-dist` (gera o pacote pré-compilado), `make help` (lista todos os targets). Cross-compile, linkagem avançada e geração do tarball: [`docs/getting-started.md`](docs/getting-started.md#build-do-sdk-a-partir-do-source).

Todo código nativo fica em [`src/`](src/). Veja [`docs/overview.md`](docs/overview.md) para o layout completo do repository.

## Uso com AI agents

Este repositório inclui [`AGENTS.md`](AGENTS.md), um briefing curto e escaneável para coding agents (Cursor, Copilot, Claude Code, etc.). Leia-o **antes** de pedir mudanças no SDK ou em apps que o embutem.

**Neste repositório**

- Ferramentas compatíveis com [AGENTS.md](https://agents.md/) carregam o arquivo automaticamente na raiz do workspace.
- No chat, cite `@AGENTS.md` ou aponte o agent para a tabela de [Links rápidos](#links-rápidos) e para o example da sua linguagem.
- Peça builds reproduzíveis com `make docker-all` ou `make docker-linux` em vez de instalar toolchains manualmente.

**No seu app que consome o SDK**

- Adicione [`AGENTS.md`](AGENTS.md) ao contexto do agent (referência, `@`-mention ou regra de projeto) para evitar escolhas erradas de client e auth.
- Diga explicitamente: linguagem (C++ / C# / Python / Go), client (`ItscamClient`, `ItscamRestClient` ou `ItscamCgiClient`) e se a tarefa é capture em real time, config REST ou snapshot CGI.

**Regras que os agents devem respeitar** (detalhes em [`AGENTS.md`](AGENTS.md)):

| Tópico | Regra rápida |
| ------ | ------------ |
| Três clients | Cougar **:60000** para pipeline/capture; REST para equipment; CGI para endpoints HTTP de imagem. |
| Auth | REST sempre exige `login`; CGI é anonymous por default - não adicione `cgi.login()` sem opt-in. |
| Novas features | Core C++ → C API → todos os wrappers; mantenha parity C# / Python / Go. |
| REST types | Gerados de `tools/codegen/spec/default.yaml` - use `make codegen`, não edite à mão. |
| Partial updates | Use `patchJson()` / `PatchJSON`; PUT completo de profile retorna HTTP 500. |

**Assistant na documentação**

O [site de docs](docs-site/) (GitHub Pages) pode incluir um assistant via [Cloudflare AI Search](https://developers.cloudflare.com/ai-search/), com corpus sincronizado de `docs/`, examples e `AGENTS.md`. Veja [`docs-site/README.md`](docs-site/README.md) para setup local e deploy.

## Mapa da documentação

Este README é intencionalmente direto. A documentação completa fica em [`docs/`](docs/):

- [Índice da documentação](docs/README.md) -- ponto de partida dos docs.
- [Overview](docs/overview.md) -- o que existe no SDK e onde encontrar.
- [Getting started](docs/getting-started.md) -- build, execução de examples e link do seu app.
- [HTTPS / TLS](docs/https-tls.md) -- mbedTLS vendored, configuration e troubleshooting.
- [Error handling](docs/error-handling.md) -- `Result<T>`, `Future<T>`, error codes e logging.
- [JPEG metadata (COM marker)](docs/jpeg-metadata.md) -- extração e parsing de metadados de reconhecimento/classificação embutidos nas imagens JPEG.
- [Typed REST helpers & codegen](docs/codegen.md) -- snapshot OpenAPI bundled e regeneration workflows.
- API reference: [Binary client](docs/api/binary-client.md) -- [REST client](docs/api/rest-client.md) -- [CGI client](docs/api/cgi-client.md).
- Wrappers: [C++ (nativo)](docs/wrappers/cpp.md) -- [C# / .NET](docs/wrappers/csharp.md) -- [Python](docs/wrappers/python.md) -- [Go](docs/wrappers/go.md) -- [Java](docs/wrappers/java.md) -- [Node.js](docs/wrappers/nodejs.md).
- Tutoriais (primeira imagem em disco): [C++](docs/tutorials/first-image-cpp.md) -- [C# / .NET](docs/tutorials/first-image-csharp.md) -- [Python](docs/tutorials/first-image-python.md) -- [Go](docs/tutorials/first-image-go.md) -- [Java](docs/tutorials/first-image-java.md) -- [Node.js](docs/tutorials/first-image-nodejs.md).
- [Adicionar um novo wrapper](docs/adding-a-new-wrapper.md) -- procedimento canônico para futuros bindings (Rust, Ruby, Swift, ...).
- [Migration from CougarClient](docs/migration-cougar.md).
- [`AGENTS.md`](AGENTS.md) - briefing para coding agents; veja [Uso com AI agents](#uso-com-ai-agents).
- **[Documentation website](docs-site/)** -- site VitePress para GitHub Pages com assistant opcional via [Cloudflare AI Search](https://developers.cloudflare.com/ai-search/). Veja [`docs-site/README.md`](docs-site/README.md) para setup.

## Destaques

- **Três client surfaces, uma library.** Cougar binary para capture em real time e pipeline control. REST para equipment administration. CGI para endpoints HTTP de imagem.
- **HTTPS out of the box.** O mbedTLS 3.6 LTS fica vendored em [`src/core/3rdparty/mbedtls/`](src/core/3rdparty/mbedtls/) e é statically linked em `libitscam_sdk`.
- **Examples auth-aware.** REST sempre exige login. CGI é anonymous por default (`configCgi.blockAPI = false`) e credentials são opt-in (`--user / --password`) em todos os language wrappers.
- **Feature parity** entre os wrappers C#, Python, Go, Java e Node.js por meio da C API em [`src/core/c_api/`](src/core/c_api/).

## Platform support

| Platform | Architectures | Toolchain |
| -------- | ------------- | --------- |
| Linux | x86_64, ARMv7, ARM64 | GCC / Clang |
| Windows | x86_64 | MinGW-w64 |

## Licença e contato

Software proprietário. 
Copyright (c) 2026 Pumatronix Equipamentos Eletrônicos LTDA.
Entre em contato com a Pumatronix para termos de licenciamento.
