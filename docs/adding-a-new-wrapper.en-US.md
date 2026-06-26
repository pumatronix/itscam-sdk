# Adding a new language wrapper

[Português (Brasil)](adding-a-new-wrapper.md) | [English (US)](adding-a-new-wrapper.en-US.md)

This chapter documents the canonical procedure for adding a new language (Rust, Ruby, Swift, Kotlin, PHP, ...) to the ITSCAM Client SDK. The SDK already exposes six wrappers -- C++, C#, Python, Go, Java, and Node.js -- and they all follow the same pattern. Replicate the pattern for any new language.

## 1. Why a C ABI?

Every wrapper talks to the same C++ core through a stable **C ABI** under [`src/core/c_api/`](../src/core/c_api/) (`itscam_sdk_c.h`, `itscam_rest_client_c.h`, `itscam_cgi_client_c.h`).

The golden rule from [`AGENTS.md`](../AGENTS.md):

> Start in `src/core/`, extend the C API, then mirror the surface in **every** wrapper.

You will **not** be writing new C++ for a language wrapper. Your job is to translate the ~120 entry-points in the C API into idiomatic code for your target language.

## 2. Decision #1 -- pick the binding technology

| Technology | Use it when |
| ---------- | ----------- |
| **Dynamic FFI** (libffi-style) | Your language has a mature, well-maintained FFI library that loads `.so`/`.dll` at runtime. **Preferred.** Examples: ctypes (Python), JNA (Java), koffi (Node.js), cgo (Go), P/Invoke (C#), CFFI (Lua/Ruby), Foreign (Common Lisp / Clojure), Foreign Function & Memory API (Java 22+ modern), `ffi` (Rust dynamic). |
| **Compiled native module** (JNI/NAPI/Ruby C ext/Python C ext/Rust crate) | Your language **does not** have an idiomatic dynamic FFI **or** the FFI overhead is unacceptable **or** you need specialised types (NIO buffers, JS Buffer zero-copy, etc.). Caveat: you now have to build against SDK headers across every OS x arch combination. |
| **Auto-generated bindings** (SWIG, cbindgen->bindgen) | Not recommended for the user-facing wrapper -- they emit a low-level surface that fights the idiomatic style of each language. They can serve as an internal layer beneath an idiomatic facade. |

**Safe default:** dynamic FFI. That's what Python (ctypes), Go (cgo via wrappers), C# (P/Invoke), Java (JNA), and Node.js (koffi) chose. Deterministic build, no extra toolchain for the consumer.

## 3. Decision #2 -- pick the packaging

| Language | Package format |
| -------- | -------------- |
| Python    | wheel (setuptools) |
| C# / .NET | NuGet (.nupkg) |
| Go        | module + cgo directives |
| Java      | JAR (Maven), with native libraries under `META-INF/native/<os>-<arch>/` |
| Node.js   | npm tarball, with native libraries under `native/<platform>-<arch>/` |
| Rust      | Cargo crate, with `build.rs` that copies `libitscam_sdk` |
| Ruby      | gem with FFI extension |
| Swift     | Swift Package + binary target |
| Kotlin / JVM | publish through the same Java JAR (Kotlin is interop-native) |
| PHP       | Composer with FFI extension (PHP 8.2+) |

**Packaging rules:**

1. **Native binary embedded per OS x arch.** The consumer must not need to install `libitscam_sdk` system-wide. Use the staging-script pattern (`tools/packaging/stage-*-natives.sh`).
2. **Documented native-library resolution order.** The standard order: env-var override -> package directory -> `LD_LIBRARY_PATH`/system loader -> embedded fallback. Document this in the wrapper README.
3. **Synchronised versions.** Versions come from `tools/version/gen-version.sh`. Add an output for your language (Python, C#, and Go all do this).

## 4. Decision #3 -- pick the async model

| Pattern | Use it when |
| ------- | ----------- |
| Sync only | Languages without idiomatic async support (Lua, shell scripts). |
| Sync + async parallel | Default. Every blocking call has a `*Async` companion that returns the language's native async type (`Promise`, `Future<T>`, `Task<T>`, coroutine, ...). The wrapper internally uses `Task.Run` / an executor / `Promise.resolve(() => syncFn())`. |
| Async-first | Only if the language has **no** sync APIs (rarely the case). |

**Safe default:** sync + async in parallel. C# uses `Task<T>`, Java uses `Future<T>` for JDK 7 compatibility, Node uses `Promise<T>`, Python exposes sync (asyncio is opt-in).

## 5. Expected directory layout

```
src/wrappers/<language>/
  README.md                  # wrapper-specific, mirrors the existing style
  <package-manifest>         # pom.xml / setup.py / package.json / Cargo.toml / ...
  <source-tree>/
    bindings.<ext>           # loads libitscam_sdk and declares signatures
    errors.<ext>             # ItscamError + subclasses
    itscam_client.<ext>      # ItscamClient
    itscam_rest_client.<ext> # ItscamRestClient
    itscam_cgi_client.<ext>  # ItscamCgiClient
    jpeg_utils.<ext>         # pure-language COM marker parser (no native dep)
    types.<ext>              # POJOs / dataclasses / interfaces
  examples/
    capture_example.<ext>    # binary client (TCP 60000)
    rest_example.<ext>       # REST (login required)
    cgi_snapshot_example.<ext>  # CGI (auth optional)
```

## 6. Implementation checklist

Use the Java/Node.js wrappers as the reference. Every item is mandatory or explicitly out of scope (with justification in the wrapper README).

### 6.1 Core surface

- [ ] **Lifecycle:** `create` / `destroy` mapped to constructor + destructor / `close()` / `Symbol.dispose` / `IDisposable` / `with`-context / `defer Close()`.
- [ ] **Connection (binary):** `connect`, `disconnect`, `isConnected`, `setAutoReconnect`, or `AutoReconnectConfig` parameter on `connect`.
- [ ] **Authentication:**
    - Binary: `authenticate(password)`.
    - REST: `login(user, pass)` **with explicit documentation** that REST always requires authentication.
    - CGI: `login(...)` opt-in, `setBasicAuth`, `setAuthToken`. **Do not** call login from library / example code by default. See [`AGENTS.md`](../AGENTS.md) Section 10 (anti-patterns).
- [ ] **Capture (binary):** `captureSnapshot`, `getLastFrame`, `subscribe`, `subscribeCaptures`.
- [ ] **Profiles (binary):** `getActiveProfileId`, `setActiveProfile`, `listProfiles`.
- [ ] **REST verbs:** `httpGet`, `httpPut`, `httpPost`, `httpDelete`, `patchJson`. **Typed setters are the preferred path for writes** -- they use partial serialization (unset fields are omitted from the PUT body). The generic `patchJson` remains available for untyped payloads or endpoints without a typed helper.
- [ ] **REST typed helpers:** at minimum `getProfiles`, `getOcrConfig`/`setOcrConfig`, `getAnalyticsConfig`/`setAnalyticsConfig`, `getClassifierConfig`/`setClassifierConfig`, `getLanesConfig`/`setLanesConfig`, `getItscamproConfig`/`setItscamproConfig`, `getVolatileInfo`. Decide whether to return typed structs (preferred when the language has POCOs with codegen, see Section 6.6) or raw JSON.
- [ ] **CGI:** `getLastFrame`, `getSnapshot`, `startMjpegStream`/`stopMjpegStream`, `forceTrigger`, `reboot`.
- [ ] **TLS configuration:** `setBaseUrl(host, port, scheme)`, `setCaCertFile`, `setCaCertData`, `setVerifyServerCertificate`, `setClientCertificate` (on REST and CGI).
- [ ] **System utilities:** `getVersion`, `getSystemLocalTime`, `getSystemUtcTime`, `getEpochTime`, `getEpochTimeMs`, `getLastError`.

### 6.2 Errors

- [ ] Exception hierarchy rooted in `ItscamError` (or `ItscamException`, depending on naming convention).
- [ ] Subclasses for `Timeout`, `NotAuthenticated`, `ConnectionFailed`/`Disconnected`, `InvalidParameter`, `ServerError`.
- [ ] Every call checks the C `Result::Code` and converts to the right exception. Append the `ITSCAM_getLastError()` text to the message.
- [ ] Exception type carries the numeric code (`getCode()` / `.code` / equivalent) so callers can discriminate programmatically.

### 6.3 Callbacks / streaming

- [ ] Callbacks (`onTriggerImage`, `onSnapshotImage`, `onConnectionState`, `onDisconnect`, `onLog`, `startMjpegStream`) are **always** delivered on the SDK worker thread.
- [ ] Document: **do not block callbacks**. Marshalling (queue / channel / dispatcher) is the user's responsibility.
- [ ] Hold strong references to the trampoline / callback objects so the GC does not collect them while native code is invoking them. See `_callbackRefs` in Node, `callbacks` map in Java/Python, `_streamDelegate` in C#.
- [ ] Swallow exceptions inside callbacks (with optional log/trace). Never let an exception cross back into C.

### 6.4 JPEG utilities

- [ ] Helpers to extract the COM marker and parse `Placa`/`CoordPlaca`/`ClassifierList`/`BMCList`. **No native dependency** (pure language code).
- [ ] Mirror the Python helpers ([`jpeg_utils.py`](../src/wrappers/python/itscam/jpeg_utils.py)) and Java helpers ([`JpegMetadata.java`](../src/wrappers/java/itscam-sdk/src/main/java/com/pumatronix/itscam/jpeg/JpegMetadata.java)) exactly.

### 6.5 Examples (minimum 3)

- [ ] **`capture_example`** -- binary client, optional password. Mirror of [`capture_example.py`](../src/wrappers/python/examples/capture_example.py).
- [ ] **`rest_example`** -- REST login + typed helpers + generic verbs.
- [ ] **`cgi_snapshot_example`** -- CGI: `lastframe`, `snapshot.cgi` (multi-image), MJPEG stream for 5 seconds. Optional `--user/--password` auth.
- [ ] Every example honours the flag protocol: `--https`, `--insecure`, `--user`, `--password`. CGI is anonymous by default.

### 6.6 REST type codegen (typed helpers)

The SDK generates POCOs / dataclasses / structs for the REST configuration types from the bundled OpenAPI snapshot in [`tools/codegen/spec/default.yaml`](../tools/codegen/spec/default.yaml). See [`docs/codegen.md`](codegen.md).

Current status:

| Wrapper | Typed REST POCOs |
| ------- | ---------------- |
| C++ | yes (`itscam_rest_types.h`) |
| C# | yes (`RestTypes/RestTypes.g.cs`) |
| Python | yes (`itscam/rest_types.py`) |
| Go | yes (`itscam/rest_types.go`) |
| Java | yes (`resttypes/`, Gson-backed maintained source) |
| Node.js | **follow-up** -- generic verbs return parsed `JsonValue`; codegen will return TS interfaces. |

**How to add codegen for a new language:**

1. The generator is [`tools/codegen/codegen.mjs`](../tools/codegen/codegen.mjs), backed by [quicktype](https://quicktype.io/), which already supports Java, TypeScript, Kotlin, Swift, Rust, Ruby, Crystal, Elm, Haskell, etc. natively.
2. Add a new entry to the `TARGETS` array in `codegen.mjs`. You need:
   - `name` (human-readable label).
   - `lang` (a string accepted by quicktype: `"java"`, `"typescript"`, `"rust"`, `"kotlin"`, `"swift"`, `"ruby"`, ...).
   - `rendererOptions` (language-specific quicktype flags; check their docs).
   - `outRel` (path relative to the repo root for the generated output).
   - `notice` (file header comment with SPDX and the phrase "AUTO-GENERATED").
   - `postProcess` (optional -- a function that takes the quicktype output and returns the final, idempotent version).
3. Update `docs/codegen.md` with the new row in the "Generated outputs" table.
4. Run `make codegen` (Node 18+ locally) or `make docker-codegen`. Commit the new generated files together with the `codegen.mjs` change in a single commit. CI (`make codegen-check`) enforces synchronisation.

### 6.7 Build / packaging

- [ ] Package manifest at the canonical path (see Section 5).
- [ ] `tools/packaging/stage-<lang>-natives.sh` script that copies `src/core/build/<rid>/libitscam_sdk.{so,dll}` into the package's resource directory.
- [ ] Top-level Makefile with at least:
    - `make <lang>` -- build the library.
    - `make <lang>-pack` -- produce the distributable artifact (JAR / .nupkg / .whl / .tgz / .gem / .crate).
    - `make <lang>-examples` -- build examples.
    - `make docker-<lang>` / `make docker-<lang>-pack` -- same flow inside the Docker builder.
- [ ] Update `make help` to list the new targets.
- [ ] Update `clean:` to remove artifacts (`target/`, `node_modules/`, `__pycache__/`, `bin/`/`obj/`, etc.).

### 6.8 Versioning

- [ ] Extend [`tools/version/gen-version.sh`](../tools/version/gen-version.sh) to emit your language's version in the right format:
    - Plain SemVer (Maven, npm, Cargo).
    - PEP 440 (Python wheels).
    - .NET-friendly (NuGet 4-part).
- [ ] The wrapper exposes `getVersion()` (or equivalent property) returning the package version.

### 6.9 Distribution

- [ ] Extend [`tools/packaging/make-sdk-dist.sh`](../tools/packaging/make-sdk-dist.sh) to ship the artifact in the consumer tarball (`itscam-sdk-<version>.tar.gz`) under `linux-x64/<lang>/`, `win-x64/<lang>/`, etc.
- [ ] The `sdk-dist` Makefile target invokes the new language build before staging.

### 6.10 Documentation

- [ ] **Wrapper guide:** `docs/wrappers/<lang>.md` (PT-BR) + `<lang>.en-US.md` (EN). Mirror the structure of [`docs/wrappers/python.md`](wrappers/python.md): installation, native-library resolution, idiomatic surface, three examples (binary/REST/CGI), partial serialization on typed setters, link to tutorial.
- [ ] **Tutorial:** `docs/tutorials/first-image-<lang>.md` + EN counterpart. From-scratch walkthrough with an optional binary-client fallback.
- [ ] **Quick-link matrix** in [`README.md`](../README.md) -- add a new column to the use-case table and a row to the wrapper guides table.
- [ ] **Doc index** [`docs/README.md`](README.md) -- add bullets under "Language wrappers" and "Tutorials".
- [ ] **Auto-generated API reference** (optional but recommended): add `docs-api-<lang>` to the Makefile (Doxygen for C++, pdoc for Python, DocFX for C#, gomarkdoc for Go, javadoc for Java, typedoc for TS/JS, rustdoc for Rust, ...).

## 7. Anti-patterns

Don't:

- **Do not** create a parallel C API. Everything goes through `src/core/c_api/`. If the C API does not cover what you need, extend the C API (and propagate to every other wrapper; see [`AGENTS.md`](../AGENTS.md) Section 11).
- **Do not** change the idiomatic naming "to stay consistent with C++". Each wrapper follows the naming convention of its language (`camelCase` / `snake_case` / `PascalCase` / ...).
- **Do not** add `cgi.login()` to non-opt-in code. CGI is anonymous by default ([`AGENTS.md`](../AGENTS.md) Section 10).
- **Do not** silently ignore `Result<T>` or error codes. Throw an exception or return an error.
- **Do not** dispatch callbacks on the user's thread. Always on the SDK worker; the user marshals.
- **Do not** hand-edit codegen output (the files have an "AUTO-GENERATED" header). Extend `codegen.mjs` instead.
- **Do not** rebuild the native library from the wrapper. The wrapper depends on `libitscam_sdk.{so,dll}` produced by `make lib` / `make windows`.

## 8. Final PR checklist

Before opening the PR adding the new wrapper:

- [ ] Every item in Section 6 is covered or explicitly justified in the wrapper README (e.g. "Codegen REST helpers -- follow-up tracked in #NNN").
- [ ] `make all` passes (build everything + new wrapper).
- [ ] `make docker-all` passes (same, inside Docker; reproducible for CI).
- [ ] `make codegen-check` passes if the wrapper participates in codegen.
- [ ] `make regression-examples CAMERA_IP=<ip>` passes against a live camera -- including the new wrapper's capture/rest/cgi examples.
- [ ] The version is generated by `tools/version/gen-version.sh` (or has a hard-coded fallback `0.0.0` while the version integration is pending; leave a TODO in the shell script).
- [ ] PR mentions `AGENTS.md` if a wrapper rule changed (e.g. new error codes that need to be reflected in every wrapper).
- [ ] Quick-link matrix updated in `README.md` and `docs/README.md`.

## See also

- [`AGENTS.md`](../AGENTS.md) -- short briefing for coding agents; Sections 3-6 explain auth, threading, errors, and wrapper layout.
- [`docs/codegen.md`](codegen.md) -- codegen workflow for typed REST helpers.
- [Wrapper guides](README.en-US.md#language-wrappers) -- existing guides under `docs/wrappers/` (use as reference).
- [`tools/codegen/codegen.mjs`](../tools/codegen/codegen.mjs) -- POCO generation pipeline.
- [`tools/version/gen-version.sh`](../tools/version/gen-version.sh) -- single source of truth for versioning.
