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

## Build

```bash
make csharp              # gera Itscam.Sdk.dll para a host platform
make csharp-pack         # gera native binaries + produz o NuGet
```

`make csharp-pack` produz `src/wrappers/csharp/nupkg/Pumatronix.Itscam.Sdk.<version>.nupkg` contendo o managed assembly e um native binary por **host RID** em `runtimes/<rid>/native/`. O RID descreve a máquina onde a aplicação .NET vai rodar (Linux x64, Windows x64, etc.) -- **não** a câmera ITSCAM, que recebe REST/CGI/binary pela rede e não roda o seu app.

Por default o pack inclui os RIDs que o `Makefile` realmente builda:

- `linux-x64` -- gerado por `make lib`
- `win-x64`, `win-x86` -- gerados por `make windows` (cross-compile MinGW)

Slots adicionais existem no [`Itscam.Sdk.csproj`](../../src/wrappers/csharp/Itscam.Sdk/Itscam.Sdk.csproj) e são empacotados *somente se* você produzir os artefatos manualmente:

- `linux-arm` -- precisa de `src/core/build/linux-arm/libitscam_sdk.so.*`
- `linux-arm64` -- precisa de `src/core/build/linux-arm64/libitscam_sdk.so.*`

A bundled toolchain ainda não compila essas variantes; veja o comentário "ARM toolchains may be wired in" no [`Makefile`](../../Makefile) caso precise adicioná-las.

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

// Configuration changes: partial PUT (envie só o que você muda).
await rest.PatchJsonAsync("/api/image/profiles/0",
    new JsonObject { ["trigger"] = new JsonObject { ["enabled"] = false } });

// Raw JSON para endpoints ainda não promovidos a typed helpers:
string json = await rest.GetAsync("/api/equipment/misc/readonly/volatile");
```

`PatchJsonAsync` faz um PUT de documento JSON parcial; o daemon faz merge dos campos enviados sobre a config existente. **Não** dê GET no profile inteiro e PUT de volta -- `PUT /api/image/profiles/{id}` rejeita bodies com documento completo com HTTP 500. Veja [`docs/api/rest-client.md`](../api/rest-client.md) para detalhes.

## Projeto de example

Um example end-to-end completo fica em [`src/wrappers/csharp/examples/CaptureExample/`](../../src/wrappers/csharp/examples/CaptureExample/):

```bash
cd src/wrappers/csharp/examples/CaptureExample
dotnet run -- 192.168.254.254
dotnet run -- 192.168.254.254 --https --insecure --user admin --password 1234
```

A parte de CGI sempre é executada; a parte de REST é pulada quando credentials não são fornecidas (REST sempre exige auth).

## Tutorial passo a passo

Para um walkthrough do zero (criar `dotnet new console`, referenciar o SDK e salvar a primeira imagem em disco), veja [Primeira imagem com C#](../tutorials/first-image-csharp.md).
