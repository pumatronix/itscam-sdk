import { defineConfig } from "vitepress";
import { withMermaid } from "vitepress-plugin-mermaid";

const base = process.env.VITEPRESS_BASE ?? "/itscam-sdk/";

// Cloudflare AI Search public endpoint that backs the embedded chat/search
// widgets (search-modal-snippet, chat-bubble-snippet, chat-page-snippet).
// The widget appends /search, /chat/completions and /assets/<version>/...
// internally, so this MUST be the base URL with a trailing slash, never
// the /search API endpoint. Override per-environment with the
// VITE_AI_SEARCH_API_URL env var (or repo variable in CI).
const DEFAULT_AI_SEARCH_API_URL =
  "https://252e40ae-0329-4b41-94f3-ed7ec9885d7f.search.ai.cloudflare.com/";
const aiSearchUrl =
  process.env.VITE_AI_SEARCH_API_URL || DEFAULT_AI_SEARCH_API_URL;

/** Prefix a path under content/public/ with the VitePress base URL.
 *  Required for sidebar links to static .html API refs: VITE_EXTRA_EXTENSIONS
 *  treats .html as a static asset, so client-side normalizeLink() skips withBase(). */
function publicAsset(path: string): string {
  return `${base}${path.replace(/^\//, "")}`.replace(/(?<!:)\/\/+/g, "/");
}

export default withMermaid(
  defineConfig({
  title: "ITSCAM SDK",
  description:
    "SDK cross-platform para câmeras Pumatronix ITSCAM: binary TCP, REST e CGI clients.",
  lang: "pt-BR",
  base,
  srcDir: "content",
  cleanUrls: true,
  lastUpdated: true,

  // Markdown across docs/ intentionally links into the repository source
  // tree (e.g. ../../src/core/itscam_sdk.h, ../README, ../Dockerfile) so
  // GitHub renders the same files correctly. Those targets sit outside
  // the synced VitePress content/ folder, so we tell the build to skip
  // them instead of failing.
  ignoreDeadLinks: [
    /^\.\.?\/(?:\.\.\/)*src\//,
    /^\.\.?\/(?:\.\.\/)*tools\//,
    /^\.\.?\/(?:\.\.\/)*Dockerfile/,
    /^\.\.?\/(?:\.\.\/)*Makefile/,
    /^\.\.?\/(?:\.\.\/)*README(?:$|[#.])/,
    /^\.\.?\/(?:\.\.\/)*AGENTS(?:$|[#.])/,
    /^\.\.?\/(?:\.\.\/)*docs\//,
    // The /api-ref/<lang>/ trees are produced by Doxygen / pdoc /
    // DocFX / gomarkdoc into content/public/api-ref/ at build time
    // (see Makefile docs-api targets). Skip dead-link checks for them
    // so the docs site still builds locally when only some generators
    // have run.
    /^\/api-ref\//,
  ],

  themeConfig: {
    logo: "/logo.svg",
    siteTitle: "ITSCAM SDK",
    nav: [
      { text: "Guia", link: "/overview" },
      { text: "API", link: "/api/binary-client" },
      { text: "Wrappers", link: "/wrappers/python" },
      { text: "Assistant", link: "/assistant" },
      {
        text: "English",
        link: "/README.en-US",
      },
      {
        text: "GitHub",
        link: "https://github.com/pumatronix/itscam-sdk",
      },
    ],
    sidebar: [
      {
        text: "Introdução",
        items: [
          { text: "Overview", link: "/overview" },
          { text: "Getting started", link: "/getting-started" },
          { text: "Índice da documentação", link: "/documentation-index" },
        ],
      },
      {
        text: "Fundamentos",
        items: [
          { text: "Error handling", link: "/error-handling" },
          { text: "HTTPS / TLS", link: "/https-tls" },
        ],
      },
      {
        text: "API reference",
        items: [
          { text: "Binary client (TCP 60000)", link: "/api/binary-client" },
          { text: "REST client", link: "/api/rest-client" },
          { text: "CGI client", link: "/api/cgi-client" },
        ],
      },
      {
        text: "Referência gerada (API ref)",
        items: [
          // Doxygen / DocFX / pdoc HTML lives under
          // content/public/api-ref/<lang>/ and is shipped verbatim by
          // VitePress. Links use absolute .html paths so the router
          // does not try to resolve them as markdown routes.
          { text: "C / C++ (Doxygen)", link: publicAsset("api-ref/cpp/index.html"), target: "_self" },
          { text: "Python (pdoc)", link: publicAsset("api-ref/python/itscam.html"), target: "_self" },
          { text: "C# / .NET (DocFX)", link: publicAsset("api-ref/csharp/index.html"), target: "_self" },
          // gomarkdoc emits a single markdown page that VitePress
          // routes natively at /api-ref/go.
          { text: "Go (gomarkdoc)", link: "/api-ref/go" },
        ],
      },
      {
        text: "Language wrappers",
        items: [
          { text: "C++ (nativo)", link: "/wrappers/cpp" },
          { text: "Python", link: "/wrappers/python" },
          { text: "Go", link: "/wrappers/go" },
          { text: "C# / .NET", link: "/wrappers/csharp" },
        ],
      },
      {
        text: "Tutoriais",
        items: [
          { text: "Primeira imagem -- C++", link: "/tutorials/first-image-cpp" },
          { text: "Primeira imagem -- C# / .NET", link: "/tutorials/first-image-csharp" },
          { text: "Primeira imagem -- Python", link: "/tutorials/first-image-python" },
          { text: "Primeira imagem -- Go", link: "/tutorials/first-image-go" },
        ],
      },
      {
        text: "Histórico",
        items: [
          { text: "Migration from CougarClient", link: "/migration-cougar" },
        ],
      },
      {
        text: "Reference",
        items: [
          { text: "AI agent briefing (AGENTS.md)", link: "/agents" },
        ],
      },
      {
        text: "AI assistant",
        items: [{ text: "Chat com os docs", link: "/assistant" }],
      },
    ],
    socialLinks: [
      {
        icon: "github",
        link: "https://github.com/pumatronix/itscam-sdk",
      },
    ],
    search: {
      provider: "local",
    },
    footer: {
      message: "Copyright © 2026 Pumatronix Equipamentos Eletrônicos",
      copyright: "Software proprietário. Entre em contato com a Pumatronix para licenciamento.",
    },
  },

  vite: {
    vue: {
      template: {
        compilerOptions: {
          isCustomElement: (tag) => tag.endsWith("-snippet"),
        },
      },
    },
    define: {
      __AI_SEARCH_API_URL__: JSON.stringify(aiSearchUrl),
      __AI_SEARCH_SNIPPET_VERSION__: JSON.stringify(
        process.env.VITE_AI_SEARCH_SNIPPET_VERSION ?? "v0.0.25",
      ),
      // Generated API refs (Doxygen / pdoc / DocFX) ship as static HTML
      // under content/public/api-ref/. Without this, VitePress intercepts
      // in-site .html links and shows its 404 page instead of loading the
      // files from public/ (Go works because it is a native markdown route).
      "import.meta.env.VITE_EXTRA_EXTENSIONS": JSON.stringify("html"),
    },
  },

  mermaid: {
    theme: "neutral",
  },
  }),
);
