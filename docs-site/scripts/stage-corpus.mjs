#!/usr/bin/env node
/**
 * Stage the SDK documentation corpus into a single directory so a single
 * `aws s3 sync ... --delete` upload mirrors the entire AI Search corpus
 * onto R2.
 *
 * The staged tree preserves repository-relative paths, e.g.:
 *
 *   .corpus-staging/
 *     README.md
 *     AGENTS.md
 *     docs/api/binary-client.md
 *     src/examples/itscam_sdk_example.cpp
 *     src/wrappers/python/examples/capture_example.py
 *
 * The list of files comes from `corpus-manifest.mjs`, which is also used by
 * `sync-ai-search.mjs`, so the R2 bucket and the (legacy) API-based
 * ingestion stay in lockstep.
 *
 * Optional environment variables:
 *   STAGING_DIR  Override the output directory (default: docs-site/.corpus-staging)
 */
import { cpSync, existsSync, mkdirSync, rmSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { collectCorpusPaths, resolveRepoPath } from "./corpus-manifest.mjs";

const SCRIPT_DIR = dirname(fileURLToPath(import.meta.url));
const DOCS_SITE = join(SCRIPT_DIR, "..");
const STAGING = process.env.STAGING_DIR
  ? process.env.STAGING_DIR
  : join(DOCS_SITE, ".corpus-staging");

function reset() {
  if (existsSync(STAGING)) {
    rmSync(STAGING, { recursive: true, force: true });
  }
  mkdirSync(STAGING, { recursive: true });
}

function ensureDir(filePath) {
  mkdirSync(dirname(filePath), { recursive: true });
}

reset();

const paths = collectCorpusPaths();
for (const repoPath of paths) {
  const src = resolveRepoPath(repoPath);
  const dest = join(STAGING, repoPath);
  ensureDir(dest);
  cpSync(src, dest);
}

console.log(`Staged ${paths.length} files into ${STAGING}`);
