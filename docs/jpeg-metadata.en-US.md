# JPEG metadata (COM marker)

[Português (Brasil)](jpeg-metadata.md) | [English (US)](jpeg-metadata.en-US.md)

ITSCAM cameras embed plate-recognition and vehicle-classification results directly into the JPEG image bytes using the standard JFIF **COM marker** (0xFF 0xFE). This makes every image self-contained: metadata travels with the file and is never lost, even when the image is copied, transferred via FTP, or stored on disk without any sidecar.

The SDK provides utilities to extract and parse this metadata in every supported language.

## Comment format

The COM marker body is an ASCII string of semicolon-delimited `key=value` pairs:

```
Placa=ABC1234;CoordPlaca=100x200,150x40;CorPlaca=0;OCRCountryCode=55;ClassifierList=[1,95,50,100,300,200];BMCList=["VW",90,"Gol",85,"Branco",92]
```

### Plate-recognition tags

| Tag              | Format                      | Description                                                       |
| ---------------- | --------------------------- | ----------------------------------------------------------------- |
| `Placa`          | `ABC1234` or `ABC_DEF`      | Plate text. Multiple plates separated by `_`.                     |
| `CoordPlaca`     | `XxY,WxH` (`_`-separated)  | Bounding box of each plate in the image.                          |
| `CorPlaca`       | `0` or `1` (`_`-separated)  | Plate color (`0` = white on black, `1` = black on white).        |
| `OCRCountryCode` | integer (`_`-separated)     | Country code for each plate.                                      |

### Vehicle-classification tags

| Tag              | Format                                        | Description                                             |
| ---------------- | --------------------------------------------- | ------------------------------------------------------- |
| `ClassifierList` | `[type,prob,x,y,w,h],[...]`                   | Detections. `prob` is 0–100 (percentage).               |
| `BMCList`        | `["brand",bP,"model",mP,"color",cP],[...]`    | Brand/Model/Color. Probabilities are 0–100.             |

## API by language

### C++ (header-only)

```cpp
#include "itscam_jpeg_utils.h"

// Extract raw comment
std::string comment = itscam::extractJpegComment(jpeg.data(), jpeg.size());

// Parse into a key=value map
auto tags = itscam::parseJpegCommentTags(comment);

// Extract structured results
auto plates  = itscam::extractPlateRecognitions(tags);
auto objects = itscam::extractObjectDetections(tags);

// One-shot: populate a CaptureResult from JPEG COM
itscam::populateFromJpegComment(result);
```

All functions are `inline` and defined in [`src/core/itscam_jpeg_utils.h`](../src/core/itscam_jpeg_utils.h). No additional library linkage required.

### C API

```c
#include "itscam_sdk_c.h"

// Query length, then extract
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

# From raw JPEG bytes
comment = extract_jpeg_comment(jpeg_bytes)
tags = parse_jpeg_comment_tags(comment)

# One-shot with structured results
meta = parse_jpeg_metadata(jpeg_bytes)
for plate in meta.plates:
    print(f"Plate: {plate.plate} at ({plate.x},{plate.y})")
for obj in meta.objects:
    print(f"Vehicle type {obj.type} ({obj.probability:.0%})")

# Via CaptureResult (lazy, cached)
result = client.capture_snapshot()[0]
print(result.comment)       # raw string
print(result.comment_tags)  # dict
```

### C# / .NET

```csharp
using Pumatronix.Itscam;

// Static helpers
string comment = JpegUtils.ExtractComment(jpegBytes);
var tags = JpegUtils.ParseCommentTags(comment);
var plates = JpegUtils.ExtractPlateRecognitions(tags);
var objects = JpegUtils.ExtractObjectDetections(tags);

// Via CaptureResult (lazy, cached)
var result = await client.CaptureSnapshotAsync();
Console.WriteLine(result.Comment);       // raw string
Console.WriteLine(result.CommentTags);   // IReadOnlyDictionary
```

### Go

```go
import "github.com/pumatronix/itscam-sdk/src/wrappers/go/itscam"

// Package-level functions
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

## When the comment is populated

The COM marker is written by the camera **only when `embedComments` is enabled** in IMGPKG (which is the default). In the binary client API, `CaptureSubscriptionConfig` exposes this flag:

```cpp
CaptureSubscriptionConfig cfg;
cfg.embedComments = true;   // default
```

If `embedComments` is `false`, the JPEG will have no COM marker and the extraction functions will return an empty string / map.

## Fallback: JPEG comment vs. protocol

The binary TCP client (port 60000) can deliver metadata in two ways:

1. **Via IMGPKG protocol** -- the `FrameInfo.plates` and `FrameInfo.objects` fields are populated directly by the binary protocol parser. This is the preferred path.
2. **Via JPEG COM marker** -- when running on older firmware that does not send metadata in the protocol, `populateFromJpegComment()` (C++) can be called as a fallback to populate the same fields from the image.

The SDK **does not** overwrite data already provided by the protocol. `populateFromJpegComment()` only acts when `plates` and `objects` are both empty.

For images obtained via CGI (`snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`), the COM marker is the **only** source of recognition metadata since CGI has no side-channel protocol.
