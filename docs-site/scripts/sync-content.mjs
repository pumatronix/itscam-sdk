import {
  copyFileSync,
  cpSync,
  existsSync,
  mkdirSync,
  readdirSync,
  readFileSync,
  rmSync,
  writeFileSync,
} from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { collectCorpusPaths, REPO_ROOT, resolveRepoPath } from "./corpus-manifest.mjs";

const SCRIPT_DIR = dirname(fileURLToPath(import.meta.url));
const DOCS_SITE = join(SCRIPT_DIR, "..");
const CONTENT = join(DOCS_SITE, "content");
const PUBLIC = join(CONTENT, "public");

const GITHUB_BLOB_BASE = "https://github.com/pumatronix/itscam-sdk/blob/main";

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
  // docs/README{,.en-US}.md is rendered via writeDocsIndex below so the
  // links land on the site routes instead of GitHub paths.
  if (contentPath === "README.md" || contentPath === "README.en-US.md") return;
  const dest = join(CONTENT, contentPath);
  ensureDir(dest);
  copyFileSync(resolveRepoPath(repoPath), dest);
}

/**
 * Rewrite repo-relative links so the README content renders correctly when
 * served as a VitePress page. The README is the source of truth for the
 * landing page, so rewriting happens at sync time rather than asking
 * authors to maintain a second copy.
 */
function rewriteReadmeLinks(markdown) {
  let out = markdown;

  // Repository-internal images live under docs/images/. Sync copies them
  // to docs-site/public/images/ so /images/... resolves on the site.
  out = out.replace(/\]\(\.?\/?docs\/images\//g, "](/images/");

  // Language switcher and self-references at the top of the README.
  out = out.replace(/\]\(README\.md([^)]*)\)/g, "](/$1)");
  out = out.replace(/\]\(README\.en-US\.md([^)]*)\)/g, "](/README.en-US$1)");

  // docs/README{,.en-US}.md → /documentation-index{,.en-US}
  out = out.replace(
    /\]\(docs\/README\.en-US\.md([^)]*)\)/g,
    "](/documentation-index.en-US$1)",
  );
  out = out.replace(
    /\]\(docs\/README\.md([^)]*)\)/g,
    "](/documentation-index$1)",
  );

  // docs/foo/bar.md(#anchor) → /foo/bar(#anchor)
  out = out.replace(
    /\]\(docs\/([^)#]+?)\.md(#[^)]*)?\)/g,
    (_m, path, anchor) => `](/${path}${anchor ?? ""})`,
  );

  // AGENTS.md → /agents
  out = out.replace(/\]\(AGENTS\.md([^)]*)\)/g, "](/agents$1)");

  // docs-site/README.md → GitHub (meta-doc that does not belong on the site).
  out = out.replace(
    /\]\(docs-site\/README\.md\)/g,
    `](${GITHUB_BLOB_BASE}/docs-site/README.md)`,
  );
  // docs-site/ index → site root.
  out = out.replace(/\]\(docs-site\/?\)/g, "](/)");

  // Top-level repo files that have no site equivalent.
  for (const file of ["Dockerfile", "Makefile"]) {
    out = out.replace(
      new RegExp(`\\]\\(${file}\\)`, "g"),
      `](${GITHUB_BLOB_BASE}/${file})`,
    );
  }

  // src/... source paths point at the repo, never at the site.
  out = out.replace(
    /\]\((src\/[^)#]+?)(#[^)]*)?\)/g,
    (_m, path, anchor) => `](${GITHUB_BLOB_BASE}/${path}${anchor ?? ""})`,
  );

  return out;
}

function syncImages() {
  const src = join(REPO_ROOT, "docs", "images");
  if (!existsSync(src)) return;
  const dest = join(PUBLIC, "images");
  rmSync(dest, { recursive: true, force: true });
  cpSync(src, dest, { recursive: true });
}

function writeReadmePages() {
  const ptbr = readFileSync(resolveRepoPath("README.md"), "utf8");
  const en = readFileSync(resolveRepoPath("README.en-US.md"), "utf8");

  const ptbrFrontmatter = `---
title: ITSCAM Client SDK
titleTemplate: Pumatronix
---

`;
  const enFrontmatter = `---
title: ITSCAM Client SDK
titleTemplate: Pumatronix
---

`;

  writeFileSync(
    join(CONTENT, "index.md"),
    ptbrFrontmatter + rewriteReadmeLinks(ptbr),
  );
  writeFileSync(
    join(CONTENT, "README.en-US.md"),
    enFrontmatter + rewriteReadmeLinks(en),
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
  const sitePtbr = readFileSync(resolveRepoPath("docs/README.md"), "utf8")
    .replace("[../README.md](../README.md)", "[Home](/)")
    .replace("(README.en-US.md)", "(/documentation-index.en-US)")
    .replace("(README.md)", "(/documentation-index)")
    .replace("(overview.md)", "(/overview)")
    .replace("(getting-started.md)", "(/getting-started)")
    .replace("(error-handling.md)", "(/error-handling)")
    .replace("(https-tls.md)", "(/https-tls)")
    .replace("(api/binary-client.md)", "(/api/binary-client)")
    .replace("(api/rest-client.md)", "(/api/rest-client)")
    .replace("(api/cgi-client.md)", "(/api/cgi-client)")
    .replace("(wrappers/cpp.md)", "(/wrappers/cpp)")
    .replace("(wrappers/csharp.md)", "(/wrappers/csharp)")
    .replace("(wrappers/python.md)", "(/wrappers/python)")
    .replace("(wrappers/go.md)", "(/wrappers/go)")
    .replace("(tutorials/first-image-cpp.md)", "(/tutorials/first-image-cpp)")
    .replace("(tutorials/first-image-csharp.md)", "(/tutorials/first-image-csharp)")
    .replace("(tutorials/first-image-python.md)", "(/tutorials/first-image-python)")
    .replace("(tutorials/first-image-go.md)", "(/tutorials/first-image-go)")
    .replace("(migration-cougar.md)", "(/migration-cougar)");

  writeFileSync(join(CONTENT, "documentation-index.md"), sitePtbr);

  const enPath = resolveRepoPath("docs/README.en-US.md");
  if (existsSync(enPath)) {
    // Chapter pages whose `*.en-US.md` companion exists (other markdown
    // routes still serve English content under their default name).
    const enRouteFor = (slug) =>
      existsSync(resolveRepoPath(`docs/${slug}.en-US.md`))
        ? `/${slug}.en-US`
        : `/${slug}`;

    const siteEn = readFileSync(enPath, "utf8")
      .replace("[../README.md](../README.md)", "[Home](/README.en-US)")
      .replace("(README.en-US.md)", "(/documentation-index.en-US)")
      .replace("(README.md)", "(/documentation-index)")
      .replace("(overview.md)", `(${enRouteFor("overview")})`)
      .replace("(getting-started.md)", `(${enRouteFor("getting-started")})`)
      .replace("(error-handling.md)", `(${enRouteFor("error-handling")})`)
      .replace("(https-tls.md)", `(${enRouteFor("https-tls")})`)
      .replace("(codegen.md)", `(${enRouteFor("codegen")})`)
      .replace("(api/binary-client.md)", `(${enRouteFor("api/binary-client")})`)
      .replace("(api/rest-client.md)", `(${enRouteFor("api/rest-client")})`)
      .replace("(api/cgi-client.md)", `(${enRouteFor("api/cgi-client")})`)
      .replace("(wrappers/cpp.en-US.md)", "(/wrappers/cpp.en-US)")
      .replace("(wrappers/csharp.en-US.md)", "(/wrappers/csharp.en-US)")
      .replace("(wrappers/python.en-US.md)", "(/wrappers/python.en-US)")
      .replace("(wrappers/go.en-US.md)", "(/wrappers/go.en-US)")
      .replace("(tutorials/first-image-cpp.md)", "(/tutorials/first-image-cpp)")
      .replace("(tutorials/first-image-csharp.md)", "(/tutorials/first-image-csharp)")
      .replace("(tutorials/first-image-python.md)", "(/tutorials/first-image-python)")
      .replace("(tutorials/first-image-go.md)", "(/tutorials/first-image-go)")
      .replace("(migration-cougar.md)", `(${enRouteFor("migration-cougar")})`);

    writeFileSync(join(CONTENT, "documentation-index.en-US.md"), siteEn);
  }
}


/**
 * Wipe everything we manage inside content/, but preserve generated API
 * reference output:
 *
 *   content/public/api-ref/  - Doxygen (cpp), pdoc (python), DocFX (csharp)
 *   content/api-ref/         - gomarkdoc (go) markdown rendered by VitePress
 *
 * Those trees are regenerated by `make docs-api-*` and we do not want
 * to wipe them on every `npm run dev`.
 */
function resetContent() {
  if (!existsSync(CONTENT)) {
    mkdirSync(CONTENT, { recursive: true });
    return;
  }

  for (const name of readdirSync(CONTENT)) {
    if (name === "public" || name === "api-ref") continue;
    rmSync(join(CONTENT, name), { recursive: true, force: true });
  }

  const publicDir = join(CONTENT, "public");
  if (existsSync(publicDir)) {
    for (const name of readdirSync(publicDir)) {
      if (name === "api-ref") continue;
      rmSync(join(publicDir, name), { recursive: true, force: true });
    }
  } else {
    mkdirSync(publicDir, { recursive: true });
  }
}

resetContent();

for (const repoPath of collectCorpusPaths()) {
  copyMarkdown(repoPath);
}

syncImages();
writeReadmePages();
writeAssistantPage();
writeDocsIndex();

console.log(`Synced markdown content into ${CONTENT}`);
