/**
 * Canonical list of files indexed by Cloudflare AI Search and the doc site.
 * Keys use forward-slash paths relative to the repository root.
 */
import { existsSync, readdirSync, statSync } from "node:fs";
import { dirname, join, relative, extname } from "node:path";
import { fileURLToPath } from "node:url";

const SCRIPT_DIR = dirname(fileURLToPath(import.meta.url));
const REPO_ROOT = join(SCRIPT_DIR, "../..");

const STATIC_ENTRIES = [
  "README.md",
  "README.en-US.md",
  "docs/README.md",
  "docs/README.en-US.md",
  "AGENTS.md",
];

const EXAMPLE_GLOBS = [
  "src/examples/itscam_sdk_example.cpp",
  "src/examples/itscam_rest_example.cpp",
  "src/examples/itscam_cgi_example.cpp",
  "src/examples/itscam_trigger_recorder.cpp",
  "src/examples/README.md",
  "src/wrappers/python/examples/capture_example.py",
  "src/wrappers/python/examples/rest_example.py",
  "src/wrappers/python/examples/cgi_snapshot_example.py",
  "src/wrappers/go/examples/capture_example.go",
  "src/wrappers/go/examples/rest_example.go",
  "src/wrappers/go/examples/cgi_snapshot_example.go",
  "src/wrappers/csharp/examples/CaptureExample/Program.cs",
  "src/wrappers/csharp/examples/MjpegGrabberExample/Program.cs",
  "src/wrappers/csharp/examples/SoftwareTriggerSnapshotExample/Program.cs",
];

const CPP_API_REF_DIR = join(
  REPO_ROOT,
  "docs-site/content/public/api-ref/cpp",
);

// Public C/C++ SDK surface -- keep in sync with tools/docs/Doxyfile INPUT.
const CORE_API_HEADERS = [
  "src/core/itscam_sdk.h",
  "src/core/itscam_client.h",
  "src/core/itscam_rest_client.h",
  "src/core/itscam_cgi_client.h",
  "src/core/itscam_types.h",
  "src/core/itscam_sdk_utils.h",
  "src/core/itscam_jpeg_utils.h",
  "src/core/itscam_rest_types.h",
];

const CORE_API_SOURCES = [
  "src/core/itscam_client.cpp",
  "src/core/itscam_rest_client.cpp",
  "src/core/itscam_cgi_client.cpp",
];

const CORE_C_API_DIR = join(REPO_ROOT, "src/core/c_api");

function walkDocs() {
  const docsDir = join(REPO_ROOT, "docs");
  const entries = [];
  for (const name of readdirSync(docsDir)) {
    const full = join(docsDir, name);
    const stat = statSync(full);
    if (stat.isDirectory()) {
      for (const file of readdirSync(full)) {
        if (file.endsWith(".md")) {
          entries.push(relative(REPO_ROOT, join(full, file)));
        }
      }
    } else if (
      name.endsWith(".md") &&
      name !== "README.md" &&
      name !== "README.en-US.md"
    ) {
      entries.push(relative(REPO_ROOT, full));
    }
  }
  return entries;
}

/** Doxygen HTML under docs-site/content/public/api-ref/cpp/ (from `make docs-api-cpp`). */
function walkApiRefCpp() {
  if (!existsSync(CPP_API_REF_DIR)) {
    return [];
  }

  /** @param {string} dir */
  function walk(dir) {
    /** @type {string[]} */
    const found = [];
    for (const name of readdirSync(dir)) {
      const full = join(dir, name);
      const stat = statSync(full);
      if (stat.isDirectory()) {
        found.push(...walk(full));
      } else if (name.endsWith(".html")) {
        found.push(relative(REPO_ROOT, full));
      }
    }
    return found;
  }

  return walk(CPP_API_REF_DIR);
}

/** Public headers and sources under src/core/c_api/ (excludes c_api/impl/). */
function walkCoreCApi() {
  if (!existsSync(CORE_C_API_DIR)) {
    return [];
  }

  /** @param {string} dir */
  function walk(dir) {
    /** @type {string[]} */
    const found = [];
    for (const name of readdirSync(dir)) {
      const full = join(dir, name);
      const rel = relative(REPO_ROOT, full).replace(/\\/g, "/");
      if (rel.includes("/impl/")) continue;

      const stat = statSync(full);
      if (stat.isDirectory()) {
        found.push(...walk(full));
      } else if (/\.(h|hpp|cpp)$/.test(name)) {
        found.push(rel);
      }
    }
    return found;
  }

  return walk(CORE_C_API_DIR);
}

function collectCoreApiSourcePaths() {
  const paths = [...CORE_API_HEADERS, ...CORE_API_SOURCES, ...walkCoreCApi()];
  return paths.filter((repoPath) => existsSync(resolveRepoPath(repoPath)));
}

/** @param {string} repoPath */
export function inferMetadata(repoPath) {
  const normalized = repoPath.replace(/\\/g, "/");
  /** @type {Record<string, string>} */
  const meta = {
    type: "guide",
    client: "general",
    language: "general",
    source: normalized,
  };

  if (normalized === "AGENTS.md") {
    meta.type = "reference";
  } else if (normalized === "README.md") {
    meta.type = "reference";
  } else if (normalized.includes("/examples/")) {
    meta.type = "example";
  } else if (normalized.startsWith("docs/tutorials/")) {
    meta.type = "tutorial";
  } else if (normalized.includes("/api-ref/cpp/") && normalized.endsWith(".html")) {
    meta.type = "reference";
    meta.language = "cpp";
  } else if (normalized.startsWith("src/core/")) {
    meta.type = "reference";
    meta.language = "cpp";
  }

  const ext = extname(normalized);
  if (ext === ".py") meta.language = "python";
  else if (ext === ".go") meta.language = "go";
  else if (ext === ".cs") meta.language = "csharp";
  else if (ext === ".cpp") meta.language = "cpp";
  else if (ext === ".md" && normalized.startsWith("docs/")) meta.language = "markdown";

  const lower = normalized.toLowerCase();
  if (lower.includes("rest")) meta.client = "rest";
  else if (lower.includes("cgi") || lower.includes("mjpeg") || lower.includes("snapshot")) {
    meta.client = "cgi";
  } else if (
    lower.includes("capture") ||
    lower.includes("trigger") ||
    lower.includes("sdk_example") ||
    lower.includes("recorder")
  ) {
    meta.client = "binary";
  }

  if (normalized.startsWith("docs/api/binary")) meta.client = "binary";
  if (normalized.startsWith("docs/api/rest")) meta.client = "rest";
  if (normalized.startsWith("docs/api/cgi")) meta.client = "cgi";

  if (normalized.includes("/api-ref/cpp/")) {
    if (lower.includes("itscamrestclient")) meta.client = "rest";
    else if (lower.includes("itscamcgiclient")) meta.client = "cgi";
    else if (lower.includes("itscamclient")) meta.client = "binary";
  }

  if (normalized.startsWith("src/core/")) {
    if (lower.includes("rest")) meta.client = "rest";
    else if (lower.includes("cgi")) meta.client = "cgi";
    else if (lower.includes("itscam_client") || lower.includes("itscam_sdk")) {
      meta.client = "binary";
    }
  }

  if (normalized.startsWith("docs/wrappers/cpp")) meta.language = "cpp";
  else if (normalized.startsWith("docs/wrappers/python")) meta.language = "python";
  else if (normalized.startsWith("docs/wrappers/go")) meta.language = "go";
  else if (normalized.startsWith("docs/wrappers/csharp")) meta.language = "csharp";

  if (normalized.startsWith("docs/tutorials/first-image-")) {
    meta.client = "cgi";
    if (normalized.includes("first-image-cpp")) meta.language = "cpp";
    else if (normalized.includes("first-image-python")) meta.language = "python";
    else if (normalized.includes("first-image-go")) meta.language = "go";
    else if (normalized.includes("first-image-csharp")) meta.language = "csharp";
  }

  return meta;
}

/** AI Search item key (virtual folder layout for built-in folder metadata). */
export function toItemKey(repoPath) {
  return repoPath.replace(/\\/g, "/");
}

export function collectCorpusPaths() {
  const paths = new Set([
    ...STATIC_ENTRIES,
    ...walkDocs(),
    ...EXAMPLE_GLOBS,
    ...walkApiRefCpp(),
    ...collectCoreApiSourcePaths(),
  ]);
  return [...paths].sort();
}

export function resolveRepoPath(repoPath) {
  return join(REPO_ROOT, repoPath);
}

export { REPO_ROOT };
