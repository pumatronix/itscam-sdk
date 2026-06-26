# Adicionando um novo wrapper de linguagem

[Português (Brasil)](adding-a-new-wrapper.md) | [English (US)](adding-a-new-wrapper.en-US.md)

Este chapter descreve o procedimento canônico para adicionar suporte a uma nova linguagem (Rust, Ruby, Swift, Kotlin, PHP, ...) ao ITSCAM Client SDK. O SDK já expõe seis wrappers — C++, C#, Python, Go, Java e Node.js — e todos seguem o mesmo padrão. Replique o padrão para qualquer linguagem nova.

## 1. Por que existe uma C API?

Todos os wrappers conversam com o mesmo core C++ via uma **C ABI** estável em [`src/core/c_api/`](../src/core/c_api/) (`itscam_sdk_c.h`, `itscam_rest_client_c.h`, `itscam_cgi_client_c.h`).

A regra de ouro está em [`AGENTS.md`](../AGENTS.md):

> Start in `src/core/`, extend the C API, then mirror the surface in **every** wrapper.

Você **não** vai escrever código C++ novo para um wrapper de linguagem. Sua tarefa é traduzir os ~120 entry-points C em uma API idiomática da sua linguagem.

## 2. Decisão #1 — escolher a tecnologia de binding

| Tecnologia | Quando usar |
| ---------- | ----------- |
| **FFI dinâmica** (libffi-style) | Sua linguagem tem uma FFI library madura e bem mantida que carrega `.so`/`.dll` em runtime. **Preferida.** Usa: ctypes (Python), JNA (Java), koffi (Node.js), cgo (Go), P/Invoke (C#), CFFI (Lua/Ruby), Foreign (Common Lisp / Clojure), `ffi` (Rust se quiser dynamic), Foreign Function & Memory API (Java 22+ moderno). |
| **Native module compilado** (JNI/NAPI/Ruby C ext/Python C ext/Rust crate) | Sua linguagem **não** tem FFI dinâmica idiomática **ou** a FFI tem overhead inaceitável **ou** você precisa de tipos especializados (NIO buffers, Buffer JS zero-copy, etc.). Cuidado: você passa a precisar buildar contra os headers do SDK em todas as combinações de OS×arch. |
| **Bindings auto-gerados** (SWIG, cbindgen→bindgen) | Não recomendado como wrapper user-facing — gera surface de baixo nível que rejeita o pattern idiomático que cada linguagem espera. OK como base interna se você for envolver com uma camada idiomática em cima. |

**Default seguro:** FFI dinâmica. Foi a escolha de Python (ctypes), Go (cgo via wrappers), C# (P/Invoke), Java (JNA) e Node.js (koffi). Build determinístico, sem toolchain extra para o consumidor.

## 3. Decisão #2 — escolher o packaging

| Linguagem | Package format |
| --------- | -------------- |
| Python    | wheel via setuptools |
| C# / .NET | NuGet (.nupkg) |
| Go        | módulo + cgo directives |
| Java      | JAR via Maven, com native libs em `META-INF/native/<os>-<arch>/` |
| Node.js   | npm tarball, com native libs em `native/<platform>-<arch>/` |
| Rust      | crate Cargo, com `build.rs` que copia `libitscam_sdk` |
| Ruby      | gem com extensão FFI |
| Swift     | Swift Package + binary target |
| Kotlin / JVM | publicar via mesmo JAR Java (Kotlin é interop nativo) |
| PHP       | Composer com extensão FFI (PHP 8.2+) |

**Regras de empacotamento:**

1. **Native binary embutido por OS×arch.** O consumer não deve precisar instalar `libitscam_sdk` system-wide. Use o staging script pattern (veja `tools/packaging/stage-*-natives.sh`).
2. **Resolução de native lib em ordem documentada.** A ordem padrão é: env var override → diretório do package → `LD_LIBRARY_PATH`/system loader → fallback embutido. Documente isso no README do wrapper.
3. **Versão sincronizada.** A versão do package vem de `tools/version/gen-version.sh`. Adicione um output no script para a sua linguagem (veja como Python, C# e Go fazem).

## 4. Decisão #3 — escolher o async model

| Padrão | Quando usar |
| ------ | ----------- |
| Sync apenas | Linguagens sem suporte ao async idiomático (Lua, shell scripting). |
| Sync + async parallel | Default. Toda blocking call tem uma versão `*Async` que retorna o tipo async nativo (`Promise`, `Future<T>`, `Task<T>`, `coroutine`, ...). Wrapper interno usa `Task.Run` / um executor / `Promise.resolve(() => syncFn())`. |
| Async-first | Só faz sentido se a linguagem **não tem** APIs sync (raramente o caso). |

**Default seguro:** sync + async parallel. C# faz `Task<T>`, Java faz `Future<T>` para compatibilidade com JDK 7, Node faz `Promise<T>`, Python expõe sync (asyncio é opcional).

## 5. Layout de diretório esperado

```
src/wrappers/<linguagem>/
  README.md                  # specifico do wrapper, espelha o estilo dos outros
  <package-manifest>         # pom.xml / setup.py / package.json / Cargo.toml / ...
  <source-tree>/
    bindings.<ext>           # carrega libitscam_sdk e declara as signatures
    errors.<ext>             # ItscamError + subclasses
    itscam_client.<ext>      # ItscamClient
    itscam_rest_client.<ext> # ItscamRestClient
    itscam_cgi_client.<ext>  # ItscamCgiClient
    jpeg_utils.<ext>         # COM marker parser puro (sem native dep)
    types.<ext>              # POJOs / dataclasses / interfaces
  examples/
    capture_example.<ext>    # binary client (TCP 60000)
    rest_example.<ext>       # REST (login required)
    cgi_snapshot_example.<ext>  # CGI (auth opcional)
```

## 6. Checklist de implementação

Use os Java/Node.js wrappers como referência. Cada item é obrigatório (ou explicitamente fora de scope com justificativa no README do wrapper).

### 6.1 Core surface

- [ ] **Lifecycle:** `create` / `destroy` mapeados para construtor + destrutor / `close()` / `Symbol.dispose` / IDisposable / `with`-context / `defer Close()`.
- [ ] **Connection (binary):** `connect`, `disconnect`, `isConnected`, `setAutoReconnect` ou `AutoReconnectConfig` no `connect`.
- [ ] **Authentication:**
    - Binary: `authenticate(password)`.
    - REST: `login(user, pass)` **+ documentação explícita** que REST sempre exige auth.
    - CGI: `login(...)` opt-in, `setBasicAuth`, `setAuthToken`. **NÃO** chame login em código library / examples por default. Veja [`AGENTS.md`](../AGENTS.md) §10 (anti-patterns).
- [ ] **Capture (binary):** `captureSnapshot`, `getLastFrame`, `subscribe`, `subscribeCaptures`.
- [ ] **Profiles (binary):** `getActiveProfileId`, `setActiveProfile`, `listProfiles`.
- [ ] **REST verbs:** `httpGet`, `httpPut`, `httpPost`, `httpDelete`, `patchJson`. **Setters tipados são o caminho preferencial para writes** — usam serialização parcial (fields não setados são omitidos do body PUT). O `patchJson` genérico permanece disponível para payloads não tipados ou endpoints sem typed helper.
- [ ] **REST typed helpers:** Pelo menos `getProfiles`, `getOcrConfig`/`setOcrConfig`, `getAnalyticsConfig`/`setAnalyticsConfig`, `getClassifierConfig`/`setClassifierConfig`, `getLanesConfig`/`setLanesConfig`, `getItscamproConfig`/`setItscamproConfig`, `getVolatileInfo`. Decidir se retorna struct/typed (preferido se a linguagem tem POCOs com codegen, veja §6.6) ou JSON cru.
- [ ] **CGI:** `getLastFrame`, `getSnapshot`, `startMjpegStream`/`stopMjpegStream`, `forceTrigger`, `reboot`.
- [ ] **TLS configuration:** `setBaseUrl(host, port, scheme)`, `setCaCertFile`, `setCaCertData`, `setVerifyServerCertificate`, `setClientCertificate` (em REST e CGI).
- [ ] **System utilities:** `getVersion`, `getSystemLocalTime`, `getSystemUtcTime`, `getEpochTime`, `getEpochTimeMs`, `getLastError`.

### 6.2 Errors

- [ ] Hierarquia de exceção rooted em `ItscamError` (ou `ItscamException`, conforme idioma).
- [ ] Subclasses para `Timeout`, `NotAuthenticated`, `ConnectionFailed`/`Disconnected`, `InvalidParameter`, `ServerError`.
- [ ] Toda chamada checa o `Result::Code` C e converte para a exception correta. Inclui o `ITSCAM_getLastError()` na mensagem.
- [ ] Exception type carrega o numeric code (`getCode()` / `.code` / equivalente) para discriminação programática.

### 6.3 Callbacks / streaming

- [ ] Callbacks (`onTriggerImage`, `onSnapshotImage`, `onConnectionState`, `onDisconnect`, `onLog`, `startMjpegStream`) **sempre** entregues na worker thread do SDK.
- [ ] Documente: **não bloqueie callbacks**. Marshalling (queue / channel / dispatcher) é responsabilidade do user.
- [ ] Mantenha referências fortes para os trampolines/callback objects para impedir GC enquanto native está chamando. Veja `_callbackRefs` em Node, `callbacks` map em Java/Python, `_streamDelegate` em C#.
- [ ] Engole exceptions dentro do callback (com log/trace opcional). Nunca deixe uma exception vazar de volta para C.

### 6.4 JPEG utilities

- [ ] Helpers para extrair o COM marker e parsear `Placa`/`CoordPlaca`/`ClassifierList`/`BMCList`. **Sem native dep** (puro código da linguagem).
- [ ] Mirror exato dos helpers Python ([`jpeg_utils.py`](../src/wrappers/python/itscam/jpeg_utils.py)) e Java ([`JpegMetadata.java`](../src/wrappers/java/itscam-sdk/src/main/java/com/pumatronix/itscam/jpeg/JpegMetadata.java)).

### 6.5 Examples (mínimo 3)

- [ ] **`capture_example`** — binary client, com optional password. Mirror de [`capture_example.py`](../src/wrappers/python/examples/capture_example.py).
- [ ] **`rest_example`** — REST login + typed helpers + generic verbs.
- [ ] **`cgi_snapshot_example`** — CGI: `lastframe`, `snapshot.cgi` (multi-image), MJPEG stream por 5 segundos. Auth `--user/--password` opcional.
- [ ] Todos os examples respeitam o protocolo de flags: `--https`, `--insecure`, `--user`, `--password`. Anonymous-by-default em CGI.

### 6.6 Codegen REST types (typed helpers)

O SDK gera POCOs / dataclasses / structs para as configs REST a partir do snapshot OpenAPI bundled em [`tools/codegen/spec/default.yaml`](../tools/codegen/spec/default.yaml). Veja [`docs/codegen.md`](codegen.md).

Status atual:

| Wrapper | Typed REST POCOs |
| ------- | ---------------- |
| C++ | sim (`itscam_rest_types.h`) |
| C# | sim (`RestTypes/RestTypes.g.cs`) |
| Python | sim (`itscam/rest_types.py`) |
| Go | sim (`itscam/rest_types.go`) |
| Java | sim (`resttypes/`, source mantido com Gson) |
| Node.js | **follow-up** — generic verbs retornam `JsonValue` parseado; codegen vai retornar interfaces TS. |

**Como adicionar codegen para uma nova linguagem:**

1. O tool é [`tools/codegen/codegen.mjs`](../tools/codegen/codegen.mjs), baseado em [quicktype](https://quicktype.io/) que já suporta Java, TypeScript, Kotlin, Swift, Rust, Ruby, Crystal, Elm, Haskell, etc. nativamente.
2. Adicione uma entrada nova ao array `TARGETS` no `codegen.mjs`. Você precisa:
   - `name` (rótulo legível).
   - `lang` (string aceita por quicktype: `"java"`, `"typescript"`, `"rust"`, `"kotlin"`, `"swift"`, `"ruby"`, ...).
   - `rendererOptions` (flags do quicktype específicas dessa linguagem; consulte a doc deles).
   - `outRel` (caminho relativo ao repo root onde o output é gravado).
   - `notice` (header de comentário do arquivo gerado, com SPDX e a frase "AUTO-GENERATED").
   - `postProcess` (opcional — função que recebe a string emitida pelo quicktype e retorna a versão final, idempotente).
3. Atualize `docs/codegen.md` adicionando a sua linha à tabela "Generated outputs".
4. Rode `make codegen` (Node 18+ local) ou `make docker-codegen`. Commit os outputs novos junto com a entrada do `codegen.mjs` num único commit. CI (`make codegen-check`) força a sincronização.

### 6.7 Build / packaging

- [ ] Manifest do package no path canônico (ver §5).
- [ ] Staging script `tools/packaging/stage-<lang>-natives.sh` que copia `src/core/build/<rid>/libitscam_sdk.{so,dll}` para o diretório resources do package.
- [ ] Top-level Makefile com pelo menos:
    - `make <lang>` — build da library.
    - `make <lang>-pack` — produz o artifact distribuível (JAR / .nupkg / .whl / .tgz / .gem / .crate).
    - `make <lang>-examples` — build dos examples.
    - `make docker-<lang>` / `make docker-<lang>-pack` — versão dentro do builder Docker.
- [ ] Atualizar `make help` para listar os targets novos.
- [ ] Atualizar `clean:` para remover artifacts (`target/`, `node_modules/`, `__pycache__/`, `bin/`/`obj/`, etc.).

### 6.8 Versioning

- [ ] Estender [`tools/version/gen-version.sh`](../tools/version/gen-version.sh) para emitir a versão da sua linguagem no formato apropriado:
    - SemVer puro (Maven, npm, Cargo).
    - PEP 440 (Python wheels).
    - .NET-friendly (NuGet 4-part).
- [ ] Wrapper expõe `getVersion()` (ou propriedade equivalente) que retorna a versão do package.

### 6.9 Distribution

- [ ] Estender [`tools/packaging/make-sdk-dist.sh`](../tools/packaging/make-sdk-dist.sh) para incluir o artifact no tarball consumer (`itscam-sdk-<version>.tar.gz`) sob `linux-x64/<lang>/`, `win-x64/<lang>/` etc.
- [ ] `sdk-dist` target no Makefile chama o build da nova linguagem antes do staging.

### 6.10 Documentation

- [ ] **Wrapper guide:** `docs/wrappers/<lang>.md` (PT-BR) + `<lang>.en-US.md` (EN). Espelhe a estrutura de [`docs/wrappers/python.md`](wrappers/python.md): instalação, resolução da native lib, superfície idiomática, três exemplos (binary/REST/CGI), serialização parcial nos typed setters, link para tutorial.
- [ ] **Tutorial:** `docs/tutorials/first-image-<lang>.md` + EN counterpart. Walkthrough do zero, com fallback opcional para o binary client.
- [ ] **Quick-link matrix** no [`README.md`](../README.md) — adicione coluna nova à tabela de use cases e a row de wrapper guides.
- [ ] **Doc index** [`docs/README.md`](README.md) — adicione bullet em "Language wrappers" e "Tutoriais".
- [ ] **API reference auto-gerada** (opcional mas recomendado): adicione `docs-api-<lang>` no Makefile (Doxygen para C++, pdoc para Python, DocFX para C#, gomarkdoc para Go, javadoc para Java, typedoc para TS/JS, rustdoc para Rust, ...).

## 7. Anti-patterns

Não:

- **Não** crie uma C API paralela. Tudo passa por `src/core/c_api/`. Se o C API não cobre o que você precisa, estenda o C API (e propague para todos os outros wrappers; veja [`AGENTS.md`](../AGENTS.md) §11).
- **Não** mude o naming idiomático para "ficar consistente com C++". Cada wrapper segue o naming da sua linguagem (`camelCase` / `snake_case` / `PascalCase`/...).
- **Não** adicione `cgi.login()` em código não opt-in. CGI é anonymous por default ([`AGENTS.md`](../AGENTS.md) §10).
- **Não** silenciosamente ignore `Result<T>` ou error codes. Levante exception / retorne error.
- **Não** dispare callbacks na thread do user. Sempre na worker do SDK; o user marshals.
- **Não** edite à mão arquivos gerados pelo codegen (eles têm header "AUTO-GENERATED"). Estenda `codegen.mjs` em vez disso.
- **Não** regenere a native lib do zero ao buildar o wrapper. O wrapper depende de `libitscam_sdk.{so,dll}` produzido por `make lib` / `make windows`.

## 8. PR checklist final

Antes de abrir o PR adicionando o wrapper novo:

- [ ] Todo o checklist da §6 está coberto ou tem uma justificativa explícita no README do wrapper (ex.: "Codegen REST helpers — follow-up rastreado em #NNN").
- [ ] `make all` passa (build de tudo + wrapper novo).
- [ ] `make docker-all` passa (mesmo, dentro do Docker; reproduzível para CI).
- [ ] `make codegen-check` passa, se o wrapper participa do codegen.
- [ ] `make regression-examples CAMERA_IP=<ip>` passa contra uma câmera viva — incluindo o capture/rest/cgi examples do wrapper novo.
- [ ] Versão é gerada por `tools/version/gen-version.sh` (ou tem fallback hardcoded em `0.0.0` enquanto a integração de versão não foi feita; deixe um TODO na shell script).
- [ ] PR mention `AGENTS.md` se algum wrapper rule mudou (ex.: novos error codes que precisam ser refletidos em todos os wrappers).
- [ ] Quick-link matrix atualizado no `README.md` e em `docs/README.md`.

## Veja também

- [`AGENTS.md`](../AGENTS.md) — briefing curto para coding agents; §3-§6 explicam auth, threading, errors e wrapper layout.
- [`docs/codegen.md`](codegen.md) — workflow de codegen para typed REST helpers.
- [Wrapper guides](README.md#language-wrappers) — guias existentes em `docs/wrappers/` (use como referência).
- [`tools/codegen/codegen.mjs`](../tools/codegen/codegen.mjs) — pipeline de geração de POCOs.
- [`tools/version/gen-version.sh`](../tools/version/gen-version.sh) — single source of truth para versionamento.
