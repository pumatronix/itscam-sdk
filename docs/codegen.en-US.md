# Typed REST helpers and codegen

[Português (Brasil)](codegen.md) | [English (US)](codegen.en-US.md)

The ITSCAM REST surface (and its sibling C# / Python / Go wrappers) ships
**typed** data structures for the camera's configuration endpoints --
all of `ProfileConfig`, `OcrConfig`, `AnalyticsConfig`, `ClassifierConfig`,
`StreamConfig`, `Misc`, `MiscVolatile`, `AutoFocus`, `ItscamproConfig`
(Phase 1), plus `ItscamproStatus`, `ImageSignConfig`, `FtpConfig`,
`LinceConfig`/`LinceStatus`, `VehicleIndicatorConfig`, `ProtocolsConfig`,
`ProfileTransitioner`, `LanesConfig`, `IoConfig`/`IoBasic`,
`RestApiClientConfig`/`RestApiClientStatus`, `Licenses` (Phase 2), and the
primitives they share.  These types are **auto-generated** from an
OpenAPI snapshot of the camera's webapp backend that lives inside the
SDK repo:

```
tools/codegen/spec/itscam-<version>.yaml          # versioned snapshot(s)
tools/codegen/spec/default.yaml                   # pointer to the active one
src/core/itscam_rest_types.hpp                    # generated C++
src/wrappers/csharp/Itscam.Sdk/RestTypes/RestTypes.g.cs   # generated C#
src/wrappers/python/itscam/rest_types.py          # generated Python
src/wrappers/go/itscam/rest_types.go              # generated Go
```

Day-to-day SDK builds (`make`, `make all`, `make csharp`, ...) **never
invoke codegen**; the generated files are committed to the repo and
consumed by the regular build.  You only run codegen when:

* (Maintainer) a new ITSCAM firmware ships and you want to refresh the
  bundled snapshot, **or**
* (Downstream user) you maintain a fork and want types matching the
  exact firmware version your cameras run.

Both flows use the same machinery -- one OpenAPI document in, four
language outputs out.

---

## For SDK maintainers -- refreshing against a new firmware release

When a new itscam600 firmware is cut:

1. Build the OpenAPI document in the itscam600 tree:

   ```bash
   cd <itscam600>/modules/camera-apps/webapp/backend
   npm install
   npm run docs        # produces src/static/docs/itscam.yaml
   ```

2. Drop the YAML into the SDK and update the pointer:

   ```bash
   cp <itscam600>/modules/camera-apps/webapp/backend/src/static/docs/itscam.yaml \
      tools/codegen/spec/itscam-<info.version>-<short-sha>.yaml
   cp    tools/codegen/spec/itscam-<info.version>-<short-sha>.yaml \
         tools/codegen/spec/default.yaml
   ```

   The version label follows the convention `<info.version>-<short-sha>`,
   e.g. `itscam-1.0.0-12f96a13.yaml`.  `info.version` comes from the
   YAML's top-level `info:` block; the SHA is the itscam600 commit used
   to produce the docs.

3. Regenerate and review:

   ```bash
   make codegen
   git status
   git diff
   ```

4. Commit the new snapshot, the updated `default.yaml`, and the
   regenerated language outputs in a **single** commit.  CI
   (`make docker-codegen-check`) will reject a commit that touches the
   spec without regenerating, and vice versa.

5. Keep prior `itscam-<version>.yaml` snapshots around for traceability
   -- the `spec/` directory is intentionally additive.

## For downstream users -- regenerating against your own firmware

Have a fork and want types tailored to the firmware your cameras
actually run?  You do not need an itscam600 checkout to do this:

1. Get the OpenAPI document for your firmware -- typically either via
   `curl http://<camera-ip>/api/docs/itscam.yaml > my.yaml` (when the
   route is enabled on the deployment) or by building the
   `webapp/backend` docs yourself from the matching itscam600 tag.

2. Regenerate the SDK types from your file:

   ```bash
   make codegen SPEC=/path/to/my.yaml
   ```

   The bundled `tools/codegen/spec/default.yaml` is untouched; the
   override is fully self-contained.  Generated files overwrite the
   committed copies under `src/...`.

3. Rebuild the SDK as usual (`make lib`, `make examples`, `make csharp`,
   etc.).

### Without Node.js installed locally

Everything runs in the project's Docker builder image:

```bash
make docker-codegen SPEC=/sdk/path/to/my.yaml
```

(Host paths are mounted at `/sdk` automatically; pass a path that is
either under the repo or otherwise reachable inside the container.)

### Writing outputs elsewhere

To inspect the generated files without touching `src/`:

```bash
make codegen OUT_DIR=/tmp/itscam-gen
```

## Compatibility expectations

* **Forward-compatible reads.** All four target languages
  (`nlohmann::json`, `System.Text.Json`, Python `dataclasses`, Go
  structs) tolerate **unknown JSON fields**, so a camera running a
  firmware newer than the bundled snapshot will still deserialise into
  the SDK types -- the new fields are silently dropped.
* **Caveat on writes.** Typed setters round-trip the whole object, so
  fields the SDK does not know about are lost on a `setXxxConfig()`
  call.  If that matters (e.g. you are PUT'ing a partial update),
  fall back to the generic `httpGet`/`httpPut` escape hatch or
  regenerate types against your firmware.
* **Backward-compatible reads.** A camera running an older firmware
  works as long as none of the fields the SDK actually *uses* have
  disappeared.  If a required field is missing the typed getter
  surfaces `Error::Code::InvalidParameter` with a `schema mismatch:
  ...` message -- the cue to regenerate, or to consult the raw JSON
  via the generic verbs.

## Gaps and patches in the snapshot

The webapp OpenAPI document has a handful of quirks that
[`tools/codegen/postprocess.mjs`](../tools/codegen/postprocess.mjs)
papers over.  These transforms are idempotent: if upstream fixes the
quirk, the post-processor is a silent no-op.

| Gap                                                                                                       | Fix                                                                                                                                                       |
| --------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `$ref: 'api_common.yaml#/...'`  pointing at the camera-daemon schemas                                     | Inlined from `tools/codegen/spec/daemon-fixtures/api_common.yaml`                                                                                          |
| `/equipment/lanes` endpoint without `LanesConfig`/`LaneRegion` definitions                                | Injected from `tools/codegen/spec/daemon-fixtures/api_service_lanes.yaml`                                                                                  |
| `type: int` JSDoc typo on a couple of `Misc.scenarioNOverlayTextSize` fields                              | Rewritten to `type: integer` so OpenAPI 3.0 validators accept it                                                                                           |
| `ExposureConfig.level` inline schema declares only `{ targetValue: int32, roi }`, but the camera serialises a full `ControlAlgorithmConfig` (`mode`/`updateRate`/`targetValue:float`/`holdTime`) plus `roi`. | Patched to `ControlAlgorithmConfig + roi` so typed wrappers accept real camera responses without a `schema mismatch` error.                              |
| `TransitionStepConfig.level` declared as `integer/int32`, but the camera serialises a floating-point threshold.                                                                                            | Widened to `number/float` so typed wrappers accept both integer and float payloads.                                                                       |

If you discover a new gap, extend `postprocess.mjs` -- keep the patch
idempotent so it does not break once upstream catches up.

## Escape hatch: generic HTTP verbs

The typed surface intentionally covers Phase 1 schemas.  Other
endpoints, partial-update bodies, and "I just want the raw JSON" cases
all keep working through the unchanged generic verbs in every language:

| Language | Escape hatch                                                                                                |
| -------- | ----------------------------------------------------------------------------------------------------------- |
| C++      | `rest.httpGet(path)` / `httpPut(path, json)` / `httpPost` / `httpDelete` -> `Result<nlohmann::json>`        |
| C#       | `rest.GetAsync(path)` / `PutAsync(path, body)` / `PostAsync` / `DeleteAsync` -> `Task<string>` (raw JSON)   |
| Python   | `rest.get(path)` / `rest.put(path, body)` / `post` / `delete` -> parsed `dict` / `list`                     |
| Go       | `rest.Get(path)` / `Put(path, body)` / `Post` / `Delete` -> `[]byte` JSON                                    |

These are documented per-binding in [`docs/api/rest-client.md`](api/rest-client.md).

## Codegen tool details

The tool itself is documented next to its source in
[`tools/codegen/README.md`](../tools/codegen/README.md) -- pipeline
diagram, post-processor behaviour, strict validation flag, and how to
modify the tool itself.

## See also

* [`docs/api/rest-client.md`](api/rest-client.md) -- the C++ REST API,
  typed surface + escape hatch.
* [`AGENTS.md`](../AGENTS.md) §11 -- adding a new feature (typed
  surface checklist).
* [`tools/codegen/README.md`](../tools/codegen/README.md) -- the codegen
  pipeline and CI gate.
