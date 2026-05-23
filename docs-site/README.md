# ITSCAM SDK -- Site de documentação

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

Site VitePress publicado no GitHub Pages, com assistant opcional via
[Cloudflare AI Search](https://developers.cloudflare.com/ai-search/)
para Q&A contextualizado e implementation examples.

## Desenvolvimento local

```bash
cd docs-site
npm install   # generates package-lock.json on first run
npm run dev
```

Abra http://localhost:5173/itscam-sdk/ (o base path default é
`/itscam-sdk/`).

O content é sincronizado a cada `dev` / `build` via
`scripts/sync-content.mjs`:

- `../README.md` -> `content/index.md` (home page do site, com link rewriting).
- `../README.en-US.md` -> `content/README.en-US.md`.
- `../docs/**/*.md` -> `content/**/*.md` (capítulos, wrappers, tutoriais
  e a versão EN dos arquivos `*.en-US.md`).
- `../AGENTS.md` -> `content/agents.md`.
- `../docs/images/` -> `content/public/images/` (imagens referenciadas
  pelo README e pelos docs).

Editar README ou os arquivos em `docs/` é o único caminho para mudar o
site. O script reescreve links repo-relativos (`docs/foo.md`,
`src/...`, `docs-site/`) para rotas do site ou URLs do GitHub conforme
necessário.

### Habilitar o AI assistant localmente

1. Crie uma instância de AI Search no [Cloudflare dashboard](https://dash.cloudflare.com/?to=/:account/ai/ai-search).
2. Habilite **Settings -> Public Endpoint** e adicione
   `http://localhost:5173` em **Authorized hosts**.
3. Rode o dev server com a URL do public endpoint:

```bash
VITE_AI_SEARCH_API_URL=https://<INSTANCE_ID>.search.ai.cloudflare.com/ npm run dev
```

## Deploy em production

O workflow do GitHub Actions
[`.github/workflows/docs.yml`](../.github/workflows/docs.yml) faz build
e deploy no GitHub Pages em pushes para `main`.

### Setup inicial no GitHub

1. **Habilite GitHub Pages**: Settings -> Pages -> Source: **GitHub Actions**.
2. **Repository variables**: Settings -> Secrets and variables -> Actions -> Variables.

| Variable | Example | Purpose |
| -------- | ------- | ------- |
| `VITE_AI_SEARCH_API_URL` | `https://abc123.search.ai.cloudflare.com/` | Embeds dos snippets de chat/search UI |
| `AI_SEARCH_INSTANCE_ID` | `itscam-sdk-docs` | Aciona o corpus sync job |
| `AI_SEARCH_NAMESPACE` | `default` | Namespace opcional |

3. **Repository secrets**:

| Secret | Purpose |
| ------ | ------- |
| `CLOUDFLARE_ACCOUNT_ID` | Cloudflare account ID |
| `CLOUDFLARE_API_TOKEN` | Token with **AI Search:Edit** and **AI Search:Run** |

4. **Cloudflare AI Search public endpoint**: adicione a origin do GitHub
   Pages em **Authorized hosts**, por exemplo `https://pumatronix.github.io`.

### Configuration da instância AI Search

Settings recomendados para a instância `itscam-sdk-docs`:

- **Data source:** built-in storage, populado pelo CI via `scripts/sync-ai-search.mjs`.
- **Search mode:** hybrid search.
- **Query rewriting:** enabled.
- **Custom metadata schema:** `type`, `client`, `language`, `source` como text fields.
- **Generation system prompt:** cole as regras de [`AGENTS.md`](../AGENTS.md),
  especialmente REST auth required, CGI auth optional e as três client surfaces.

Veja [system prompt configuration](https://developers.cloudflare.com/ai-search/configuration/retrieval/system-prompt/).

## Scripts

| Script | Description |
| ------ | ----------- |
| `npm run sync-content` | Copia docs + examples para `content/` no VitePress |
| `npm run sync-ai-search` | Faz upload do corpus para Cloudflare AI Search (requer env vars) |
| `npm run build` | Production build em `.vitepress/dist/` |

## Site features

- VitePress sidebar mirroring `docs/`
- Local full-text search (offline)
- **Cmd/Ctrl+K** semantic search via Cloudflare `search-modal-snippet`, quando configurado
- Floating **SDK Assistant** via `chat-bubble-snippet`
- Full-page assistant em `/assistant`
