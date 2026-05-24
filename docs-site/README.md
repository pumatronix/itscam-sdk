# ITSCAM SDK -- Site de documentação

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

Site VitePress publicado no GitHub Pages, com assistant opcional via [Cloudflare AI Search](https://developers.cloudflare.com/ai-search/) para Q&A contextualizado e implementation examples.

## Desenvolvimento local

```bash
cd docs-site
npm install   # generates package-lock.json on first run
npm run dev
```

Abra http://localhost:5173/itscam-sdk/ (o base path default é `/itscam-sdk/`).

O content é sincronizado a cada `dev` / `build` via `scripts/sync-content.mjs`:

- `../README.md` -> `content/index.md` (home page do site, com link rewriting).
- `../README.en-US.md` -> `content/README.en-US.md`.
- `../docs/**/*.md` -> `content/**/*.md` (capítulos, wrappers, tutoriais e a versão EN dos arquivos `*.en-US.md`).
- `../AGENTS.md` -> `content/agents.md`.
- `../docs/images/` -> `content/public/images/` (imagens referenciadas pelo README e pelos docs).

Editar README ou os arquivos em `docs/` é o único caminho para mudar o site. O script reescreve links repo-relativos (`docs/foo.md`, `src/...`, `docs-site/`) para rotas do site ou URLs do GitHub conforme necessário.

### Habilitar o AI assistant localmente

A instância default (`https://252e40ae-0329-4b41-94f3-ed7ec9885d7f.search.ai.cloudflare.com/`) está hardcoded em [`.vitepress/config.ts`](.vitepress/config.ts), então `npm run dev` já carrega o widget — basta adicionar `http://localhost:5173` em **Settings -> Public Endpoint -> Authorized hosts** no [Cloudflare dashboard](https://dash.cloudflare.com/?to=/:account/ai/ai-search).

Para apontar para outra instância (fork, staging, etc.), exporte `VITE_AI_SEARCH_API_URL` com a base URL **sem o sufixo `/search`**:

```bash
VITE_AI_SEARCH_API_URL=https://<INSTANCE_ID>.search.ai.cloudflare.com/ npm run dev
```

## Deploy em production

O workflow do GitHub Actions [`.github/workflows/docs.yml`](../.github/workflows/docs.yml) faz build e deploy no GitHub Pages em pushes para `main`.

### Setup inicial no GitHub

1. **Habilite GitHub Pages**: Settings -> Pages -> Source: **GitHub Actions**.
2. **Crie um R2 bucket** (ex.: `itscam-sdk-docs-bucket`) na Cloudflare e gere um par **R2 API token / Access Key** com permissão **Object Read & Write** apenas nesse bucket.
3. **Repository variables** (Settings -> Secrets and variables -> Actions -> Variables):

| Variable | Example | Purpose |
| -------- | ------- | ------- |
| `R2_BUCKET_NAME` | `itscam-sdk-docs-bucket` | Aciona o job `sync-r2-corpus` e define o bucket alvo |
| `VITE_AI_SEARCH_API_URL` | `https://abc123.search.ai.cloudflare.com/` | Override **opcional** do widget de search/chat. O default em [`.vitepress/config.ts`](.vitepress/config.ts) já aponta para a instância de produção; só configure se precisar de outra. |

4. **Repository secrets**:

| Secret | Purpose |
| ------ | ------- |
| `CF_ACCOUNT_ID` | Cloudflare account ID (usado no endpoint `https://<id>.r2.cloudflarestorage.com`) |
| `R2_ACCESS_KEY_ID` | Access Key ID do R2 API token |
| `R2_SECRET_ACCESS_KEY` | Secret Access Key do R2 API token |

5. **Cloudflare AI Search public endpoint**: adicione a origin do GitHub Pages em **Authorized hosts**, por exemplo `https://pumatronix.github.io`.

### Configuration da instância AI Search

Settings recomendados para a instância `itscam-sdk-docs`:

- **Data source:** R2 bucket `R2_BUCKET_NAME`. O job `sync-r2-corpus` em [`docs.yml`](../.github/workflows/docs.yml) faz upload do corpus a cada push para `main` via `scripts/stage-corpus.mjs` + `aws s3 sync --delete`. A AI Search reindexa automaticamente quando o bucket muda.
- **Search mode:** hybrid search.
- **Query rewriting:** enabled.
- **Custom metadata schema:** `type`, `client`, `language`, `source` como text fields.
- **Generation system prompt:** cole as regras de [`AGENTS.md`](../AGENTS.md), especialmente REST auth required, CGI auth optional e as três client surfaces.

Veja [system prompt configuration](https://developers.cloudflare.com/ai-search/configuration/retrieval/system-prompt/).

> Sync alternativo via API (sem R2): o script legado `scripts/sync-ai-search.mjs` ainda existe para casos em que a AI Search está configurada com **built-in storage** em vez de R2. Não é executado pelo CI; rode manualmente com `CLOUDFLARE_ACCOUNT_ID`, `CLOUDFLARE_API_TOKEN` e `AI_SEARCH_INSTANCE_ID` setados se quiser usar esse caminho.

## Estilo do markdown

O conteúdo publicado (`README.md`, `README.en-US.md`, `docs/`, `AGENTS.md`) usa **um parágrafo por linha** em prose — sem quebras manuais em ~80 colunas. Use soft wrap no editor para ler o source confortavelmente.

Para reflow em lote após edits grandes:

```bash
node tools/docs/unwrap-markdown.mjs README.md README.en-US.md AGENTS.md docs/**/*.md
```

O script preserva fenced code blocks, tabelas, listas, blockquotes (incluindo code blocks dentro de `>`) e headings.

## Scripts

| Script | Description |
| ------ | ----------- |
| `npm run sync-content` | Copia docs + examples para `content/` no VitePress |
| `npm run stage-corpus` | Stage do corpus em `.corpus-staging/` (consumido pelo CI antes do `aws s3 sync` para R2) |
| `npm run sync-ai-search` | Upload alternativo direto via AI Search API (built-in storage); manual, requer env vars |
| `npm run build` | Production build em `.vitepress/dist/` |

## Site features

- VitePress sidebar mirroring `docs/`
- Local full-text search (offline)
- **Cmd/Ctrl+K** semantic search via Cloudflare `search-modal-snippet`, quando configurado
- Floating **SDK Assistant** via `chat-bubble-snippet`
- Full-page assistant em `/assistant` via Cloudflare `chat-page-snippet` (bubble flutuante oculto nessa rota)
