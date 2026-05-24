#!/usr/bin/env node
/**
 * Upload the SDK documentation corpus to a Cloudflare AI Search instance.
 *
 * Required environment variables:
 *   CLOUDFLARE_ACCOUNT_ID
 *   CLOUDFLARE_API_TOKEN   (AI Search:Edit + AI Search:Run)
 *   AI_SEARCH_INSTANCE_ID
 *
 * Optional:
 *   AI_SEARCH_NAMESPACE  (default: "default")
 */
import { readFileSync } from "node:fs";
import {
  collectCorpusPaths,
  inferMetadata,
  resolveRepoPath,
  toItemKey,
} from "./corpus-manifest.mjs";

const accountId = process.env.CLOUDFLARE_ACCOUNT_ID;
const apiToken = process.env.CLOUDFLARE_API_TOKEN;
const instanceId = process.env.AI_SEARCH_INSTANCE_ID;
const namespace = process.env.AI_SEARCH_NAMESPACE ?? "default";

if (!accountId || !apiToken || !instanceId) {
  console.log(
    "Skipping AI Search sync: set CLOUDFLARE_ACCOUNT_ID, CLOUDFLARE_API_TOKEN, and AI_SEARCH_INSTANCE_ID.",
  );
  process.exit(0);
}

const baseUrl =
  namespace === "default"
    ? `https://api.cloudflare.com/client/v4/accounts/${accountId}/ai-search/instances/${instanceId}`
    : `https://api.cloudflare.com/client/v4/accounts/${accountId}/ai-search/namespaces/${namespace}/instances/${instanceId}`;

async function api(path, options = {}) {
  const response = await fetch(`${baseUrl}${path}`, {
    ...options,
    headers: {
      Authorization: `Bearer ${apiToken}`,
      ...(options.headers ?? {}),
    },
  });
  const body = await response.json();
  if (!response.ok || body.success === false) {
    throw new Error(
      `AI Search API ${path} failed (${response.status}): ${JSON.stringify(body)}`,
    );
  }
  return body;
}

async function uploadItem(repoPath) {
  const key = toItemKey(repoPath);
  const filePath = resolveRepoPath(repoPath);
  const content = readFileSync(filePath);
  const metadata = inferMetadata(repoPath);

  const form = new FormData();
  form.append("file", new Blob([content]), key.split("/").pop());
  form.append("key", key);
  form.append("metadata", JSON.stringify(metadata));

  await api("/items", { method: "POST", body: form });
  console.log(`  uploaded ${key}`);
}

async function main() {
  const paths = collectCorpusPaths();
  console.log(`Uploading ${paths.length} files to AI Search instance "${instanceId}"...`);

  for (const repoPath of paths) {
    await uploadItem(repoPath);
  }

  console.log("AI Search sync complete.");
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
