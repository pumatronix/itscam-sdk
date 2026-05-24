# Codegen — typed REST helpers from OpenAPI

This directory ships an OpenAPI -> typed-helpers pipeline for the ITSCAM SDK.

```
tools/codegen/
├── spec/
│   ├── itscam-<version>.yaml      # versioned OpenAPI snapshots
│   ├── default.yaml               # points at the snapshot currently used
│   └── daemon-fixtures/           # daemon-side schemas the webapp YAML omits
├── postprocess.mjs                # normalises any OpenAPI input
├── codegen.mjs                    # postprocess + quicktype for 4 languages
└── package.json                   # quicktype-core + yaml + swagger-parser
```

The generated outputs live next to the language wrappers and **are checked
into the repo**:

```
src/core/itscam_rest_types.h                                # C++
src/wrappers/csharp/Itscam.Sdk/RestTypes/RestTypes.g.cs       # C# / .NET
src/wrappers/python/itscam/rest_types.py                      # Python
src/wrappers/go/itscam/rest_types.go                          # Go
```

A normal SDK build (`make` / `make all`) never invokes codegen.

---

## When you might need to regenerate

| Audience                                | Trigger                                            | Recipe                                       |
| --------------------------------------- | -------------------------------------------------- | -------------------------------------------- |
| **SDK maintainer**                      | A new itscam600 firmware release ships             | "Refresh snapshot" below                     |
| **Downstream user of the SDK**          | Need types that match your camera's firmware       | "Regenerate against your own firmware" below |
| **CI**                                  | Verify the committed outputs match the snapshot    | `make codegen-check`                         |

## How the pipeline works

```
spec/itscam-<ver>.yaml             # versioned OpenAPI input (or any user-supplied YAML)
        │
        ▼
postprocess.mjs                    # inlines daemon $refs, injects LanesConfig
        │                          # auto-fixes known JSDoc typos, validates
        ▼
build/itscam.postprocessed.yaml    # intermediate (gitignored)
        │
        ▼
codegen.mjs                        # quicktype × {C++, C#, Python, Go}
        │
        ▼
src/{core,wrappers/...}/rest_types.{hpp,cs,py,go}
```

`codegen.mjs` always runs `postprocess.mjs` first, so the two scripts are
glued together by `make codegen`. Use `postprocess.mjs` directly only when
you want to inspect the intermediate YAML.

## For SDK maintainers — refreshing against a new firmware release

When a new itscam600 firmware is cut:

1. Build the docs in itscam600:

   ```bash
   cd <path-to-itscam600>/modules/camera-apps/webapp/backend
   npm install
   npm run docs   # produces src/static/docs/itscam.yaml
   ```

2. Copy the YAML into the SDK and update the pointer:

   ```bash
   cp <itscam600>/modules/camera-apps/webapp/backend/src/static/docs/itscam.yaml \
      tools/codegen/spec/itscam-<info-version>-<short-sha>.yaml
   cp tools/codegen/spec/itscam-<info-version>-<short-sha>.yaml \
      tools/codegen/spec/default.yaml
   ```

   Use `<info-version>` from the YAML's `info.version` field plus the short
   commit SHA from the itscam600 master used for the build, e.g.
   `itscam-1.0.0-12f96a13.yaml`.

3. Regenerate types and review the diff:

   ```bash
   make codegen
   git status
   git diff
   ```

4. Commit the schema snapshot **and** the regenerated code in the same commit.

5. Old `itscam-<ver>.yaml` snapshots stay in `spec/` for traceability; do not
   delete them when you add a new one.

## For downstream users — regenerating against your own firmware

If you maintain a fork of the SDK and need types matching a camera that runs
a different firmware than the bundled snapshot, you can regenerate locally:

1. Obtain `itscam.yaml` from your camera. Two common paths:
   * Use the spec the firmware exposes (if the route is enabled in your
     deployment), e.g. `curl http://<camera-ip>/api/docs/itscam.yaml > my.yaml`.
   * Or build itscam600 from source: see the project README and run
     `npm run docs` inside `modules/camera-apps/webapp/backend`.

2. Regenerate the SDK types against your spec:

   ```bash
   make codegen SPEC=/path/to/my.yaml
   ```

   No need to overwrite anything in `tools/codegen/spec/` — the override is
   self-contained. The output paths under `src/` are the same as for the
   bundled snapshot.

3. Build the SDK as usual (`make lib`, `make examples`, `make csharp` ...).

### Without Node.js installed

Pass everything through the project's Docker image:

```bash
make docker-codegen SPEC=/sdk/path/to/my.yaml
```

(Paths must be reachable inside the container; the host repo is mounted at
`/sdk` automatically.)

### Writing outputs elsewhere

If you want to inspect the generated files without touching `src/`:

```bash
make codegen OUT_DIR=/tmp/itscam-gen
```

## What the post-processor patches

The webapp OpenAPI document occasionally:

* Documents the `/equipment/lanes` endpoint without bundling its `LanesConfig`
  / `LaneRegion` schemas — these live in
  `modules/camera-apps/camera-daemon/src/openapi/schemas/api_service_lanes.yaml`
  in itscam600 and are inlined here from `spec/daemon-fixtures/`.
* References daemon-side definitions via `$ref: 'api_common.yaml#/...'` — the
  post-processor resolves those against `spec/daemon-fixtures/api_common.yaml`.
* Emits the typo `type: int` for two `Misc.scenarioNOverlayTextSize` fields
  (a JSDoc bug). The post-processor rewrites them to `type: integer`.
* Declares `ExposureConfig.level` as an inline object with only
  `{ targetValue: int32, roi }`, but the camera serialises it as a full
  `ControlAlgorithmConfig` (`mode`/`updateRate`/`targetValue:float`/`holdTime`)
  plus `roi`. The post-processor rewrites the inline shape to
  `ControlAlgorithmConfig + roi` so typed wrappers do not throw a
  `schema mismatch` on the very first `getProfiles()` call.
* Declares `TransitionStepConfig.level` as `integer/int32`, but the camera
  serialises it as a floating-point threshold. The post-processor widens it
  to `number/float` so deserialisers accept both integer and float payloads.

These transforms are idempotent: if the upstream spec is fixed in a future
firmware release, the post-processor simply notes "nothing to inline" /
"already defined" and is otherwise a no-op.

## Strict OpenAPI validation

The default validator only checks that every `$ref` resolves.  Pass
`--strict` to `postprocess.mjs` (not currently surfaced via `make`) to run
swagger-parser's full OpenAPI 3.0 schema validation.  We do not enable this
by default because the upstream JSDoc has known issues that quicktype
nonetheless handles fine.

## Updating the post-processor or codegen tool itself

`postprocess.mjs` and `codegen.mjs` are vanilla Node 18+ ES modules; nothing
exotic.  After editing either:

1. Run `make codegen` and review the diff.
2. Run `make codegen-check` to confirm the committed outputs match.
3. Commit the tool change together with any resulting regenerated files.

## The CI gate

`make codegen-check` (or `make docker-codegen-check`) is wired into the SDK
CI.  It regenerates into a temp directory and exits non-zero if the result
differs from what is currently committed.  Forgetting to regenerate after
touching the spec is therefore caught before the PR can merge.
