# Wrapper Node.js

[Português (Brasil)](nodejs.md) | [English (US)](nodejs.en-US.md)

O wrapper Node.js fica em [`src/wrappers/nodejs/`](../../src/wrappers/nodejs/) e usa **[koffi](https://koffi.dev/)** sobre a C API do SDK. Suporta Node.js 16+ no Linux, Windows e macOS. As declarations TypeScript ficam em [`types/index.d.ts`](../../src/wrappers/nodejs/types/index.d.ts) — JavaScript puro e TypeScript funcionam out of the box.

> O wrapper expõe as três superfícies de cliente do SDK:
>
> | Classe | Contraparte nativa |
> | ------ | ------------------ |
> | `ItscamClient` | `src/core/itscam_client.h` |
> | `ItscamRestClient` | `src/core/itscam_rest_client.h` |
> | `ItscamCgiClient` | `src/core/itscam_cgi_client.h` |

## Instalação

### Via pacote SDK pré-compilado (recomendado)

O pacote de distribuição (`itscam-sdk-<version>.tar.gz`) inclui um tarball npm com os native binaries embutidos sob `native/<platform>-<arch>/`. Baixe na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
npm install $PWD/itscam-sdk-<version>/linux-x64/nodejs/pumatronix-itscam-sdk-<version>.tgz
```

Depois disso, no seu projeto:

```js
const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');
```

A native lib é resolvida automaticamente; nenhum `LD_LIBRARY_PATH` é necessário.

### Build a partir do source (avançado)

Se você está desenvolvendo dentro do source tree do SDK:

```bash
make lib                        # build libitscam_sdk.so primeiro
make nodejs                     # stage native + npm install
make nodejs-pack                # produz @pumatronix/itscam-sdk-x.y.z.tgz
make docker-nodejs              # mesma coisa dentro do Docker
```

`make nodejs` faz três coisas:

1. Compila `libitscam_sdk.so` se ainda não existe.
2. Roda `tools/packaging/stage-nodejs-natives.sh`, que copia os binaries pré-compilados em `src/core/build/<rid>/` para `src/wrappers/nodejs/native/<platform>-<arch>/`.
3. Roda `npm install` para baixar `koffi`.

`make nodejs-pack` adiciona um `npm pack`, produzindo um tarball `pumatronix-itscam-sdk-<version>.tgz` self-contained pronto para `npm install ./that.tgz` ou `npm publish`.

## Resolução da native lib

`src/native.js` procura `libitscam_sdk` na seguinte ordem:

1. `process.env.ITSCAM_SDK_LIBRARY` (path absoluto).
2. `native/<platform>-<arch>/<libname>` ao lado do wrapper.
3. `native/<libname>` (legacy fallback).
4. `src/core/build/linux/`, `src/core/build/win-x64/` (dev tree do SDK).
5. `LD_LIBRARY_PATH` (Linux/macOS) / `PATH` (Windows).

`<platform>` é `process.platform` (`linux`, `darwin`, `win32`); `<arch>` é `process.arch` (`x64`, `arm64`, `arm`, `ia32`).

## Superfície idiomática

| Aspecto | O que você ganha |
| ------- | ---------------- |
| Async | Toda blocking call tem `*Async` retornando `Promise<T>`. |
| Lifetime | Todos os clients têm `close()` (e `Symbol.dispose` para `using` em Node 22+ ou TS 5.2+). |
| Streaming | `startMjpegStream(callback)` invoca o callback na worker thread do SDK. |
| Errors | Hierarquia `Error` rooted em `ItscamError` (`ItscamTimeoutError`, `ItscamAuthError`, ...). |
| TS | Declarations completas em `types/index.d.ts` — IntelliSense funciona em JS e TS. |

## Uso do CGI (auth opcional)

```js
const fs = require('fs');
const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');

const cgi = new ItscamCgiClient();
try {
    cgi.setBaseUrl('192.168.254.254', 80, 'http');
    // cgi.setBaseUrl('camera.example.com', 443, 'https');
    // cgi.setVerifyServerCertificate(false);
    // cgi.login('admin', '1234', 10000);   // somente quando blockAPI=true

    const last = cgi.getLastFrame();
    fs.writeFileSync('lastframe.jpg', last.data);

    const images = cgi.getSnapshot({ quality: 80 });
    images.forEach((img, i) => fs.writeFileSync(`snap-${i}.jpg`, img.data));

    cgi.startMjpegStream((frame) => {
        // Roda na worker thread do SDK; não bloqueie.
    });
    await new Promise(r => setTimeout(r, 5000));
    cgi.stopMjpegStream();
} finally {
    cgi.close();
}
```

## Uso do REST (auth obrigatória)

```js
const { ItscamRestClient } = require('@pumatronix/itscam-sdk');

const rest = new ItscamRestClient();
try {
    rest.setBaseUrl('192.168.254.254', 80, 'http');
    rest.login('admin', '1234');

    const profiles = rest.get('/api/image/profiles');

    // Partial PUT -- preferencial para image profiles
    rest.patchJson('/api/image/profiles/0',
                   { trigger: { enabled: false } });
} finally {
    rest.close();
}
```

* **Generic verbs** (preferencial): `get`, `put`, `post`, `delete`, `patchJson`. Retornam JSON parseado (objeto / array) ou string raw quando o body não é JSON.
* **Typed convenience helpers**: `getProfiles`, `setOcrConfig` etc. Retornam JSON parseado também. Typed POCOs / codegen para Node é um follow-up — veja [`docs/codegen.md`](../codegen.md) para o status.
* **Partial PUT** — `rest.patchJson(path, partialBody)` envia somente os campos que mudaram. Obrigatório para `PUT /api/image/profiles/{id}` (que rejeita body completo com HTTP 500). Veja [`docs/api/rest-client.md`](../api/rest-client.md).

## Uso do binary client

```js
const { ItscamClient } = require('@pumatronix/itscam-sdk');

const camera = new ItscamClient();
try {
    camera.connect('192.168.254.254', 60000, 10000, { enabled: true });
    camera.authenticate('1234');
    camera.subscribeCaptures();

    const frames = camera.captureSnapshot();
    for (const r of frames) {
        require('fs').writeFileSync(`snap-${r.info.requestId}.jpg`, r.jpeg);
    }
} finally {
    camera.close();
}
```

## TypeScript

```ts
import {
    ItscamCgiClient,
    SnapshotCgiRequest,
    CgiStreamFrame,
} from '@pumatronix/itscam-sdk';

const cgi = new ItscamCgiClient();
const images = cgi.getSnapshot({ quality: 80, mosaic: false });
```

As declarations cobrem todos os clients, error types, dataclasses (com `JSDoc` no JS source) e helpers JPEG.

## Examples

Em [`src/wrappers/nodejs/examples/`](../../src/wrappers/nodejs/examples/):

| Script | Demonstra |
| ------ | --------- |
| `capture-example.js <host> [pass]` | Binary client connect + authenticate + snapshot. |
| `rest-example.js <host> <user> <pass>` | REST login + leitura de configuration. |
| `cgi-snapshot-example.js <host> [...]` | `lastframe.cgi`, `snapshot.cgi`, MJPEG streaming. |

Run com:

```bash
make nodejs
node src/wrappers/nodejs/examples/capture-example.js 192.168.254.254 1234
node src/wrappers/nodejs/examples/cgi-snapshot-example.js 192.168.254.254 --user admin --password 1234
```

## Tutorial passo a passo

Para um walkthrough do zero (`npm init`, instalar e salvar a primeira imagem em disco), veja [Primeira imagem com Node.js](../tutorials/first-image-nodejs.md).
