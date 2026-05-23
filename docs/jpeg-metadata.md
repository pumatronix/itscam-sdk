# Metadados JPEG (COM marker)

[Português (Brasil)](jpeg-metadata.md) | [English (US)](jpeg-metadata.en-US.md)

Câmeras ITSCAM embutem resultados de reconhecimento de placas e classificação de veículos diretamente nos bytes da imagem JPEG, usando o **COM marker** (0xFF 0xFE) do padrão JFIF. Isso significa que toda imagem é autossuficiente: os metadados viajam junto com o arquivo e nunca se perdem, mesmo quando a imagem é copiada, transferida por FTP, ou armazenada em disco sem nenhum sidecar.

O SDK fornece utilitários para extrair e parsear esses metadados em todas as linguagens suportadas.

## Formato do comment

O corpo do COM marker é uma string ASCII com pares `chave=valor` separados por ponto-e-vírgula:

```
Placa=ABC1234;CoordPlaca=100x200,150x40;CorPlaca=0;OCRCountryCode=55;ClassifierList=[1,95,50,100,300,200];BMCList=["VW",90,"Gol",85,"Branco",92]
```

### Tags de reconhecimento de placas

| Tag              | Formato                     | Descrição                                                        |
| ---------------- | --------------------------- | ---------------------------------------------------------------- |
| `Placa`          | `ABC1234` ou `ABC_DEF`      | Texto da placa. Múltiplas placas separadas por `_`.              |
| `CoordPlaca`     | `XxY,WxH` (sep. por `_`)   | Bounding box de cada placa na imagem.                            |
| `CorPlaca`       | `0` ou `1` (sep. por `_`)   | Cor da placa (`0` = branca sobre preto, `1` = preta sobre branco). |
| `OCRCountryCode` | inteiro (sep. por `_`)      | Código do país de cada placa.                                    |

### Tags de classificação de veículos

| Tag              | Formato                                       | Descrição                                              |
| ---------------- | --------------------------------------------- | ------------------------------------------------------ |
| `ClassifierList` | `[tipo,prob,x,y,w,h],[...]`                   | Detecções. `prob` é 0–100 (porcentagem).               |
| `BMCList`        | `["marca",pM,"modelo",pMo,"cor",pC],[...]`    | Brand/Model/Color. Probabilidades 0–100.               |

## API por linguagem

### C++ (header-only)

```cpp
#include "itscam_jpeg_utils.h"

// Extrair comment cru
std::string comment = itscam::extractJpegComment(jpeg.data(), jpeg.size());

// Parsear em mapa chave=valor
auto tags = itscam::parseJpegCommentTags(comment);

// Extrair resultados estruturados
auto plates  = itscam::extractPlateRecognitions(tags);
auto objects = itscam::extractObjectDetections(tags);

// One-shot: popular CaptureResult a partir do JPEG COM
itscam::populateFromJpegComment(result);
```

Todas as funções são `inline` e definidas em [`src/core/itscam_jpeg_utils.h`](../src/core/itscam_jpeg_utils.h). Não requerem link com nenhuma biblioteca adicional.

### C API

```c
#include "itscam_sdk_c.h"

// Consultar tamanho e extrair
size_t len = ITSCAM_Jpeg_extractComment(jpegData, jpegSize, NULL, 0);
if (len > 0) {
    char* buf = malloc(len + 1);
    ITSCAM_Jpeg_extractComment(jpegData, jpegSize, buf, len + 1);
    printf("Comment: %s\n", buf);
    free(buf);
}
```

### Python

```python
from itscam import (
    extract_jpeg_comment,
    parse_jpeg_comment_tags,
    parse_jpeg_metadata,
)

# A partir de bytes JPEG crus
comment = extract_jpeg_comment(jpeg_bytes)
tags = parse_jpeg_comment_tags(comment)

# One-shot com resultados estruturados
meta = parse_jpeg_metadata(jpeg_bytes)
for plate in meta.plates:
    print(f"Placa: {plate.plate} em ({plate.x},{plate.y})")
for obj in meta.objects:
    print(f"Veículo tipo {obj.type} ({obj.probability:.0%})")

# Via CaptureResult (lazy, com cache)
result = client.capture_snapshot()[0]
print(result.comment)       # string crua
print(result.comment_tags)  # dict
```

### C# / .NET

```csharp
using Pumatronix.Itscam;

// Funções estáticas
string comment = JpegUtils.ExtractComment(jpegBytes);
var tags = JpegUtils.ParseCommentTags(comment);
var plates = JpegUtils.ExtractPlateRecognitions(tags);
var objects = JpegUtils.ExtractObjectDetections(tags);

// Via CaptureResult (lazy, com cache)
var result = await client.CaptureSnapshotAsync();
Console.WriteLine(result.Comment);       // string crua
Console.WriteLine(result.CommentTags);   // IReadOnlyDictionary
```

### Go

```go
import "github.com/pumatronix/itscam-sdk/src/wrappers/go/itscam"

// Funções de pacote
comment := itscam.ExtractJpegComment(jpegData)
tags := itscam.ParseJpegCommentTags(comment)
plates := itscam.ExtractPlateRecognitions(tags)
objects := itscam.ExtractObjectDetections(tags)

// One-shot
meta := itscam.ParseJpegMetadata(jpegData)

// Via CaptureResult
result, _ := client.CaptureSnapshot(nil, 10000)
fmt.Println(result[0].Comment())
fmt.Println(result[0].CommentTags())
```

## Quando o comment é populado

O COM marker é escrito pela câmera **somente quando a opção `embedComments` está habilitada** no IMGPKG (que é o default). Na API do binary client, o `CaptureSubscriptionConfig` expõe essa flag:

```cpp
CaptureSubscriptionConfig cfg;
cfg.embedComments = true;   // default
```

Se `embedComments` for `false`, o JPEG não terá COM marker e as funções de extração retornarão string vazia / mapa vazio.

## Fallback: JPEG comment vs. protocolo

O binary TCP client (porta 60000) pode entregar metadados de duas formas:

1. **Via protocolo IMGPKG** -- os campos `FrameInfo.plates` e `FrameInfo.objects` são preenchidos diretamente pelo parser do protocolo binário. Este é o caminho preferido.
2. **Via JPEG COM marker** -- quando o firmware é mais antigo e não envia metadados no protocolo, `populateFromJpegComment()` (C++) pode ser chamado como fallback para preencher os mesmos campos a partir da imagem.

O SDK **não** sobrescreve dados já preenchidos pelo protocolo. A função `populateFromJpegComment()` só atua quando `plates` e `objects` estão vazios.

Para images obtidas via CGI (`snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`), o COM marker é a **única** fonte de metadados de reconhecimento, já que o CGI não tem canal lateral de protocolo.
