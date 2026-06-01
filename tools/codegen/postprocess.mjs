#!/usr/bin/env node
// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// postprocess.mjs -- normalise a raw itscam.yaml (the OpenAPI document
// produced by the itscam600 webapp backend's `npm run docs`) into a clean,
// self-contained spec suitable for code generation.
//
// Tasks:
//   1. Rewrite any $ref pointing at daemon files (api_common.yaml#/..., etc.)
//      to local definitions inlined from spec/daemon-fixtures/.
//   2. Inject LanesConfig / LaneRegion schemas if the input does not define
//      them (the webapp documents the endpoint but not the schema).
//   3. Auto-fix known upstream JSDoc-induced glitches (e.g. `type: int` typos)
//      so the spec parses cleanly.
//   4. Resolve $refs with swagger-parser; abort with a precise error on
//      anything unresolvable.  Full OpenAPI 3.0 schema-level validation is
//      opt-in via --strict because the upstream JSDoc occasionally produces
//      shapes that violate the spec without actually being unusable for
//      code generation.
//
// The post-processed spec is a faithful, self-contained copy of the input
// plus the injected/inlined fixtures.  Decisions about which schemas the SDK
// actually exposes as typed wrappers happen later, in codegen.mjs.
//
// Usage:
//   node postprocess.mjs --input <spec.yaml> --output <out.yaml>
//
// The tool is intentionally permissive about its input -- end users can hand
// it any well-formed OpenAPI 3.0 document; downstream firmware that adds the
// missing schemas natively triggers no-ops on the relevant steps.

import fs from "node:fs/promises"
import path from "node:path"
import url from "node:url"
import YAML from "yaml"
import SwaggerParser from "@apidevtools/swagger-parser"

const __dirname = path.dirname(url.fileURLToPath(import.meta.url))
const FIXTURES_DIR = path.join(__dirname, "spec", "daemon-fixtures")

function die(message) {
  console.error(`[postprocess] ERROR: ${message}`)
  process.exit(1)
}

function info(message) {
  console.log(`[postprocess] ${message}`)
}

function parseArgs() {
  const args = { input: null, output: null, strict: false }
  const argv = process.argv.slice(2)
  for (let i = 0; i < argv.length; ++i) {
    const arg = argv[i]
    if ((arg === "--input" || arg === "-i") && argv[i + 1]) {
      args.input = argv[++i]
    } else if ((arg === "--output" || arg === "-o") && argv[i + 1]) {
      args.output = argv[++i]
    } else if (arg === "--strict") {
      args.strict = true
    } else if (arg === "--help" || arg === "-h") {
      console.log(
        "Usage: postprocess.mjs --input <spec.yaml> --output <out.yaml> [--strict]"
      )
      process.exit(0)
    } else {
      die(`Unrecognised argument: ${arg}`)
    }
  }
  if (!args.input || !args.output) {
    die("--input and --output are required (see --help)")
  }
  return args
}

async function readYaml(filePath) {
  const buf = await fs.readFile(filePath, "utf8")
  try {
    return YAML.parse(buf)
  } catch (e) {
    die(`Failed to parse ${filePath}: ${e.message}`)
  }
}

async function loadFixture(name) {
  const filePath = path.join(FIXTURES_DIR, name)
  return readYaml(filePath)
}

// Recursively walk an object and apply transform() to every $ref string.
function rewriteRefs(node, transform) {
  if (Array.isArray(node)) {
    node.forEach((item) => rewriteRefs(item, transform))
  } else if (node && typeof node === "object") {
    for (const key of Object.keys(node)) {
      if (key === "$ref" && typeof node[key] === "string") {
        node[key] = transform(node[key])
      } else {
        rewriteRefs(node[key], transform)
      }
    }
  }
}

// Inline schemas referenced by external paths (api_common.yaml#/..., etc.)
async function inlineDaemonRefs(doc) {
  doc.components = doc.components ?? {}
  doc.components.schemas = doc.components.schemas ?? {}
  const schemas = doc.components.schemas

  // Index every fixture file we ship so external refs can be resolved.
  const fixtureFiles = await fs.readdir(FIXTURES_DIR)
  const fixtureIndex = {}
  for (const f of fixtureFiles) {
    if (!f.endsWith(".yaml") && !f.endsWith(".yml")) continue
    fixtureIndex[f] = await loadFixture(f)
  }

  // Collect refs we need to inline + the local target name we'll use.
  const toInline = new Map() // localName -> { file, pointer, schemaObj }

  const externalRefRe = /^([\w\.\-]+\.ya?ml)#\/components\/schemas\/([\w]+)$/

  rewriteRefs(doc, (ref) => {
    const m = ref.match(externalRefRe)
    if (!m) return ref
    const [, file, name] = m
    if (schemas[name]) {
      // Already present locally; just point to the local copy.
      return `#/components/schemas/${name}`
    }
    const fixture = fixtureIndex[file]
    if (!fixture) {
      die(`Reference '${ref}' points at unknown fixture file '${file}'`)
    }
    const schemaObj = fixture.components?.schemas?.[name]
    if (!schemaObj) {
      die(
        `Reference '${ref}' has no matching schema '${name}' in fixture '${file}'`
      )
    }
    toInline.set(name, schemaObj)
    return `#/components/schemas/${name}`
  })

  if (toInline.size > 0) {
    info(`Inlining ${toInline.size} daemon schema(s): ${[...toInline.keys()].join(", ")}`)
    for (const [name, schemaObj] of toInline) {
      schemas[name] = schemaObj
    }
  } else {
    info("No external daemon $refs found (nothing to inline).")
  }

  return doc
}

// Pull in LanesConfig / LaneRegion from the daemon fixture if the webapp
// spec doesn't carry them.  Future firmware that adds them upstream makes
// this a no-op.
async function injectLanesConfigIfMissing(doc) {
  const schemas = doc.components?.schemas ?? {}
  const wantedNames = ["LanesConfig", "LaneRegion"]
  const missing = wantedNames.filter((n) => !schemas[n])
  if (missing.length === 0) {
    info("LanesConfig / LaneRegion already defined; skipping injection.")
    return doc
  }
  const fixture = await loadFixture("api_service_lanes.yaml")
  for (const name of missing) {
    const schemaObj = fixture.components?.schemas?.[name]
    if (!schemaObj) {
      die(`Fixture api_service_lanes.yaml is missing required schema '${name}'`)
    }
    schemas[name] = schemaObj
    info(`Injected schema '${name}' from daemon fixture.`)
  }
  doc.components.schemas = schemas
  return doc
}

// Walk every type-bearing node and rewrite well-known typos.
function autoFixTypoes(doc) {
  let fixes = 0
  const fixed = (node) => {
    if (Array.isArray(node)) {
      node.forEach(fixed)
      return
    }
    if (!node || typeof node !== "object") return
    if (node.type === "int") {
      node.type = "integer"
      node.format = node.format ?? "int32"
      ++fixes
    }
    for (const key of Object.keys(node)) {
      fixed(node[key])
    }
  }
  fixed(doc)
  if (fixes > 0) {
    info(`Auto-fixed ${fixes} 'type: int' typo(s) to 'type: integer'.`)
  }
  return doc
}

// Apply targeted schema patches for known upstream JSDoc bugs that even the
// camera disagrees with at runtime.  Each patch is **idempotent**: if the
// upstream spec is fixed in a future firmware release the patch becomes a
// no-op (we still log a notice so anyone watching the postprocess output
// knows the workaround is in effect).
function applyUpstreamPatches(doc) {
  const schemas = doc.components?.schemas ?? {}

  // Patch 1: ExposureConfig.level
  //
  // Upstream JSDoc declares `level` as an inline object with only
  // `{ targetValue: integer, roi }`, but the camera actually serialises it
  // as a full ControlAlgorithmConfig (mode/updateRate/targetValue:float/
  // holdTime) plus `roi`.  Force the on-the-wire shape so the typed
  // wrappers can deserialise real camera responses.
  const exposure = schemas.ExposureConfig
  const cac = schemas.ControlAlgorithmConfig
  if (exposure?.properties?.level && cac?.properties) {
    const level = exposure.properties.level
    const tv = level.properties?.targetValue
    const needsFix =
      (tv && tv.type === "integer") || !level.properties?.mode
    if (needsFix) {
      level.type = "object"
      level.properties = {
        ...cac.properties,
        // Preserve roi (a ControlAlgorithmConfig extension specific to
        // ExposureConfig.level).
        roi: level.properties?.roi ?? {
          $ref: "#/components/schemas/RoiWithEnable",
        },
      }
      info(
        "Patched ExposureConfig.level to match camera shape "
          + "(ControlAlgorithmConfig + roi)."
      )
    }
  }

  // Patch 2: TransitionStepConfig.level
  //
  // Upstream JSDoc declares `level` as `integer/int32`, but the camera
  // serialises it as a float (e.g. a brightness threshold like 32.5).
  // Widen the type to `number/float` so JSON deserialisers accept both
  // integer and floating-point payloads.
  const tsc = schemas.TransitionStepConfig
  if (tsc?.properties?.level && tsc.properties.level.type === "integer") {
    tsc.properties.level = { type: "number", format: "float" }
    info(
      "Patched TransitionStepConfig.level: integer/int32 -> number/float "
        + "(camera serialises this as a float threshold)."
    )
  }

  return doc
}

// Fix quicktype naming of MultipleExposuresConfig.
//
// ProfileConfig.multipleExposures is an inline object in the upstream spec.
// quicktype derives the type name "MultipleExposures" from the property name,
// then when it encounters the $ref'd MultipleExposuresConfig schema it detects
// a near-collision and falls back to the placeholder name "Something".
//
// Fix: extract the inline property to a named schema ("MultipleExposures") so
// quicktype gets both names from explicit schema names.  Also set `title` on
// MultipleExposuresConfig as a belt-and-suspenders measure.
//
// Idempotent: if a future firmware defines MultipleExposures natively and uses
// a $ref in ProfileConfig, both steps become no-ops.
function fixMultipleExposuresNaming(doc) {
  const schemas = doc.components?.schemas ?? {}
  const profile = schemas.ProfileConfig

  // Step 1: extract inline multipleExposures → named schema
  if (
    profile?.properties?.multipleExposures &&
    !profile.properties.multipleExposures.$ref
  ) {
    const inline = profile.properties.multipleExposures
    if (!schemas.MultipleExposures) {
      schemas.MultipleExposures = inline
      info(
        "Extracted inline ProfileConfig.multipleExposures → named schema 'MultipleExposures'."
      )
    }
    profile.properties.multipleExposures = {
      $ref: "#/components/schemas/MultipleExposures",
    }
  }

  // Step 2: ensure MultipleExposuresConfig carries a title so quicktype
  // preserves the name even as a transitive dependency.
  if (schemas.MultipleExposuresConfig && !schemas.MultipleExposuresConfig.title) {
    schemas.MultipleExposuresConfig.title = "MultipleExposuresConfig"
    info("Added title to MultipleExposuresConfig schema for stable codegen naming.")
  }

  doc.components.schemas = schemas
  return doc
}

async function validate(doc, { strict }) {
  // SwaggerParser mutates its input when dereferencing; pass a deep clone so
  // the document we write to disk keeps its $refs intact.
  const clone = JSON.parse(JSON.stringify(doc))
  try {
    if (strict) {
      await SwaggerParser.validate(clone)
      info("OpenAPI strict validation OK.")
    } else {
      await SwaggerParser.parse(clone)
      info("OpenAPI $ref resolution OK (run with --strict for full validation).")
    }
  } catch (e) {
    die(`OpenAPI validation failed: ${e.message}`)
  }
}

async function main() {
  const { input, output, strict } = parseArgs()
  info(`Loading ${input}`)
  const doc = await readYaml(input)

  if (!doc || doc.openapi == null) {
    die(`Input ${input} is not a valid OpenAPI 3 document (no 'openapi' field)`)
  }
  info(`OpenAPI ${doc.openapi}  info.version=${doc.info?.version ?? "?"}`)

  await inlineDaemonRefs(doc)
  await injectLanesConfigIfMissing(doc)
  autoFixTypoes(doc)
  applyUpstreamPatches(doc)
  fixMultipleExposuresNaming(doc)
  await validate(doc, { strict })

  await fs.mkdir(path.dirname(output), { recursive: true })
  await fs.writeFile(output, YAML.stringify(doc, { lineWidth: 0 }))
  info(`Wrote ${output}`)
}

main().catch((e) => {
  console.error(e)
  process.exit(1)
})
