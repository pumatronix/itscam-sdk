/**
 * Canonical list of files indexed by Cloudflare AI Search and the doc site.
 * Keys use forward-slash paths relative to the repository root.
 */
import { readdirSync, statSync } from "node:fs";
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
  ]);
  return [...paths].sort();
}

export function resolveRepoPath(repoPath) {
  return join(REPO_ROOT, repoPath);
}

export { REPO_ROOT };
