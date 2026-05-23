# Typed REST helpers e codegen

[Português (Brasil)](codegen.md) | [English (US)](codegen.en-US.md)

A REST surface do ITSCAM (e os wrappers irmãos C# / Python / Go)
entrega data structures **tipadas** para os endpoints de configuration
da câmera -- todos os `ProfileConfig`, `OcrConfig`, `AnalyticsConfig`,
`ClassifierConfig`, `StreamConfig`, `Misc`, `MiscVolatile`,
`AutoFocus`, `ItscamproConfig` (Phase 1), além de `ItscamproStatus`,
`ImageSignConfig`, `FtpConfig`, `LinceConfig`/`LinceStatus`,
`VehicleIndicatorConfig`, `ProtocolsConfig`, `ProfileTransitioner`,
`LanesConfig`, `IoConfig`/`IoBasic`,
`RestApiClientConfig`/`RestApiClientStatus`, `Licenses` (Phase 2), e
os primitives compartilhados. Esses tipos são **gerados
automaticamente** a partir de um snapshot OpenAPI do webapp backend da
câmera, que vive dentro do repo do SDK:

```
tools/codegen/spec/itscam-<version>.yaml          # snapshot(s) versionado(s)
tools/codegen/spec/default.yaml                   # ponteiro para o ativo
src/core/itscam_rest_types.hpp                    # C++ gerado
src/wrappers/csharp/Itscam.Sdk/RestTypes/RestTypes.g.cs   # C# gerado
src/wrappers/python/itscam/rest_types.py          # Python gerado
src/wrappers/go/itscam/rest_types.go              # Go gerado
```

Builds rotineiros do SDK (`make`, `make all`, `make csharp`, ...)
**nunca rodam codegen**; os arquivos gerados ficam committed no repo e
são consumidos pelo build normal. Você só roda codegen quando:

* (Maintainer) sai um firmware ITSCAM novo e você quer atualizar o
  snapshot bundled, **ou**
* (Downstream user) você mantém um fork e quer types que batem com a
  versão exata do firmware das suas câmeras.

Os dois fluxos usam a mesma máquina -- um documento OpenAPI de input,
quatro outputs por linguagem.

---

## Para SDK maintainers -- atualizar contra um firmware novo

Quando sai um firmware itscam600 novo:

1. Construa o documento OpenAPI no tree do itscam600:

   ```bash
   cd <itscam600>/modules/camera-apps/webapp/backend
   npm install
   npm run docs        # produz src/static/docs/itscam.yaml
   ```

2. Coloque o YAML no SDK e atualize o ponteiro:

   ```bash
   cp <itscam600>/modules/camera-apps/webapp/backend/src/static/docs/itscam.yaml \
      tools/codegen/spec/itscam-<info.version>-<short-sha>.yaml
   cp    tools/codegen/spec/itscam-<info.version>-<short-sha>.yaml \
         tools/codegen/spec/default.yaml
   ```

   O label da versão segue a convenção `<info.version>-<short-sha>`,
   por exemplo `itscam-1.0.0-12f96a13.yaml`. `info.version` vem do
   bloco top-level `info:` do YAML; o SHA é o commit do itscam600 que
   produziu os docs.

3. Regenere e revise:

   ```bash
   make codegen
   git status
   git diff
   ```

4. Commite o snapshot novo, o `default.yaml` atualizado e os outputs
   regenerados de cada linguagem em **um único** commit. CI
   (`make docker-codegen-check`) rejeita commit que toca o spec sem
   regerar, e vice-versa.

5. Mantenha os snapshots `itscam-<version>.yaml` antigos para
   traceability -- o diretório `spec/` é intencionalmente aditivo.

## Para downstream users -- regenerar contra seu próprio firmware

Você mantém um fork e quer types feitos sob medida para o firmware que
suas câmeras realmente rodam? Não precisa de um checkout do itscam600
para isso:

1. Pegue o documento OpenAPI do seu firmware -- normalmente via
   `curl http://<camera-ip>/api/docs/itscam.yaml > my.yaml` (quando a
   rota está habilitada no deployment) ou buildando o
   `webapp/backend` docs por conta própria a partir do tag itscam600
   correspondente.

2. Regenere os types do SDK a partir do seu arquivo:

   ```bash
   make codegen SPEC=/path/to/my.yaml
   ```

   O `tools/codegen/spec/default.yaml` bundled fica intocado; o
   override é totalmente self-contained. Os arquivos gerados sobrescrevem
   as cópias committed em `src/...`.

3. Recompile o SDK como sempre (`make lib`, `make examples`,
   `make csharp`, etc.).

### Sem Node.js instalado localmente

Tudo roda no builder Docker do projeto:

```bash
make docker-codegen SPEC=/sdk/path/to/my.yaml
```

(Paths do host são montados em `/sdk` automaticamente; passe um path
que esteja dentro do repo ou de outro modo alcançável dentro do
container.)

### Escrever outputs em outro lugar

Para inspecionar os arquivos gerados sem mexer em `src/`:

```bash
make codegen OUT_DIR=/tmp/itscam-gen
```

## Compatibility expectations

* **Reads forward-compatible.** Os quatro targets (`nlohmann::json`,
  `System.Text.Json`, `dataclasses` Python, structs Go) toleram
  **fields JSON desconhecidos**, então uma câmera rodando firmware
  mais novo que o snapshot bundled ainda deserializa para os types do
  SDK -- os fields novos são silenciosamente descartados.
* **Cuidado em writes.** Setters tipados fazem round-trip do objeto
  inteiro, então fields que o SDK não conhece são perdidos numa
  chamada `setXxxConfig()`. Se isso é importante (por exemplo, você
  está fazendo PUT de partial update), caia para o escape hatch
  genérico `httpGet`/`httpPut` ou regenere types contra seu firmware.
* **Reads backward-compatible.** Uma câmera rodando firmware mais
  antigo funciona enquanto nenhum dos fields que o SDK *usa* tiver
  sumido. Se um field obrigatório está ausente, o getter tipado
  expõe `Error::Code::InvalidParameter` com mensagem
  `schema mismatch: ...` -- a deixa para regerar, ou para consultar o
  JSON cru via os verbos genéricos.

## Gaps e patches no snapshot

O documento OpenAPI do webapp tem alguns quirks que
[`tools/codegen/postprocess.mjs`](../tools/codegen/postprocess.mjs)
disfarça. Esses transforms são idempotentes: se upstream corrigir o
quirk, o post-processor vira um no-op silencioso.

| Gap                                                                                                       | Fix                                                                                                                                                       |
| --------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `$ref: 'api_common.yaml#/...'` apontando para schemas do camera-daemon                                    | Inlined a partir de `tools/codegen/spec/daemon-fixtures/api_common.yaml`                                                                                   |
| Endpoint `/equipment/lanes` sem definições `LanesConfig`/`LaneRegion`                                     | Injetado a partir de `tools/codegen/spec/daemon-fixtures/api_service_lanes.yaml`                                                                           |
| Typo de JSDoc `type: int` em alguns fields `Misc.scenarioNOverlayTextSize`                                 | Reescrito para `type: integer` para que validators OpenAPI 3.0 aceitem                                                                                     |
| `ExposureConfig.level` declara `{ targetValue: int32, roi }` no schema inline, mas a câmera serializa um `ControlAlgorithmConfig` completo (`mode`/`updateRate`/`targetValue:float`/`holdTime`) mais `roi`. | Patched para `ControlAlgorithmConfig + roi` para que wrappers tipados aceitem responses reais sem `schema mismatch`.                                      |
| `TransitionStepConfig.level` declarado como `integer/int32`, mas a câmera serializa um threshold de ponto flutuante.                                                                                            | Alargado para `number/float` para que wrappers tipados aceitem payloads tanto integer quanto float.                                                       |

Se você descobrir um novo gap, estenda `postprocess.mjs` -- mantenha o
patch idempotente para que ele não quebre quando upstream alcançar.

## Escape hatch: HTTP verbs genéricos

A typed surface cobre intencionalmente os schemas Phase 1. Outros
endpoints, bodies de partial-update e casos do tipo "só quero o JSON
cru" continuam funcionando pelos verbos genéricos inalterados em cada
linguagem:

| Linguagem | Escape hatch                                                                                                |
| --------- | ----------------------------------------------------------------------------------------------------------- |
| C++       | `rest.httpGet(path)` / `httpPut(path, json)` / `httpPost` / `httpDelete` -> `Result<nlohmann::json>`        |
| C#        | `rest.GetAsync(path)` / `PutAsync(path, body)` / `PostAsync` / `DeleteAsync` -> `Task<string>` (raw JSON)   |
| Python    | `rest.get(path)` / `rest.put(path, body)` / `post` / `delete` -> `dict` / `list` parseados                  |
| Go        | `rest.Get(path)` / `Put(path, body)` / `Post` / `Delete` -> `[]byte` JSON                                    |

Estão documentados por binding em
[`docs/api/rest-client.md`](api/rest-client.md).

## Detalhes do tool de codegen

O tool em si está documentado ao lado do source em
[`tools/codegen/README.md`](../tools/codegen/README.md) -- diagrama do
pipeline, comportamento do post-processor, strict validation flag e
como modificar o próprio tool.

## Veja também

* [`docs/api/rest-client.md`](api/rest-client.md) -- a REST API C++,
  typed surface + escape hatch.
* [`AGENTS.md`](../AGENTS.md) §11 -- adicionar uma feature nova
  (checklist da typed surface).
* [`tools/codegen/README.md`](../tools/codegen/README.md) -- o pipeline
  de codegen e o CI gate.
