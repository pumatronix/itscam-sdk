# ITSCAM SDK Documentation Website

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

VitePress site published to GitHub Pages, with an optional [Cloudflare AI Search](https://developers.cloudflare.com/ai-search/) assistant for grounded Q&A and implementation examples.

## Local Development

```bash
cd docs-site
npm install   # generates package-lock.json on first run
npm run dev
```

Open http://localhost:5173/itscam-sdk/ (the base path defaults to `/itscam-sdk/`).

Content is synced on every `dev` / `build` via `scripts/sync-content.mjs`:

- `../README.md` -> `content/index.md` (site home, with link rewriting).
- `../README.en-US.md` -> `content/README.en-US.md`.
- `../docs/**/*.md` -> `content/**/*.md` (chapters, wrappers, tutorials, and `*.en-US.md` companions).
- `../AGENTS.md` -> `content/agents.md`.
- `../docs/images/` -> `content/public/images/` (images referenced from the README and the docs).

Editing the README or the files under `docs/` is the only way to change the site. The script rewrites repo-relative links (`docs/foo.md`, `src/...`, `docs-site/`) to either site routes or GitHub URLs as needed.

### Enable the AI Assistant Locally

The production instance (`https://252e40ae-0329-4b41-94f3-ed7ec9885d7f.search.ai.cloudflare.com/`) is hardcoded as the default in [`.vitepress/config.ts`](.vitepress/config.ts), so `npm run dev` already loads the widget — just add `http://localhost:5173` under **Settings -> Public Endpoint -> Authorized hosts** in the [Cloudflare dashboard](https://dash.cloudflare.com/?to=/:account/ai/ai-search).

To point at a different instance (fork, staging, etc.), export `VITE_AI_SEARCH_API_URL` with the base URL **without the `/search` suffix**:

```bash
VITE_AI_SEARCH_API_URL=https://<INSTANCE_ID>.search.ai.cloudflare.com/ npm run dev
```

## Production Deployment

The GitHub Actions workflow [`.github/workflows/docs.yml`](../.github/workflows/docs.yml) builds and deploys to GitHub Pages on pushes to `main`.

### One-Time GitHub Setup

1. **Enable GitHub Pages**: Settings -> Pages -> Source: **GitHub Actions**.
2. **Create an R2 bucket** (e.g. `itscam-sdk-docs-bucket`) in Cloudflare and generate an **R2 API token / Access Key** pair scoped to that bucket with **Object Read & Write** permissions.
3. **Repository variables** (Settings -> Secrets and variables -> Actions -> Variables):

| Variable | Example | Purpose |
| -------- | ------- | ------- |
| `R2_BUCKET_NAME` | `itscam-sdk-docs-bucket` | Enables the `sync-r2-corpus` job and selects the target bucket |
| `VITE_AI_SEARCH_API_URL` | `https://abc123.search.ai.cloudflare.com/` | **Optional** override for the search/chat widget. The default in [`.vitepress/config.ts`](.vitepress/config.ts) already points at the production instance; only set this to switch to a different one. |

4. **Repository secrets**:

| Secret | Purpose |
| ------ | ------- |
| `CF_ACCOUNT_ID` | Cloudflare account ID (used in the `https://<id>.r2.cloudflarestorage.com` endpoint) |
| `R2_ACCESS_KEY_ID` | R2 API token Access Key ID |
| `R2_SECRET_ACCESS_KEY` | R2 API token Secret Access Key |

5. **Cloudflare AI Search public endpoint**: add your GitHub Pages origin to **Authorized hosts**, for example `https://pumatronix.github.io`.

### AI Search Instance Configuration

Recommended settings for the `itscam-sdk-docs` instance:

- **Data source:** R2 bucket `R2_BUCKET_NAME`. The `sync-r2-corpus` job in [`docs.yml`](../.github/workflows/docs.yml) uploads the corpus on every push to `main` via `scripts/stage-corpus.mjs` + `aws s3 sync --delete`. AI Search auto-reindexes whenever the bucket changes.
- **Search mode:** hybrid search.
- **Query rewriting:** enabled.
- **Custom metadata schema:** `type`, `client`, `language`, `source` as text fields.
- **Generation system prompt:** paste the rules from [`AGENTS.md`](../AGENTS.md), especially REST auth required, CGI auth optional, and the three client surfaces.

See [system prompt configuration](https://developers.cloudflare.com/ai-search/configuration/retrieval/system-prompt/).

> Alternative API-based sync (no R2): the legacy `scripts/sync-ai-search.mjs` script is still available for setups that use AI Search **built-in storage** instead of R2. It is no longer wired into CI; run it manually with `CLOUDFLARE_ACCOUNT_ID`, `CLOUDFLARE_API_TOKEN`, and `AI_SEARCH_INSTANCE_ID` set if you prefer that path.

## Markdown style

Published content (`README.md`, `README.en-US.md`, `docs/`, `AGENTS.md`) uses **one line per prose paragraph** — no manual ~80-column wraps. Enable editor soft wrap for comfortable source reading.

To reflow in bulk after large edits:

```bash
node tools/docs/unwrap-markdown.mjs README.md README.en-US.md AGENTS.md docs/**/*.md
```

The script preserves fenced code blocks, tables, lists, blockquotes (including fenced code inside `>`), and headings.

## Scripts

| Script | Description |
| ------ | ----------- |
| `npm run sync-content` | Copies docs and examples into `content/` for VitePress |
| `npm run stage-corpus` | Stages the corpus in `.corpus-staging/` (consumed by CI before `aws s3 sync` to R2) |
| `npm run sync-ai-search` | Manual fallback uploader straight to AI Search built-in storage (requires env vars) |
| `npm run build` | Production build in `.vitepress/dist/` |

## Site Features

- VitePress sidebar mirroring `docs/`
- Local full-text search (offline)
- **Cmd/Ctrl+K** semantic search through Cloudflare `search-modal-snippet` when configured
- Floating **SDK Assistant** through `chat-bubble-snippet`
- Full-page assistant at `/assistant` via Cloudflare `chat-page-snippet` (floating bubble hidden on that route)
