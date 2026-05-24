# `ItscamRestClient` -- HTTP/JSON API

[Português (Brasil)](rest-client.md) | [English (US)](rest-client.en-US.md)

O REST client encapsula o **ITSCAM webapp backend** -- o serviço Node.js que roda na câmera e faz proxy de admin endpoints para o camera-daemon. Autenticação é sempre obrigatória.

Header: [`src/core/itscam_rest_client.h`](../../src/core/itscam_rest_client.h). Example C++: [`src/examples/itscam_rest_example.cpp`](../../src/examples/itscam_rest_example.cpp).

> **Referência completa de cada método** (assinatura, typed structs, overloads): veja a [referência Doxygen do C++](/api-ref/cpp/classitscam_1_1ItscamRestClient.html). Esta página foca em conceitos (typed surface vs escape hatch, partial PUT, error handling) e exemplos.

O REST client expõe duas surfaces que coexistem:

* **Typed helpers** (preferido). `getOcrConfig()` devolve `Result<OcrConfig>`, `setOcrConfig(OcrConfig)` aceita e devolve a struct tipada. Os types vivem em [`src/core/itscam_rest_types.hpp`](../../src/core/itscam_rest_types.hpp) e são **gerados automaticamente** a partir do documento OpenAPI da câmera. Veja [`docs/codegen.md`](../codegen.md) para o workflow de refresh / regeneration.
* **HTTP verbs genéricos** (escape hatch). `httpGet` / `httpPut` / `httpPost` / `httpDelete` devolvem `Result<nlohmann::json>` para endpoints que ainda não foram tipados, partial-update bodies, ou fields fora do snapshot atual.

> **Breaking change:** antes da typed surface, `getOcrConfig()` e companhia devolviam `Result<nlohmann::json>`. Migre para os overloads tipados (preferido) ou para o escape hatch genérico `httpGet("/api/equipment/ocr")` quando você especificamente quiser raw JSON.

JSON fields desconhecidos (firmware mais novo, extensões custom) sobrevivem em um `get` tipado porque `nlohmann::json`/`System.Text.Json`/`Python dataclasses`/`Go structs` toleram todos. Round-tripping através de um `set`, porém, descarta fields que o SDK não conhece -- nesse caso, caia para os verbos genéricos ou [regenere contra o spec da sua câmera](../codegen.md).

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
        ocr.value().ocr->enabled = true;            // mutação tipada
        rest.setOcrConfig(ocr.value());
    }
}
```

## Setup e autenticação

```cpp
ItscamRestClient rest;
rest.setBaseUrl("192.168.254.254", 80);
rest.setApiPrefix("/api");                    // default

// Login: POST /api/auth -> armazena JWT internamente
auto loginResult = rest.login("admin", "1234");

// Ou seta um token pré-existente
rest.setAuthToken("eyJ...");
rest.clearAuthToken();
```

Para a configuration de HTTPS veja [`docs/https-tls.md`](../https-tls.md).

## Image profiles

```cpp
using namespace pumatronix::itscam;

auto all  = rest.getProfiles();                       // GET    /api/image/profiles
auto one  = rest.getProfile(0);                       // GET    /api/image/profiles?id=0
ProfileConfig p = all.value().front();

p.trigger.emplace();                                  // Result<ProfileConfig>
p.trigger->enabled = false;

auto updated = rest.updateProfileById((int)p.id, p);  // PUT    /api/image/profiles/{id}
// Aviso: PUT full-document falha em profiles -- use patchJson():
// rest.patchJson("/api/image/profiles/0", {{"trigger", {{"enabled", false}}}});
auto created = rest.createProfile(p);                 // POST   /api/image/profiles
auto deleted = rest.deleteProfile(0);                 // DELETE /api/image/profiles?id=0
                                                       //         (raw JSON response)

// Bulk update via o array endpoint do spec:
auto bulk    = rest.updateProfiles(all.value());      // PUT    /api/image/profiles
```

## Equipment configuration

Phase 1 (typed):

```cpp
using namespace pumatronix::itscam;

// Volatile info (diagnósticos read-only)
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

// I/O ports: bulk + por pin
auto ports     = rest.getIoPorts();             // -> Result<std::vector<IoConfig>>
rest.setIoPorts(ports.value());
auto pin0      = rest.getIoPort(0);             // -> Result<IoConfig>
rest.setIoPort(0, pin0.value());
auto pinsMeta  = rest.getIoBasic();             // -> Result<std::vector<IoBasic>>
rest.setIoBasic(pinsMeta.value());

// REST API client (webhook) servers, endereçados por index
auto webhook        = rest.getRestApiClientConfig(0);
rest.setRestApiClientConfig(0, webhook.value());
auto webhookStatus  = rest.getRestApiClientStatus(0);

auto status   = rest.getItscamproStatus();      // -> Result<ItscamproStatus>
auto licenses = rest.getLicenses();             // -> Result<Licenses>
```

Alguns endpoints ainda devolvem raw JSON porque os schemas deles ainda não fazem parte do snapshot:

```cpp
auto gen = rest.getGeneralConfig();            // -> Result<nlohmann::json>
rest.setGeneralConfig(json);                   // PUT /api/equipment/general
```

## HTTP verbs genéricos (escape hatch)

Para endpoints não cobertos por typed helpers (ou quando você precisa mandar um body de partial-update):

```cpp
auto r1 = rest.httpGet("/api/some/endpoint");
auto r2 = rest.httpPut("/api/some/endpoint", json);
auto r3 = rest.httpPost("/api/some/endpoint", json);
auto r4 = rest.httpDelete("/api/some/endpoint");
```

Os métodos genéricos usam o path **como está** -- nenhum API prefix é prepended, então inclua `/api/...` você mesmo. Use `rest.apiPrefix()` para pegar o prefix configurado se você quiser construir paths relativos a ele.

> **Quando recorrer ao escape hatch:** schema gaps (por exemplo, um endpoint que ainda não foi promovido para um typed wrapper), fields mais novos que o snapshot do SDK, partial updates que omitem fields de propósito (os setters tipados fazem round-trip do objeto inteiro) ou chamadas diagnósticas one-off. Caso contrário, a typed surface é preferível por compile-time safety, IDE completion e paridade entre language bindings.

## Read-modify-write: use partial PUT (PatchJsonAsync)

O ITSCAM daemon trata a maioria dos endpoints PUT como **partial updates**: mande só os fields que você quer mudar e o server faz merge na configuration existente. Dois padrões que **não funcionam** em vários endpoints (notavelmente `PUT /api/image/profiles/{id}`):

* **Round-trip tipado** -- `SetProfilesAsync(await GetProfilesAsync())` descarta fields JSON não documentados durante a deserialisation.
* **Round-trip do documento inteiro** -- GET da config inteira, edita um field, PUT do JSON inteiro de volta. O daemon rejeita com HTTP 500 mesmo quando o body é uma response GET sem modificação (limitação do webapp/daemon, não bug do SDK).

O padrão correto é um **partial PUT** contendo apenas os fields que estão mudando:

```csharp
// C# -- ItscamRestClient.PatchJsonAsync
await rest.PatchJsonAsync("/api/image/profiles/0",
    new JsonObject { ["trigger"] = new JsonObject { ["enabled"] = false } });
```

Equivalente em C++/Python/Go: monte um JSON object minimal e chame `patchJson` / `patch_json` / `PatchJSON`, ou o `httpPut` / `put` / `Put` genérico com o mesmo fragmento.

```cpp
rest.patchJson("/api/image/profiles/0",
               nlohmann::json{{"trigger", {{"enabled", false}}}});
```

`UpdateJsonAsync` (GET + mutação + PUT do documento inteiro) permanece disponível em C# mas não deve ser usado contra image profiles.

Os exemplos C# `MjpegGrabberExample` e `SoftwareTriggerSnapshotExample` usam `GetProfilesAsync()` para encontrar profile ids (read-only) e `PatchJsonAsync()` para todo write.

## Error handling

O REST client compartilha a taxonomia `Result<T>` / `Error::Code` com o binary client; veja [`docs/error-handling.md`](../error-handling.md) para o mapping completo de HTTP-status. Padrão típico:

```cpp
auto r = rest.getOcrConfig();
if (!r) {
    std::cerr << "REST error: " << r.error().message << '\n';
    if (r.error().code == Error::NotAuthenticated)
        rest.login("admin", "1234");      // re-autentica
}
```

Um `get` tipado que devolve sucesso mas com um body que o SDK não conseguiu deserializar para o schema esperado expõe `Error::Code::InvalidParameter` com mensagem `schema mismatch: ...` -- esse é o sinal para regerar os types ([`docs/codegen.md`](../codegen.md)) ou cair temporariamente para os verbos genéricos.

## Logging

```cpp
rest.setLogHandler([](LogLevel lvl, const std::string& msg) {
    if (lvl == LogLevel::Error)
        std::cerr << "[REST ERROR] " << msg << '\n';
    else
        std::cout << "[REST] " << msg << '\n';
});
```
