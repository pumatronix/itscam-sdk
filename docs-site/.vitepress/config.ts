import { defineConfig } from "vitepress";

const base = process.env.VITEPRESS_BASE ?? "/itscam-sdk/";
const aiSearchUrl = process.env.VITE_AI_SEARCH_API_URL ?? "";

export default defineConfig({
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
    /^\.\.?\/(?:\.\.\/)*README(?:$|[#.])/,
    /^\.\.?\/(?:\.\.\/)*AGENTS(?:$|[#.])/,
    /^\.\.?\/(?:\.\.\/)*docs\//,
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
        link: "https://github.com/pumatronix/itscam-sdk/blob/main/README.en-US.md",
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
    },
  },
});
