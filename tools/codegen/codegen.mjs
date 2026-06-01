#!/usr/bin/env node
// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// codegen.mjs -- generate typed REST helpers for the ITSCAM SDK in
// C++/C#/Python/Go from an OpenAPI 3.0 spec.
//
// Pipeline:
//   1. Run postprocess.mjs on the input spec (default: spec/default.yaml).
//   2. Feed the post-processed YAML to quicktype as an in-memory JSON Schema.
//   3. Emit one file per target language under src/.
//
// The set of Phase 1 top-level schemas is hard-coded here.  Add more entries
// to PHASE1_SCHEMAS as the SDK takes on additional typed configs.
//
// Usage:
//   node codegen.mjs [--spec <path>] [--out-dir <path>] [--check]
//
// SPEC defaults to spec/default.yaml.  OUT_DIR defaults to the SDK source
// tree (relative to repo root, computed from this file's location).  --check
// runs codegen into a temp directory and exits non-zero if outputs differ
// from what is on disk.

import fs from "node:fs/promises"
import os from "node:os"
import path from "node:path"
import url from "node:url"
import { spawn } from "node:child_process"
import YAML from "yaml"
import {
  quicktype,
  JSONSchemaInput,
  InputData,
  JSONSchemaStore,
} from "quicktype-core"

const __dirname = path.dirname(url.fileURLToPath(import.meta.url))
const REPO_ROOT = path.resolve(__dirname, "..", "..")
const TOOL_DIR = __dirname
const DEFAULT_SPEC = path.join(TOOL_DIR, "spec", "default.yaml")

// Top-level schemas we emit typed POCOs for.  Quicktype follows $refs
// so transitive dependencies (RoiWithEnable, RGBColorF, MinMaxFixedValue,
// sub-types of ProfileConfig, etc.) come along automatically.
//
// Phase 1: image profiles + the most common equipment configs.
// Phase 2: remaining equipment configs (FTP, Lince, vehicle indicator,
// protocols, profile transitioner, lanes, IO, REST API client, licenses,
// image sign).
const PHASE1_SCHEMAS = [
  "ProfileConfig",
  "OcrConfig",
  "AnalyticsConfig",
  "ClassifierConfig",
  "AutoFocus",
  "StreamConfig",
  "Misc",
  "MiscVolatile",
  "ItscamproConfig",
  "ItscamproStatus",
  // Phase 2 ----------------------------------------------------------------
  "ImageSignConfig",
  "FtpConfig",
  "LinceConfig",
  "LinceStatus",
  "VehicleIndicatorConfig",
  "ProtocolsConfig",
  "ProfileTransitioner",
  "LanesConfig",
  "IoConfig",
  "IoBasic",
  "RestApiClientConfig",
  "RestApiClientStatus",
  "Licenses",
]

const NOTICE_HEAD = [
  "// SPDX-License-Identifier: Proprietary",
  "// Copyright (c) 2026 Pumatronix",
  "//",
  "// AUTO-GENERATED FILE -- DO NOT EDIT.",
  "// Regenerate with `make codegen` (see tools/codegen/README.md).",
  "//",
  "// Generated from an OpenAPI 3.0 snapshot of the ITSCAM camera webapp.",
  "// Edit tools/codegen/codegen.mjs and rerun, do not patch this output.",
  "",
]
const NOTICE_HEAD_PY = [
  "# SPDX-License-Identifier: Proprietary",
  "# Copyright (c) 2026 Pumatronix",
  "#",
  "# AUTO-GENERATED FILE -- DO NOT EDIT.",
  "# Regenerate with `make codegen` (see tools/codegen/README.md).",
  "#",
  "# Generated from an OpenAPI 3.0 snapshot of the ITSCAM camera webapp.",
  "# Edit tools/codegen/codegen.mjs and rerun, do not patch this output.",
  "",
]

const TARGETS = [
  {
    name: "C++",
    lang: "c++",
    rendererOptions: {
      namespace: "itscam::rest_types",
      "source-style": "single-source",
      "include-location": "global-include",
      "code-format": "with-struct",
      "const-style": "east-const",
      wstring: "use-string",
      boost: "false",
    },
    outRel: "src/core/itscam_rest_types.h",
    notice: NOTICE_HEAD,
    postProcess: (text) => addCppPartialJson(fixCppOptionalInit(text)),
  },
  {
    name: "C#",
    lang: "csharp",
    rendererOptions: {
      namespace: "Pumatronix.Itscam.RestTypes",
      framework: "SystemTextJson",
      "array-type": "list",
      "csharp-version": "6",
      features: "complete",
      density: "normal",
    },
    outRel: "src/wrappers/csharp/Itscam.Sdk/RestTypes/RestTypes.g.cs",
    notice: NOTICE_HEAD,
    // The complete features pull in DateOnly / TimeOnly JsonConverter helpers
    // that target net6+.  None of our schemas have `format: date` or
    // `format: time`, so the converters are dead code.  Strip them so the
    // assembly stays compatible with netstandard2.0.
    postProcess: (text) => stripCSharpNet6Converters(fixCSharpNullableGetString(text)),
  },
  {
    name: "Python",
    lang: "python",
    rendererOptions: {
      "python-version": "3.7",
      "nice-property-names": "true",
    },
    outRel: "src/wrappers/python/itscam/rest_types.py",
    notice: NOTICE_HEAD_PY,
  },
  {
    name: "Go",
    lang: "go",
    rendererOptions: {
      package: "itscam",
      "omit-empty": "true",
    },
    outRel: "src/wrappers/go/itscam/rest_types.go",
    notice: NOTICE_HEAD,
  },
]

function info(message) {
  console.log(`[codegen] ${message}`)
}

/// Replace `std::optional<T>()` with `std::nullopt` to avoid GCC's
/// -Wmaybe-uninitialized false positive on template instantiation in
/// from_json array helpers.
function fixCppOptionalInit(text) {
  return text.replace(/return std::optional<T>\(\);/g, "return std::nullopt;")
}

/// Generate `to_partial_json()` free functions for every struct that has a
/// `to_json()`.  The partial variant omits keys whose `std::optional<T>`
/// member is `nullopt`, and recurses into nested structs via their own
/// `to_partial_json()`.  Required (non-optional) fields are always emitted.
///
/// This enables typed partial PUT: construct a struct with only the fields
/// you want to change, serialize with `to_partial_json(cfg)`, and the
/// daemon receives only those keys.
function addCppPartialJson(text) {
  // 1. Parse struct declarations → { StructName: { cppMember: { optional, innerType } } }
  const structFields = new Map()
  const structRe = /struct\s+(\w+)\s*\{([^}]*)\}/g
  let m
  while ((m = structRe.exec(text)) !== null) {
    const name = m[1]
    const body = m[2]
    const fields = new Map()
    const fieldRe = /^\s*(?:std::optional<(.+?)>|(.+?))\s+(\w+)\s*;/gm
    let fm
    while ((fm = fieldRe.exec(body)) !== null) {
      const optInner = fm[1] // e.g. "bool", "Advanced", "std::vector<Power>"
      const plainType = fm[2] // e.g. "int64_t", "bool", "std::string"
      const member = fm[3]
      fields.set(member, {
        optional: !!optInner,
        innerType: optInner || plainType,
      })
    }
    structFields.set(name, fields)
  }

  // 2. Collect the set of struct names that have to_json (i.e. non-enum types)
  const structsWithToJson = new Set()
  const toJsonSigRe = /inline void to_json\(json & j, (\w+) const & x\) \{/g
  while ((m = toJsonSigRe.exec(text)) !== null) {
    if (structFields.has(m[1])) structsWithToJson.add(m[1])
  }

  // 3. Parse each struct-type to_json() body and build to_partial_json()
  //    Match: inline void to_json(json & j, TypeName const & x) { ... }
  const toJsonBlockRe =
    /inline void to_json\(json & j, (\w+) const & x\) \{\s*\n\s*j = json::object\(\);\n([\s\S]*?)\n\s*\}/g

  const partialFns = []
  const partialDecls = []

  while ((m = toJsonBlockRe.exec(text)) !== null) {
    const typeName = m[1]
    const assignBlock = m[2]
    const fields = structFields.get(typeName)
    if (!fields) continue // enum to_json — skip

    const lines = []
    // Parse assignments: j["jsonKey"] = x.cppMember;
    const assignRe = /j\["(\w+)"\]\s*=\s*x\.(\w+);/g
    let am
    while ((am = assignRe.exec(assignBlock)) !== null) {
      const jsonKey = am[1]
      const cppMember = am[2]
      const fi = fields.get(cppMember)
      if (!fi) {
        lines.push(`        j["${jsonKey}"] = x.${cppMember};`)
        continue
      }
      if (!fi.optional) {
        // Required field — always write.
        if (structsWithToJson.has(fi.innerType)) {
          lines.push(`        j["${jsonKey}"] = to_partial_json(x.${cppMember});`)
        } else {
          lines.push(`        j["${jsonKey}"] = x.${cppMember};`)
        }
      } else {
        // Optional field — skip when nullopt; recurse for struct types.
        const inner = fi.innerType
        // Check if innerType is a known struct (possibly wrapped in vector).
        const vecMatch = inner.match(/^std::vector<(\w+)>$/)
        const elementType = vecMatch ? vecMatch[1] : inner
        const isStructType = structsWithToJson.has(elementType)

        if (vecMatch && isStructType) {
          // std::optional<std::vector<StructType>>
          lines.push(`        if (x.${cppMember}) {`)
          lines.push(`            json arr = json::array();`)
          lines.push(`            for (auto const & e : *x.${cppMember}) arr.push_back(to_partial_json(e));`)
          lines.push(`            j["${jsonKey}"] = std::move(arr);`)
          lines.push(`        }`)
        } else if (isStructType) {
          // std::optional<StructType>
          lines.push(`        if (x.${cppMember}) j["${jsonKey}"] = to_partial_json(*x.${cppMember});`)
        } else {
          // std::optional<scalar|string|enum|vector<scalar>>
          lines.push(`        if (x.${cppMember}) j["${jsonKey}"] = *x.${cppMember};`)
        }
      }
    }

    partialFns.push(
      `    inline json to_partial_json(${typeName} const & x) {\n` +
      `        json j = json::object();\n` +
      lines.join("\n") + "\n" +
      `        return j;\n` +
      `    }`
    )
    partialDecls.push(`    json to_partial_json(${typeName} const & x);`)
  }

  if (partialFns.length === 0) return text

  // 4. Insert forward declarations after the to_json forward declarations.
  //    Find the last `void to_json(...)` forward-decl line and append after
  //    its subsequent blank line.
  const partialDeclBlock =
    "\n    // Partial-JSON serializers (omit nullopt fields for typed partial PUT)\n" +
    partialDecls.join("\n") + "\n"

  // Find the boundary: last forward-decl to_json line before the inline impls
  const lastFwdRe = /( {4}void to_json\(json & j, \w+ const & x\);\n)\n( {4}inline void from_json)/
  text = text.replace(lastFwdRe, "$1" + partialDeclBlock + "\n$2")

  // 5. Append partial-json implementations before the closing `}\n}`
  const implBlock =
    "\n    // ---- Partial-JSON serializers (generated) ----\n\n" +
    partialFns.join("\n\n") + "\n"

  const closingNs = /\n\}\n\}\n$/
  text = text.replace(closingNs, implBlock + "\n}\n}\n")

  return text
}

/// Fix CS8602 (possible null dereference) in quicktype-emitted converters:
/// `reader.GetString()` returns `string?`; add null-forgiving `!`.
function fixCSharpNullableGetString(text) {
  return text.replace(
    /var value = reader\.GetString\(\);/g,
    "var value = reader.GetString()!;"
  )
}

/// Drop the `DateOnlyConverter` and `TimeOnlyConverter` classes that quicktype
/// emits unconditionally for C#.  They reference net6+ types but are never
/// instantiated by our schemas (no `format: date` / `format: time`).
function stripCSharpNet6Converters(text) {
  // Each converter is a `public class ... { ... }` block.  Match
  // conservatively: from `public class DateOnlyConverter` (resp.
  // TimeOnlyConverter) up to and including the matching closing brace at
  // indentation level 4 (the namespace's class scope), and the trailing
  // blank line.
  const stripBlock = (src, name) => {
    const startMarker = `    public class ${name}Converter : JsonConverter<${name}>`
    const start = src.indexOf(startMarker)
    if (start < 0) return src
    // Find matching closing brace.
    let depth = 0
    let i = src.indexOf("{", start)
    if (i < 0) return src
    for (; i < src.length; ++i) {
      const ch = src[i]
      if (ch === "{") ++depth
      else if (ch === "}") {
        --depth
        if (depth === 0) {
          // Skip any trailing newline + blank line.
          let end = i + 1
          while (end < src.length && src[end] === "\n") ++end
          return src.slice(0, start) + src.slice(end)
        }
      }
    }
    return src
  }
  let out = text
  out = stripBlock(out, "DateOnly")
  out = stripBlock(out, "TimeOnly")
  // Also drop the `new DateOnlyConverter()` / `new TimeOnlyConverter()` lines
  // that quicktype inserts into the shared Converter.Settings list.
  out = out.replace(
    /\n?\s*new\s+(?:Date|Time)OnlyConverter\(\)\s*,?\n/g,
    "\n"
  )
  return out
}

function die(message) {
  console.error(`[codegen] ERROR: ${message}`)
  process.exit(1)
}

function parseArgs() {
  const args = {
    spec: process.env.SPEC || DEFAULT_SPEC,
    outDir: process.env.OUT_DIR || REPO_ROOT,
    check: false,
  }
  const argv = process.argv.slice(2)
  for (let i = 0; i < argv.length; ++i) {
    const arg = argv[i]
    if ((arg === "--spec" || arg === "-s") && argv[i + 1]) {
      args.spec = argv[++i]
    } else if ((arg === "--out-dir" || arg === "-o") && argv[i + 1]) {
      args.outDir = argv[++i]
    } else if (arg === "--check") {
      args.check = true
    } else if (arg === "--help" || arg === "-h") {
      console.log(
        "Usage: codegen.mjs [--spec <path>] [--out-dir <path>] [--check]"
      )
      process.exit(0)
    } else {
      die(`Unrecognised argument: ${arg}`)
    }
  }
  return args
}

function runPostprocess(input, output) {
  return new Promise((resolve, reject) => {
    const child = spawn(
      process.execPath,
      [path.join(TOOL_DIR, "postprocess.mjs"), "--input", input, "--output", output],
      { stdio: "inherit" }
    )
    child.on("error", reject)
    child.on("close", (code) => {
      if (code === 0) resolve()
      else reject(new Error(`postprocess.mjs exited with code ${code}`))
    })
  })
}

class InMemoryStore extends JSONSchemaStore {
  constructor(schema) {
    super()
    this.schema = schema
  }
  async fetch(_address) {
    return this.schema
  }
}

async function generateFor(target, doc) {
  const store = new InMemoryStore(doc)
  const input = new JSONSchemaInput(store)
  for (const name of PHASE1_SCHEMAS) {
    await input.addSource({
      name,
      uris: [`itscam.json#/components/schemas/${name}`],
    })
  }
  const inputData = new InputData()
  inputData.addInput(input)
  const result = await quicktype({
    inputData,
    lang: target.lang,
    rendererOptions: target.rendererOptions,
  })
  let body = result.lines.join("\n")
  if (typeof target.postProcess === "function") {
    body = target.postProcess(body)
  }
  return target.notice.join("\n") + body + (body.endsWith("\n") ? "" : "\n")
}

async function checkOrWrite(target, generated, outDir, check) {
  const dest = path.resolve(outDir, target.outRel)
  await fs.mkdir(path.dirname(dest), { recursive: true })
  if (check) {
    let onDisk = ""
    try {
      onDisk = await fs.readFile(dest, "utf8")
    } catch (e) {
      info(`MISSING (check failed): ${target.outRel}`)
      return false
    }
    if (onDisk !== generated) {
      info(`DRIFT (check failed): ${target.outRel}`)
      return false
    }
    info(`OK ${target.outRel}`)
    return true
  }
  await fs.writeFile(dest, generated)
  info(`Wrote ${target.outRel}`)
  return true
}

async function main() {
  const args = parseArgs()
  info(`Using spec: ${args.spec}`)
  info(`Output dir: ${args.outDir}${args.check ? " (--check)" : ""}`)

  const buildDir = path.join(TOOL_DIR, "build")
  await fs.mkdir(buildDir, { recursive: true })
  const processed = path.join(buildDir, "itscam.postprocessed.yaml")
  await runPostprocess(args.spec, processed)

  const doc = YAML.parse(await fs.readFile(processed, "utf8"))

  let ok = true
  for (const target of TARGETS) {
    info(`Generating ${target.name} (${PHASE1_SCHEMAS.length} top-level type(s))...`)
    const generated = await generateFor(target, doc)
    const result = await checkOrWrite(target, generated, args.outDir, args.check)
    if (!result) ok = false
  }

  if (args.check) {
    if (ok) {
      info("All generated outputs match committed snapshot.")
      process.exit(0)
    } else {
      info("Generated outputs DIFFER from the committed snapshot.")
      info("Run `make codegen` and commit the updated files.")
      process.exit(1)
    }
  } else {
    info("Done.")
  }
}

main().catch((e) => {
  console.error(e)
  process.exit(1)
})
