# `ItscamRestClient` -- HTTP/JSON API

[Português (Brasil)](rest-client.md) | [English (US)](rest-client.en-US.md)

The REST client wraps the **ITSCAM webapp backend** -- the Node.js service that runs on the camera and proxies admin endpoints to the camera-daemon. Authentication is always required.

Header: [`src/core/itscam_rest_client.h`](../../src/core/itscam_rest_client.h). C++ example: [`src/examples/itscam_rest_example.cpp`](../../src/examples/itscam_rest_example.cpp).

> **Full per-method reference** (signatures, typed structs, overloads): see the [generated Doxygen reference](/api-ref/cpp/classitscam_1_1ItscamRestClient.html). This page focuses on concepts (typed surface vs escape hatch, partial serialization, error handling) and examples.

The REST client exposes two coexisting surfaces:

* **Typed helpers** (preferred). `getOcrConfig()` returns `Result<OcrConfig>`, `setOcrConfig(OcrConfig)` accepts and returns the typed struct. The types live in [`src/core/itscam_rest_types.h`](../../src/core/itscam_rest_types.h) and are **auto-generated** from the camera's OpenAPI document. See [`docs/codegen.md`](../codegen.md) for the refresh / regeneration workflow.
* **Generic HTTP verbs** (escape hatch). `httpGet` / `httpPut` / `httpPost` / `httpDelete` return `Result<nlohmann::json>` for endpoints that aren't typed yet, partial-update bodies, or fields outside the current schema snapshot.

> **Breaking change:** prior to the typed surface, `getOcrConfig()` and friends returned `Result<nlohmann::json>`. Migrate to either the typed overloads (preferred) or the generic `httpGet("/api/equipment/ocr")` escape hatch when you specifically want raw JSON.

Unknown JSON fields (newer firmware, custom extensions) survive a typed `get` because `nlohmann::json`/`System.Text.Json`/`Python dataclasses`/ `Go structs` all tolerate them. Round-tripping through a `set` strips fields the SDK does not know about, however -- in that case fall back to the generic verbs or [regenerate against your camera's spec](../codegen.md).

## Quick start

```cpp
#include "itscam_sdk.h"

int main() {
    using namespace itscam;
    using namespace pumatronix::itscam;
    ItscamRestClient rest;

    rest.setBaseUrl("192.168.254.254", 80);                       // HTTP
    // rest.setBaseUrl("camera.example.com", 443, "https");       // HTTPS

    if (!rest.login("admin", "1234")) return 1;

    auto profiles = rest.getProfiles();
    if (profiles)
        std::cout << "profiles: " << profiles.value().size() << '\n';

    auto ocr = rest.getOcrConfig();
    if (ocr && ocr.value().ocr) {
        ocr.value().ocr->enabled = true;            // typed mutation
        rest.setOcrConfig(ocr.value());
    }
}
```

## Setup & authentication

```cpp
ItscamRestClient rest;
rest.setBaseUrl("192.168.254.254", 80);
rest.setApiPrefix("/api");                    // default

// Login: POST /api/auth -> stores JWT internally
auto loginResult = rest.login("admin", "1234");

// Or set a pre-existing token
rest.setAuthToken("eyJ...");
rest.clearAuthToken();
```

For HTTPS configuration see [`docs/https-tls.md`](../https-tls.md).

## Image profiles

```cpp
using namespace pumatronix::itscam;

auto all  = rest.getProfiles();                       // GET    /api/image/profiles
auto one  = rest.getProfile(0);                       // GET    /api/image/profiles?id=0
ProfileConfig p = all.value().front();

p.trigger.emplace();                                  // Result<ProfileConfig>
p.trigger->enabled = false;

auto updated = rest.updateProfileById((int)p.id, p);  // PUT    /api/image/profiles/{id}
                                                       //        (partial serialization --
                                                       //         only set fields are sent)
auto created = rest.createProfile(p);                 // POST   /api/image/profiles
auto deleted = rest.deleteProfile(0);                 // DELETE /api/image/profiles?id=0
                                                       //         (raw JSON response)

// Bulk update via the spec's array endpoint:
auto bulk    = rest.updateProfiles(all.value());      // PUT    /api/image/profiles
```

## Equipment configuration

Phase 1 (typed):

```cpp
using namespace pumatronix::itscam;

// Volatile info (read-only diagnostics)
auto vol = rest.getVolatileInfo();             // -> Result<MiscVolatile>

// Analytics
auto ana = rest.getAnalyticsConfig();          // -> Result<AnalyticsConfig>
rest.setAnalyticsConfig(ana.value());          // PUT /api/equipment/analytics

// OCR
auto ocr = rest.getOcrConfig();                // -> Result<OcrConfig>
rest.setOcrConfig(ocr.value());                // PUT /api/equipment/ocr

// Classifier
auto cls = rest.getClassifierConfig();         // -> Result<ClassifierConfig>
rest.setClassifierConfig(cls.value());         // PUT /api/equipment/classifier

// ITSCAM PRO server
auto pro = rest.getItscamproConfig();          // -> Result<ItscamproConfig>
rest.setItscamproConfig(pro.value());          // PUT /api/equipment/servers/itscampro
```

Phase 2 (typed):

```cpp
auto af = rest.getAutoFocus();                  // -> Result<AutoFocus>
rest.setAutoFocus(af.value());                  // PUT /api/equipment/autofocus

auto streams = rest.getStreamConfig();          // -> Result<StreamConfig>
rest.setStreamConfig(streams.value());          // PUT /api/video/streams

auto misc = rest.getMisc();                     // -> Result<Misc>
rest.setMisc(misc.value());                     // PUT /api/equipment/misc

auto lanes = rest.getLanesConfig();             // -> Result<LanesConfig>
rest.setLanesConfig(lanes.value());             // PUT /api/equipment/lanes

auto sign = rest.getImageSignConfig();          // -> Result<ImageSignConfig>
auto ftp  = rest.getFtpConfig();                // -> Result<FtpConfig>
rest.setFtpConfig(ftp.value());                 // PUT /api/equipment/servers/ftp
auto lince     = rest.getLinceConfig();         // -> Result<LinceConfig>
auto linceStat = rest.getLinceStatus();         // -> Result<LinceStatus>
auto vi        = rest.getVehicleIndicatorConfig();
rest.setVehicleIndicatorConfig(vi.value());     // PUT /api/equipment/vehicleIndicator
auto proto     = rest.getProtocolsConfig();
rest.setProtocolsConfig(proto.value());         // PUT /api/equipment/servers/protocols
auto tr        = rest.getProfileTransitioner();
rest.setProfileTransitioner(tr.value());        // PUT /api/equipment/transitioner

// I/O ports: bulk + per-pin
auto ports     = rest.getIoPorts();             // -> Result<std::vector<IoConfig>>
rest.setIoPorts(ports.value());
auto pin0      = rest.getIoPort(0);             // -> Result<IoConfig>
rest.setIoPort(0, pin0.value());
auto pinsMeta  = rest.getIoBasic();             // -> Result<std::vector<IoBasic>>
rest.setIoBasic(pinsMeta.value());

// REST API client (webhook) servers, addressed by index
auto webhook        = rest.getRestApiClientConfig(0);
rest.setRestApiClientConfig(0, webhook.value());
auto webhookStatus  = rest.getRestApiClientStatus(0);

auto status   = rest.getItscamproStatus();      // -> Result<ItscamproStatus>
auto licenses = rest.getLicenses();             // -> Result<Licenses>
```

A handful of endpoints still surface raw JSON because their schemas aren't part of the snapshot yet:

```cpp
auto gen = rest.getGeneralConfig();            // -> Result<nlohmann::json>
rest.setGeneralConfig(json);                   // PUT /api/equipment/general
```

## Generic HTTP verbs (escape hatch)

For endpoints not covered by typed helpers (or when you need to send a partial-update body):

```cpp
auto r1 = rest.httpGet("/api/some/endpoint");
auto r2 = rest.httpPut("/api/some/endpoint", json);
auto r3 = rest.httpPost("/api/some/endpoint", json);
auto r4 = rest.httpDelete("/api/some/endpoint");
```

The generic methods use the path **as-is** -- no API prefix is prepended, so include `/api/...` yourself. Use `rest.apiPrefix()` to get the configured prefix if you want to build paths relative to it.

> **When to reach for the escape hatch:** schema gaps (e.g. an endpoint hasn't been promoted to a typed wrapper yet), fields newer than the SDK snapshot, untyped payloads for endpoints without a typed helper, or one-off diagnostic calls. Otherwise the typed surface is preferable for compile-time safety, IDE completion, and parity across language bindings.

## Partial updates: typed setters with partial serialization

Typed setters (e.g. `updateProfileById`, `SetOcrConfigAsync`, `set_ocr_config`, `SetOcrConfig`) use **partial serialization**: unset/null/nil/nullopt fields are automatically omitted from the PUT body. Construct a typed struct with only the fields you want to change and pass it directly to the setter -- the daemon merges the supplied fields into the existing configuration.

```csharp
// C# -- typed partial update (preferred)
var patch = new ProfileConfig { Trigger = new TriggerConfig { Enabled = false } };
await rest.UpdateProfileByIdAsync(0, patch);
```

```cpp
// C++ -- typed partial update (preferred)
ProfileConfig patch;
patch.trigger.emplace();
patch.trigger->enabled = false;
rest.updateProfileById(0, patch);
```

### Generic alternative: patchJson

The generic `patchJson` / `PatchJsonAsync` / `patch_json` / `PatchJSON` methods remain available for untyped payloads, endpoints without a typed helper, or when constructing the patch from raw JSON is more convenient:

```cpp
rest.patchJson("/api/image/profiles/0",
               nlohmann::json{{"trigger", {{"enabled", false}}}});
```

The C# `MjpegGrabberExample` and `SoftwareTriggerSnapshotExample` use `GetProfilesAsync()` to find profile ids (read-only) and typed setters or `PatchJsonAsync()` for writes. The [`ProfileConfigExample`](../../src/wrappers/csharp/examples/ProfileConfigExample/) shows a full equipment-configuration scenario: it programs the `Diurno` (1 exposure) and `Noturno` (2 exposures with flash 100% / 5%) profiles with continuous trigger via `UpdateProfileByIdAsync`, configures the MJPEG stream with `SetStreamConfigAsync` plus a `PatchJsonAsync` for the `useTriggerFrames` flag (not yet in the typed schema), and then consumes the resulting stream through `ItscamCgiClient.MjpegFrame`.

## Error handling

The REST client shares the `Result<T>` / `Error::Code` taxonomy with the binary client; see [`docs/error-handling.md`](../error-handling.md) for the full HTTP-status mapping. Typical pattern:

```cpp
auto r = rest.getOcrConfig();
if (!r) {
    std::cerr << "REST error: " << r.error().message << '\n';
    if (r.error().code == Error::NotAuthenticated)
        rest.login("admin", "1234");      // re-authenticate
}
```

A typed `get` that returns successfully but with a body the SDK could not deserialise into the expected schema surfaces `Error::Code::InvalidParameter` with a `schema mismatch: ...` message -- that's the signal to regenerate types ([`docs/codegen.md`](../codegen.md)) or temporarily fall back to the generic verbs.

## Logging

```cpp
rest.setLogHandler([](LogLevel lvl, const std::string& msg) {
    if (lvl == LogLevel::Error)
        std::cerr << "[REST ERROR] " << msg << '\n';
    else
        std::cout << "[REST] " << msg << '\n';
});
```
