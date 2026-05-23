import { copyFileSync, mkdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { collectCorpusPaths, resolveRepoPath } from "./corpus-manifest.mjs";

const DOCS_SITE = join(import.meta.dirname, "..");
const CONTENT = join(DOCS_SITE, "content");

function ensureDir(filePath) {
  mkdirSync(dirname(filePath), { recursive: true });
}

/** Map repository paths to VitePress content paths (markdown only). */
function toContentPath(repoPath) {
  if (repoPath.startsWith("docs/")) {
    return repoPath.slice("docs/".length);
  }
  if (repoPath === "AGENTS.md") {
    return "agents.md";
  }
  return null;
}

function copyMarkdown(repoPath) {
  const contentPath = toContentPath(repoPath);
  if (!contentPath || !contentPath.endsWith(".md")) return;
  const dest = join(CONTENT, contentPath);
  ensureDir(dest);
  copyFileSync(resolveRepoPath(repoPath), dest);
}

function writeIndex() {
  writeFileSync(
    join(CONTENT, "index.md"),
    `---
layout: home
title: ITSCAM Client SDK
titleTemplate: Pumatronix
hero:
  name: ITSCAM Client SDK
  text: Integração cross-platform para câmeras
  tagline: Binary TCP, REST e CGI clients para ITSCAM450 / ITSCAM600, com bindings C++, C#, Python e Go.
  actions:
    - theme: brand
      text: Começar
      link: /getting-started
    - theme: alt
      text: Perguntar ao assistant
      link: /assistant
features:
  - title: Três client surfaces
    details: ItscamClient para TCP em real time na porta 60000, ItscamRestClient para equipment configuration e ItscamCgiClient para endpoints HTTP de imagem.
  - title: HTTPS built in
    details: mbedTLS 3.6 LTS fica vendored e statically linked, sem dependências TLS de sistema.
  - title: Wrapper parity
    details: Os bindings C#, Python e Go expõem os mesmos três clients por meio da C ABI compartilhada.
  - title: AI assistant
    details: Pesquise nos docs com Cmd/Ctrl+K ou abra o chat assistant para respostas contextualizadas e implementation examples.
---

[Português (Brasil)](/) | [English (US)](https://github.com/pumatronix/itscam-sdk/blob/main/README.en-US.md)

## Links rápidos

| Use case | C++ | C# / .NET | Python | Go |
| -------- | --- | --------- | ------ | -- |
| Binary capture / triggers | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/examples/itscam_sdk_example.cpp) | -- | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/python/examples/capture_example.py) | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/go/examples/capture_example.go) |
| REST configuration | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/examples/itscam_rest_example.cpp) | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/csharp/examples/CaptureExample/Program.cs) | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/python/examples/rest_example.py) | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/go/examples/rest_example.go) |
| CGI snapshot / MJPEG | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/examples/itscam_cgi_example.cpp) | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/csharp/examples/CaptureExample/Program.cs) | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/python/examples/cgi_snapshot_example.py) | [Example](https://github.com/pumatronix/itscam-sdk/blob/main/src/wrappers/go/examples/cgi_snapshot_example.go) |

## Build

\`\`\`bash
make lib       # build libitscam_sdk.{so,a}
make examples  # build dos C++ example binaries
make help      # lista todos os targets
\`\`\`

Veja [Getting started](/getting-started) para o walkthrough completo.
`,
  );
}

function writeAssistantPage() {
  writeFileSync(
    join(CONTENT, "assistant.md"),
    `---
title: SDK Assistant
description: Chat com o assistant da documentação do ITSCAM SDK, powered by Cloudflare AI Search.
sidebar: false
aside: false
---

<script setup>
import AssistantPage from '../.vitepress/theme/AssistantPage.vue'
</script>

<AssistantPage />
`,
  );
}

function writeDocsIndex() {
  const raw = readFileSync(resolveRepoPath("docs/README.md"), "utf8")
    .replace("[../README.md](../README.md)", "[Home](/)")
    .replace("(README.en-US.md)", "(/README.en-US)")
    .replace("(overview.md)", "(/overview)")
    .replace("(getting-started.md)", "(/getting-started)")
    .replace("(error-handling.md)", "(/error-handling)")
    .replace("(https-tls.md)", "(/https-tls)")
    .replace("(api/binary-client.md)", "(/api/binary-client)")
    .replace("(api/rest-client.md)", "(/api/rest-client)")
    .replace("(api/cgi-client.md)", "(/api/cgi-client)")
    .replace("(wrappers/csharp.md)", "(/wrappers/csharp)")
    .replace("(wrappers/python.md)", "(/wrappers/python)")
    .replace("(wrappers/go.md)", "(/wrappers/go)")
    .replace("(migration-cougar.md)", "(/migration-cougar)");

  writeFileSync(join(CONTENT, "documentation-index.md"), raw);
}


rmSync(CONTENT, { recursive: true, force: true });
mkdirSync(CONTENT, { recursive: true });

for (const repoPath of collectCorpusPaths()) {
  copyMarkdown(repoPath);
}

writeIndex();
writeAssistantPage();
writeDocsIndex();

console.log(`Synced markdown content into ${CONTENT}`);
