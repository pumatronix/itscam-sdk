# ITSCAM SDK Documentation Website

[Português (Brasil)](README.md) | [English (US)](README.en-US.md)

VitePress site published to GitHub Pages, with an optional
[Cloudflare AI Search](https://developers.cloudflare.com/ai-search/)
assistant for grounded Q&A and implementation examples.

## Local Development

```bash
cd docs-site
npm install   # generates package-lock.json on first run
npm run dev
```

Open http://localhost:5173/itscam-sdk/ (the base path defaults to
`/itscam-sdk/`).

Content is synced from `../docs/`, `../README.md`, `../AGENTS.md`, and
example sources on every `dev` / `build` via `scripts/sync-content.mjs`.

### Enable the AI Assistant Locally

1. Create an AI Search instance in the [Cloudflare dashboard](https://dash.cloudflare.com/?to=/:account/ai/ai-search).
2. Enable **Settings -> Public Endpoint** and add `http://localhost:5173`
   to **Authorized hosts**.
3. Run the dev server with your public endpoint URL:

```bash
VITE_AI_SEARCH_API_URL=https://<INSTANCE_ID>.search.ai.cloudflare.com/ npm run dev
```

## Production Deployment

The GitHub Actions workflow
[`.github/workflows/docs.yml`](../.github/workflows/docs.yml) builds and
deploys to GitHub Pages on pushes to `main`.

### One-Time GitHub Setup

1. **Enable GitHub Pages**: Settings -> Pages -> Source: **GitHub Actions**.
2. **Repository variables**: Settings -> Secrets and variables -> Actions -> Variables.

| Variable | Example | Purpose |
| -------- | ------- | ------- |
| `VITE_AI_SEARCH_API_URL` | `https://abc123.search.ai.cloudflare.com/` | Embeds chat/search UI snippets |
| `AI_SEARCH_INSTANCE_ID` | `itscam-sdk-docs` | Triggers the corpus sync job |
| `AI_SEARCH_NAMESPACE` | `default` | Optional namespace |

3. **Repository secrets**:

| Secret | Purpose |
| ------ | ------- |
| `CLOUDFLARE_ACCOUNT_ID` | Cloudflare account ID |
| `CLOUDFLARE_API_TOKEN` | Token with **AI Search:Edit** and **AI Search:Run** |

4. **Cloudflare AI Search public endpoint**: add your GitHub Pages origin
   to **Authorized hosts**, for example `https://pumatronix.github.io`.

### AI Search Instance Configuration

Recommended settings for the `itscam-sdk-docs` instance:

- **Data source:** built-in storage, populated by CI through `scripts/sync-ai-search.mjs`.
- **Search mode:** hybrid search.
- **Query rewriting:** enabled.
- **Custom metadata schema:** `type`, `client`, `language`, `source` as text fields.
- **Generation system prompt:** paste the rules from [`AGENTS.md`](../AGENTS.md),
  especially REST auth required, CGI auth optional, and the three client surfaces.

See [system prompt configuration](https://developers.cloudflare.com/ai-search/configuration/retrieval/system-prompt/).

## Scripts

| Script | Description |
| ------ | ----------- |
| `npm run sync-content` | Copies docs and examples into `content/` for VitePress |
| `npm run sync-ai-search` | Uploads the corpus to Cloudflare AI Search (requires env vars) |
| `npm run build` | Production build in `.vitepress/dist/` |

## Site Features

- VitePress sidebar mirroring `docs/`
- Local full-text search (offline)
- **Cmd/Ctrl+K** semantic search through Cloudflare `search-modal-snippet` when configured
- Floating **SDK Assistant** through `chat-bubble-snippet`
- Full-page assistant at `/assistant`
