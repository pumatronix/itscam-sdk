#!/usr/bin/env node
/**
 * Join soft-wrapped markdown prose into single lines per paragraph.
 * Preserves fenced code blocks, tables, headings, horizontal rules,
 * and block structure. List items and blockquote paragraphs are
 * unwrapped in place.
 */
import { readFileSync, writeFileSync } from "node:fs";

function isBlank(line) {
  return line.trim() === "";
}

function isFenceLine(line) {
  return /^(`{3,}|~{3,})/.test(line.trim());
}

function fenceMarker(line) {
  const m = line.trim().match(/^(`{3,}|~{3,})/);
  return m ? m[1] : null;
}

function isTableLine(line) {
  const t = line.trim();
  return t.startsWith("|") && t.endsWith("|");
}

function isTableSeparator(line) {
  return /^\|?[\s:-]+\|[\s|:-]*$/.test(line.trim());
}

function isHr(line) {
  return /^(\*{3,}|-{3,}|_{3,})\s*$/.test(line.trim());
}

function isHeading(line) {
  return /^#{1,6}\s/.test(line);
}

function isBlockquote(line) {
  return /^>\s?/.test(line);
}

function blockquoteBody(line) {
  return line.replace(/^>\s?/, "");
}

function listMarkerMatch(line) {
  return line.match(/^(\s*)([-*+]|\d+\.)\s+/);
}

function isListItem(line) {
  return listMarkerMatch(line) !== null;
}

function isListContinuation(line, baseIndent) {
  if (isBlank(line)) return false;
  if (isListItem(line)) return false;
  if (isHeading(line) || isFenceLine(line) || isTableLine(line) || isHr(line)) {
    return false;
  }
  const leading = line.match(/^(\s*)/)[1].length;
  return leading >= baseIndent + 1;
}

function joinParts(parts) {
  return parts
    .map((p) => p.trim())
    .filter(Boolean)
    .join(" ")
    .replace(/\s{2,}/g, " ")
    .trimEnd();
}

function processBlockquoteBlock(lines) {
  const out = [];
  let i = 0;

  while (i < lines.length) {
    const body = blockquoteBody(lines[i]);

    if (isFenceLine(body)) {
      const marker = fenceMarker(body);
      const char = marker[0];
      const len = marker.length;
      out.push(lines[i]);
      i++;
      while (i < lines.length) {
        if (!isBlockquote(lines[i])) break;
        out.push(lines[i]);
        const inner = blockquoteBody(lines[i]).trim();
        if (inner === char.repeat(len)) {
          i++;
          break;
        }
        i++;
      }
      continue;
    }

    if (body.trim() === "") {
      out.push(">");
      i++;
      continue;
    }

    if (isListItem(body)) {
      const match = listMarkerMatch(body);
      const indent = match[1];
      const marker = match[2];
      const baseIndent = indent.length;
      const afterMarker = body.slice(match[0].length);
      const parts = [afterMarker];
      i++;
      while (i < lines.length) {
        const nextBody = blockquoteBody(lines[i]);
        if (nextBody.trim() === "" || isListItem(nextBody)) break;
        if (!isListContinuation(nextBody, baseIndent)) break;
        parts.push(nextBody.trim());
        i++;
      }
      out.push(`> ${indent}${marker} ${joinParts(parts)}`);
      continue;
    }

    const parts = [body];
    i++;
    while (i < lines.length) {
      const nextBody = blockquoteBody(lines[i]);
      if (nextBody.trim() === "" || isListItem(nextBody)) break;
      parts.push(nextBody);
      i++;
    }
    out.push(`> ${joinParts(parts)}`);
  }

  return out;
}

function unwrapMarkdown(content) {
  const lines = content.split("\n");
  const out = [];
  let i = 0;

  while (i < lines.length) {
    const line = lines[i];

    if (isBlank(line)) {
      out.push("");
      i++;
      continue;
    }

    if (isFenceLine(line)) {
      const marker = fenceMarker(line);
      const char = marker[0];
      const len = marker.length;
      out.push(line);
      i++;
      while (i < lines.length) {
        out.push(lines[i]);
        const t = lines[i].trim();
        if (t === char.repeat(len) || (t.startsWith(char.repeat(len)) && t.length === len)) {
          i++;
          break;
        }
        i++;
      }
      continue;
    }

    if (isHr(line)) {
      out.push(line);
      i++;
      continue;
    }

    if (isHeading(line)) {
      out.push(line);
      i++;
      continue;
    }

    if (isTableLine(line) || isTableSeparator(line)) {
      while (i < lines.length && (isTableLine(lines[i]) || isTableSeparator(lines[i]))) {
        out.push(lines[i]);
        i++;
      }
      continue;
    }

    if (isBlockquote(line)) {
      const block = [];
      while (i < lines.length && isBlockquote(lines[i])) {
        block.push(lines[i]);
        i++;
      }
      out.push(...processBlockquoteBlock(block));
      continue;
    }

    if (isListItem(line)) {
      const match = listMarkerMatch(line);
      const indent = match[1];
      const marker = match[2];
      const baseIndent = indent.length;
      const afterMarker = line.slice(match[0].length);
      const parts = [afterMarker];
      i++;
      while (i < lines.length && isListContinuation(lines[i], baseIndent)) {
        parts.push(lines[i].trim());
        i++;
      }
      const isOrdered = /^\d+\.$/.test(marker);
      const prefix = isOrdered
        ? `${indent}${marker} `
        : `${indent}${marker} `;
      out.push(`${prefix}${joinParts(parts)}`);
      continue;
    }

    // Prose paragraph
    const parts = [line];
    i++;
    while (i < lines.length) {
      if (isBlank(lines[i])) break;
      const next = lines[i];
      if (
        isFenceLine(next) ||
        isHr(next) ||
        isHeading(next) ||
        isTableLine(next) ||
        isTableSeparator(next) ||
        isBlockquote(next) ||
        isListItem(next)
      ) {
        break;
      }
      parts.push(next);
      i++;
    }
    out.push(joinParts(parts));
  }

  // Trim trailing blank lines, keep single trailing newline.
  while (out.length > 0 && isBlank(out[out.length - 1])) {
    out.pop();
  }
  return `${out.join("\n")}\n`;
}

function main() {
  const files = process.argv.slice(2);
  if (files.length === 0) {
    console.error("Usage: unwrap-markdown.mjs <file.md>...");
    process.exit(1);
  }
  for (const file of files) {
    const before = readFileSync(file, "utf8");
    const after = unwrapMarkdown(before);
    if (before !== after) {
      writeFileSync(file, after);
      console.log(`unwrapped ${file}`);
    }
  }
}

main();
