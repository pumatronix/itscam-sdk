# Wrapper C# / .NET

[Português (Brasil)](csharp.md) | [English (US)](csharp.en-US.md)

O wrapper C# fica em [`src/wrappers/csharp/`](../../src/wrappers/csharp/) e tem como target **netstandard2.0**, o que o torna consumível por:

> **Referência completa de classes, métodos e XML docs**: [DocFX do `Pumatronix.Itscam.Sdk`](/api-ref/csharp/index.html). Esta página cobre instalação, padrões idiomáticos e exemplos.


- .NET 6 / 7 / 8 / 9
- .NET Framework 4.6.1+
- Mono / Xamarin / Unity (qualquer runtime que fale netstandard2.0)

Ele cobre as três superfícies do SDK:

| Classe            | Contraparte nativa                            |
| ----------------- | --------------------------------------------- |
| `ItscamClient`    | `src/core/itscam_client.h`                    |
| `ItscamRestClient`| `src/core/itscam_rest_client.h`               |
| `ItscamCgiClient` | `src/core/itscam_cgi_client.h`                |

## Instalação

### Via pacote SDK pré-compilado (recomendado)

O pacote de distribuição (`itscam-sdk-<version>.tar.gz`) inclui um NuGet multi-RID pronto para consumo. Baixe na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>
cat > nuget.config <<EOF
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="itscam-sdk" value="$SDK/csharp" />
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
  </packageSources>
</configuration>
EOF
ITSCAM_VERSION=$(sed -n 's/.*"nugetVersion"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "$SDK/VERSION.json")
dotnet add package Pumatronix.Itscam.Sdk --version "$ITSCAM_VERSION"
```

O NuGet já contém native binaries para linux-x64, linux-arm, linux-arm64, win-x64 e win-x86 (cada um quando o respectivo toolchain estava disponível no momento do pack). O MSBuild target file copia o binário correto para o output de build automaticamente.

### Build a partir do source (avançado)

Se você está desenvolvendo dentro do source tree do SDK:

```bash
make csharp              # gera Itscam.Sdk.dll para a host platform
make csharp-pack         # gera native binaries + produz o NuGet
```

`make csharp-pack` produz `src/wrappers/csharp/nupkg/Pumatronix.Itscam.Sdk.<version>.nupkg` contendo o managed assembly e um native binary por **host RID** em `runtimes/<rid>/native/`. O RID descreve a máquina onde a aplicação .NET vai rodar (Linux x64, Windows x64, etc.) -- **não** a câmera ITSCAM, que recebe REST/CGI/binary pela rede e não roda o seu app.

`make csharp-pack` auto-detecta os cross-toolchains disponíveis e roda `make lib-arm` / `make lib-arm64` / `make windows` antes de empacotar -- então o NuGet final inclui:

- `linux-x64` -- gerado por `make lib`
- `linux-arm` -- gerado por `make lib-arm` (cross Arm GNU-A 8.3-2019.03; ITSCAM450)
- `linux-arm64` -- gerado por `make lib-arm64` (cross Arm GNU-A 8.3-2019.03; ITSCAM600)
- `win-x64`, `win-x86` -- gerados por `make windows` (cross-compile MinGW)

Qualquer RID cujo toolchain não estiver no `PATH` é silenciosamente omitido do pacote final.

Notas específicas do wrapper (convenções de P/Invoke, layout dos native binaries, MSBuild target file) ficam em [`src/wrappers/csharp/README.md`](../../src/wrappers/csharp/README.md).

## Superfície idiomática

| Aspecto        | O que você ganha                                                              |
| -------------- | ----------------------------------------------------------------------------- |
| Async          | `Task` / `Task<T>` em toda blocking call (roda no ThreadPool).                |
| Lifetime       | Todo client implementa `IDisposable`; use `using` ou `Dispose()`.             |
| Streaming      | `event EventHandler<CgiStreamFrame> MjpegFrame` disparado na worker do SDK.   |
| Errors         | Hierarquia `ItscamException` (`ItscamTimeoutException`, `ItscamAuthException`, ...). |
| Strings        | Helpers de UTF-8 marshalling, seguros em netstandard2.0 (sem APIs do .NET 6+).|

## Uso do CGI (auth opcional)

```csharp
using Pumatronix.Itscam;

using var cgi = new ItscamCgiClient();
cgi.SetBaseUrl("192.168.254.254", 80);
// cgi.SetBaseUrl("camera.example.com", 443, "https");
// cgi.SetVerifyServerCertificate(false);   // só para demo / dev

// Opcional: somente quando a câmera tem CGI auth habilitado.
// await cgi.LoginAsync("admin", "1234");

var last = await cgi.GetLastFrameAsync();
File.WriteAllBytes("lastframe.jpg", last.Data);

var snap = await cgi.GetSnapshotAsync(new SnapshotCgiRequest {
    Quality = 80, Mosaic = false,
});

cgi.MjpegFrame += (_, frame) => { /* roda na worker do SDK */ };
cgi.StartMjpegStream();
await Task.Delay(5000);
cgi.StopMjpegStream();
```

## Uso do REST (auth obrigatória)

```csharp
using var rest = new ItscamRestClient();
rest.SetBaseUrl("192.168.254.254", 80);
await rest.LoginAsync("admin", "1234");

// Read-only: typed POCO é conveniente.
List<ProfileConfig> profiles = await rest.GetProfilesAsync();

// Configuration changes: setter tipado com serialização parcial (preferencial).
var patch = new ProfileConfig { Trigger = new TriggerConfig { Enabled = false } };
await rest.UpdateProfileByIdAsync(0, patch);

// Alternativa: patchJson genérico para payloads não tipados.
// await rest.PatchJsonAsync("/api/image/profiles/0",
//     new JsonObject { ["trigger"] = new JsonObject { ["enabled"] = false } });

// Raw JSON para endpoints ainda não promovidos a typed helpers:
string json = await rest.GetAsync("/api/equipment/misc/readonly/volatile");
```

Setters tipados usam **serialização parcial** -- apenas fields explicitamente setados no struct são incluídos no body PUT. O daemon faz merge dos campos enviados sobre a config existente. O genérico `PatchJsonAsync` permanece disponível para payloads não tipados ou endpoints sem typed helper. Veja [`docs/api/rest-client.md`](../api/rest-client.md) para detalhes.

## Projeto de example

Um example end-to-end completo fica em [`src/wrappers/csharp/examples/CaptureExample/`](../../src/wrappers/csharp/examples/CaptureExample/):

```bash
cd src/wrappers/csharp/examples/CaptureExample
dotnet run -- 192.168.254.254
dotnet run -- 192.168.254.254 --https --insecure --user admin --password 1234
```

A parte de CGI sempre é executada; a parte de REST é pulada quando credentials não são fornecidas (REST sempre exige auth).

Para um cenário focado em **equipment configuration** (programar profiles Diurno/Noturno com trigger contínuo, ajustar o MJPEG stream e consumir os frames), veja [`src/wrappers/csharp/examples/ProfileConfigExample/`](../../src/wrappers/csharp/examples/ProfileConfigExample/):

```bash
cd src/wrappers/csharp/examples/ProfileConfigExample
dotnet run -- 192.168.254.254 --user admin --password 1234 \
              --day-profile Diurno --night-profile Noturno \
              --stream-seconds 5
```

## Tutorial passo a passo

Para um walkthrough do zero (criar `dotnet new console`, referenciar o SDK e salvar a primeira imagem em disco), veja [Primeira imagem com C#](../tutorials/first-image-csharp.md).
