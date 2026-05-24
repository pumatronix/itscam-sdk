# Primeira imagem com Node.js

Walkthrough do zero: criar um projeto Node, instalar o pacote `@pumatronix/itscam-sdk` e salvar a primeira imagem JPEG da câmera em disco. Caminho principal usa o **`ItscamCgiClient`** (HTTP, anônimo por default) e há uma seção opcional no final usando o **`ItscamClient`** (Cougar TCP :60000).

## 1. Pré-requisitos

| Item | Versão mínima | Verificar com |
| ---- | ------------- | ------------- |
| Node.js | 16+ | `node --version` |
| npm | qualquer | `npm --version` |
| Pacote SDK | `itscam-sdk-<version>.tar.gz` | extrair e localizar `linux-x64/nodejs/` |
| Câmera ITSCAM | ITSCAM450 / ITSCAM600 alcançável na rede | `ping <ip-da-camera>` |

## 2. Extrair o SDK e instalar o tarball npm

Baixe `itscam-sdk-<version>.tar.gz` na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>

mkdir -p ~/projetos/meu-app && cd ~/projetos/meu-app
npm init -y
npm install $SDK/linux-x64/nodejs/pumatronix-itscam-sdk-<version>.tgz
```

A native lib é embutida no tarball sob `native/<platform>-<arch>/` e resolvida automaticamente — nenhum `LD_LIBRARY_PATH` é necessário.

> **Compilando o SDK do zero?** Se você precisa buildar a partir do source em vez de usar o pacote pré-compilado, veja a [seção avançada de build](../getting-started.md#build-do-sdk-a-partir-do-source). Após `make nodejs-pack`, o tarball fica em `src/wrappers/nodejs/pumatronix-itscam-sdk-<version>.tgz`.

## 3. Escrever o código mínimo

```js
// meu-app.js
'use strict';

const fs = require('fs');
const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');

function main() {
    const host = process.argv[2];
    if (!host) {
        console.error('uso: node meu-app.js <ip-da-camera>');
        process.exit(1);
    }

    const cgi = new ItscamCgiClient();
    try {
        cgi.setBaseUrl(host, 80, 'http');
        // Para HTTPS:
        //   cgi.setBaseUrl(host, 443, 'https');
        // Para auth opcional (somente se configCgi.blockAPI=true):
        //   cgi.login('admin', '1234');

        const frame = cgi.getLastFrame();
        fs.writeFileSync('primeira-imagem.jpg', frame.data);

        console.log(
            `OK: ${frame.data.length} bytes salvos em primeira-imagem.jpg (${frame.mimeType})`);
    } finally {
        cgi.close();
    }
}

main();
```

## 4. Executar

```bash
node meu-app.js 192.168.254.254
```

Saída esperada:

```text
OK: 87421 bytes salvos em primeira-imagem.jpg (image/jpeg)
```

## 5. TypeScript (opcional)

```bash
npm install --save-dev typescript @types/node
npx tsc --init
```

Code:

```ts
// meu-app.ts
import * as fs from 'fs';
import { ItscamCgiClient } from '@pumatronix/itscam-sdk';

const host = process.argv[2];
if (!host) { console.error('uso: ts-node meu-app.ts <ip>'); process.exit(1); }

const cgi = new ItscamCgiClient();
try {
    cgi.setBaseUrl(host, 80, 'http');
    const frame = cgi.getLastFrame();
    fs.writeFileSync('primeira-imagem.jpg', frame.data);
    console.log(`OK: ${frame.data.length} bytes (${frame.mimeType})`);
} finally {
    cgi.close();
}
```

## 6. Troubleshooting

| Sintoma | Causa provável | Solução |
| ------- | -------------- | ------- |
| `Error: Unable to load library` em runtime | Tarball sem native binary para o seu `process.platform`/`process.arch` | Build do source (`make nodejs-pack`) ou exporte `ITSCAM_SDK_LIBRARY=/path/to/libitscam_sdk.so`. |
| `ItscamConnectionError` | IP errado ou porta 80 bloqueada | `curl -v http://<ip>/api/lastframe.cgi -o /dev/null` |
| `ItscamAuthError` em CGI | A câmera tem `configCgi.blockAPI=true` | Chame `cgi.login('user', 'pass')` antes do `getLastFrame()`. |
| TLS errors em HTTPS | CA bundle não configurado | `cgi.setCaCertFile('/etc/ssl/certs/ca-bundle.pem')` ou, só em dev, `cgi.setVerifyServerCertificate(false)`. |

## 7. Opcional: capture via `ItscamClient` (TCP :60000)

```js
const { ItscamClient } = require('@pumatronix/itscam-sdk');

const cam = new ItscamClient();
try {
    cam.connect(host, 60000, 10000);
    cam.authenticate('1234');
    cam.subscribeCaptures();

    const frames = cam.captureSnapshot();
    if (!frames.length) {
        console.error('nenhum frame retornado');
        process.exit(2);
    }
    require('fs').writeFileSync('primeira-imagem-binary.jpg', frames[0].jpeg);
} finally {
    cam.close();
}
```

## Próximos passos

- [Guia do wrapper Node.js](../wrappers/nodejs.md) — API completa + TypeScript.
- [Examples Node.js](../../src/wrappers/nodejs/examples/) — CGI, REST e binary.
- [HTTPS / TLS](../https-tls.md) — configurar mbedTLS para produção.
- [Adicionar novo wrapper](../adding-a-new-wrapper.md) — checklist para futuros bindings.
