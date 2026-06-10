# Primeira imagem com C# / .NET

Walkthrough do zero: criar um projeto .NET console, referenciar o SDK e salvar a primeira imagem JPEG da câmera em disco. Caminho principal usa o **`ItscamCgiClient`** (HTTP, anônimo por default) e há uma seção opcional no final usando o **`ItscamClient`** (Cougar TCP :60000).

## 1. Pré-requisitos

| Item | Versão mínima | Verificar com |
| ---- | ------------- | ------------- |
| .NET SDK | 6.0+ (ou Framework 4.6.1+) | `dotnet --version` |
| Pacote SDK | `itscam-sdk-<version>.tar.gz` | extrair e localizar `csharp/` |
| Câmera ITSCAM | ITSCAM450 / ITSCAM600 alcançável na rede | `ping <ip-da-camera>` |

## 2. Extrair o SDK

Baixe `itscam-sdk-<version>.tar.gz` na [página de releases](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>
```

O NuGet `Pumatronix.Itscam.Sdk` fica em `$SDK/csharp/`. Ele já inclui native binaries para linux-x64, win-x64 e win-x86 -- o MSBuild target file copia o binário correto para o output de build automaticamente.

> **Compilando o SDK do zero?** Se você precisa buildar a partir do source, veja a [seção avançada de build](../getting-started.md#build-do-sdk-a-partir-do-source). Após `make lib && make csharp`, você pode usar `ProjectReference` direto ao csproj ou rodar `make csharp-pack` para gerar o NuGet localmente.

## 3. Criar o projeto

```bash
mkdir -p meu-app && cd meu-app
dotnet new console -n MeuApp -o .
```

## 4. Referenciar o SDK via NuGet

O pacote `Pumatronix.Itscam.Sdk` depende de `System.Memory` e `System.Text.Json`, que vêm do **nuget.org**. Use um `nuget.config` com os dois feeds (local + nuget.org) — `--source $SDK/csharp` sozinho não resolve as dependências transitivas:

```bash
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

O tarball inclui `VERSION.json` na raiz; `nugetVersion` é `0.3.1` em builds de release (no tag) e `0.3.1-dev.N` em builds fora do tag — use sempre esse valor no `--version`.

## 5. Escrever o código mínimo

Substitua `Program.cs` por:

```csharp
// Program.cs
using System;
using System.IO;
using System.Threading.Tasks;

using Pumatronix.Itscam;

class Program
{
    static async Task<int> Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.Error.WriteLine("uso: dotnet run -- <ip-da-camera>");
            return 1;
        }
        string host = args[0];

        using var cgi = new ItscamCgiClient();
        cgi.SetBaseUrl(host, 80);
        // Para HTTPS:
        //   cgi.SetBaseUrl(host, 443, "https");
        // Para auth opcional (somente se configCgi.blockAPI=true):
        //   await cgi.LoginAsync("admin", "1234");

        var frame = await cgi.GetLastFrameAsync();
        File.WriteAllBytes("primeira-imagem.jpg", frame.Data);

        Console.WriteLine(
            $"OK: {frame.Data.Length} bytes salvos em " +
            $"primeira-imagem.jpg ({frame.MimeType})");
        return 0;
    }
}
```

## 6. Executar

```bash
dotnet run -- 192.168.254.254
```

Saída esperada:

```text
OK: 87421 bytes salvos em primeira-imagem.jpg (image/jpeg)
```

Verifique o arquivo:

```bash
file primeira-imagem.jpg
# primeira-imagem.jpg: JPEG image data, JFIF standard 1.01, ...
```

## 7. Troubleshooting

| Sintoma | Causa provável | Solução |
| ------- | -------------- | ------- |
| `DllNotFoundException: itscam_sdk` | Native binary não foi copiado para `bin/Debug/.../` | Confirme que o NuGet do pacote SDK foi referenciado corretamente (`dotnet list package`). |
| `ItscamConnectionException` | IP errado ou porta 80 bloqueada | `curl -v http://<ip>/api/lastframe.cgi -o /dev/null` |
| `ItscamAuthException` em CGI | A câmera tem `configCgi.blockAPI=true` | Chame `await cgi.LoginAsync("user", "pass")` antes do `GetLastFrameAsync()`. |
| `ItscamException: SSL` em HTTPS | CA bundle não configurado | `cgi.SetCaCertFile("/etc/ssl/certs/ca-bundle.pem")` ou, só em dev, `cgi.SetVerifyServerCertificate(false)`. |

## 8. Opcional: capture via `ItscamClient` (TCP :60000)

O CGI é o caminho mais simples. Se você precisa de **trigger em real time** ou multi-exposure, use o binary client (porta 60000):

```csharp
// Program.cs (variante binary)
using Pumatronix.Itscam;

string host = args[0];
string password = args.Length > 1 ? args[1] : "1234";

using var camera = new ItscamClient();
await camera.ConnectAsync(host);
await camera.AuthenticateAsync(password);
await camera.SubscribeCapturesAsync();

var results = await camera.CaptureSnapshotAsync();
if (results == null || results.Count == 0)
{
    Console.Error.WriteLine("nenhum frame retornado");
    return 2;
}

File.WriteAllBytes("primeira-imagem-binary.jpg", results[0].Jpeg);
Console.WriteLine($"OK: {results[0].Jpeg.Length} bytes (binary)");
```

Detalhe completo (auto-reconnect, exposure groups, eventos de trigger contínuos via `TriggerImage` event) em [docs/api/binary-client.md](../api/binary-client.md) e no example [`CaptureExample/Program.cs`](../../src/wrappers/csharp/examples/CaptureExample/Program.cs).

## Próximos passos

- [Guia do wrapper C#](../wrappers/csharp.md) -- API completa.
- [Examples C#](../../src/wrappers/csharp/examples/) -- `CaptureExample`, `MjpegGrabberExample`, `SoftwareTriggerSnapshotExample`, `ProfileConfigExample`.
- [HTTPS / TLS](../https-tls.md) -- configurar mbedTLS para produção.
- [Codegen REST](../codegen.md) -- regenerar `RestTypes.g.cs` para um firmware específico.
