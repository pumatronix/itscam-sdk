# Native C++ API

[Português (Brasil)](cpp.md) | [English (US)](cpp.en-US.md)

The native API lives in [`src/core/`](../../src/core/) and is the starting point for every other wrapper (C# / Python / Go) -- they all talk to the same binary through the C ABI in [`src/core/c_api/`](../../src/core/c_api/). If you are consuming the SDK directly from a C++17 application, this is the right guide.

> **Full reference for classes, structs, and functions**: [generated Doxygen reference](/api-ref/cpp/index.html). This page covers integration, build, link, and idiomatic patterns.

> This page covers **integration and idiomatic patterns** for the C++ code. The detailed reference for each client surface (tables of endpoints, request/response types, etc.) lives in `docs/api/`:
>
> - [Binary client (Cougar TCP :60000)](../api/binary-client.md)
> - [REST client (HTTP/HTTPS JSON)](../api/rest-client.md)
> - [CGI client (HTTP/HTTPS multipart)](../api/cgi-client.md)

## Where the code lives

| Location | Contents |
| -------- | -------- |
| [`src/core/itscam_sdk.h`](../../src/core/itscam_sdk.h) | Umbrella header -- single `#include` for the full public surface. |
| [`src/core/itscam_client.h`](../../src/core/itscam_client.h) | `ItscamClient` -- binary TCP (port 60000). |
| [`src/core/itscam_rest_client.h`](../../src/core/itscam_rest_client.h) | `ItscamRestClient` -- HTTP/HTTPS JSON. |
| [`src/core/itscam_cgi_client.h`](../../src/core/itscam_cgi_client.h) | `ItscamCgiClient` -- HTTP/HTTPS for CGI endpoints. |
| [`src/core/itscam_types.h`](../../src/core/itscam_types.h) | `Result<T>`, `Future<T>`, `Error`, `LogLevel`, enums. |
| [`src/core/itscam_sdk_utils.h`](../../src/core/itscam_sdk_utils.h) | Helpers (version, conversions). |
| [`src/core/3rdparty/`](../../src/core/3rdparty/) | Vendored dependencies (cpp-httplib, nlohmann/json, mbedTLS). |
| [`src/examples/`](../../src/examples/) | Four standalone C++ programs. |

## Integration with the pre-compiled SDK (recommended)

The distribution package (`itscam-sdk-<version>.tar.gz`) already includes headers and shared libraries ready to use. Download from the [releases page](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
export SDK=$PWD/itscam-sdk-<version>

g++ -std=c++17 \
    -I$SDK/linux-x64/cpp/include \
    -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L$SDK/linux-x64/cpp/lib -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN' \
    -o your_app

# Copy the .so next to your binary:
cp $SDK/linux-x64/cpp/lib/libitscam_sdk.so* ./
```

| File | Platform |
| ---- | -------- |
| `libitscam_sdk.so.1.0.0` (+ soname + symlink) | Linux shared |
| `itscam_sdk.dll` + `libitscam_sdk.a` | Windows |

Zero system dependencies for TLS: **mbedTLS 3.6 LTS** is statically linked. See [`docs/https-tls.md`](../https-tls.md).

## Building from source (advanced)

If you need to build the SDK from scratch (contributors, cross-compile, debug):

```bash
git clone https://github.com/pumatronix/itscam-sdk.git && cd itscam-sdk
make lib            # libitscam_sdk.{so,a} -> src/core/build/linux/
make windows        # itscam_sdk.dll (cross MinGW) -> src/core/build/windows/
make examples       # C++ binaries under src/examples/build/
make docker-all     # everything inside the builder image (recommended)
```

To link against the source tree, use the headers under `src/core/` and artefacts under `src/core/build/linux/`:

```bash
g++ -std=c++17 -Isrc/core/ -Isrc/core/3rdparty/ -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L./src/core/build/linux/ -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN:./src/core/build/linux' \
    -o your_app
```

See [getting-started.md](../getting-started.en-US.md) for the full build / link / Docker walkthrough.

## C++ idioms

| Pattern | API | Notes |
| ------- | --- | ----- |
| Result type | `Result<T>` in [`itscam_types.h`](../../src/core/itscam_types.h) | `bool` operator + `.value()` / `.error()`. Always check before using. |
| Async | `Future<T>` | `.get()`, `.get(ms)`, `.isReady()`, `waitAll()`. |
| Errors | `Error::Code` enum | Mapped to each wrapper's error types. See [error-handling.md](../error-handling.md). |
| Lifetime | All clients are **move-only** (PIMPL) | Do not copy -- `std::move` when transferring ownership. |
| Threading | Callbacks run on the SDK worker thread | **Do not block**: queue the payload and return. |
| Logging | `client.setLogHandler([](LogLevel lvl, const std::string& msg) {...})` | Per client; default = silent. |
| HTTPS | `setBaseUrl(host, 443, "https")` + `setCaCertFile()` or `setCaCertData()` | Vendored mbedTLS. |

### Result and Future

```cpp
#include "itscam_sdk.h"

itscam::ItscamCgiClient cgi;
cgi.setBaseUrl("192.168.254.254", 80);

auto last = cgi.getLastFrame();
if (!last) {
    std::cerr << "failed: " << last.error().message << "\n";
    return 1;
}
const auto& img = last.value();   // CgiImage { mimeType, data }
```

```cpp
auto future = cgi.getSnapshotAsync(snapReq);
// ... other work ...
if (auto images = future.get(std::chrono::seconds(15))) {
    // images.value() is std::vector<CgiImage>
}
```

## Quick usage per client

### CGI (auth optional)

```cpp
#include "itscam_sdk.h"
#include <fstream>

using namespace itscam;
ItscamCgiClient cgi;
cgi.setBaseUrl("192.168.254.254", 80);
// cgi.setBaseUrl("camera.example.com", 443, "https");
// cgi.setVerifyServerCertificate(false);   // demo / dev only
// cgi.login("admin", "1234");              // only when blockAPI=true

auto last = cgi.getLastFrame().value();
std::ofstream("lastframe.jpg", std::ios::binary)
    .write(reinterpret_cast<const char*>(last.data.data()),
           static_cast<std::streamsize>(last.data.size()));

SnapshotCgiRequest req;
req.quality = 80;
auto images = cgi.getSnapshot(req).value();
// images.size() > 1 when multi-exposure is active

cgi.startMjpegStream([](const CgiStreamFrame& f) {
    // Runs on the SDK worker thread. Do not block.
});
std::this_thread::sleep_for(std::chrono::seconds(5));
cgi.stopMjpegStream();
```

### REST (auth required)

```cpp
ItscamRestClient rest;
rest.setBaseUrl("192.168.254.254", 80);
rest.login("admin", "1234");

auto profiles = rest.getProfiles().value();   // typed POCO

nlohmann::json patch = {{"trigger", {{"enabled", false}}}};
rest.patchJson("/api/image/profiles/0", patch);

auto raw = rest.httpGet("/api/equipment/misc/readonly/volatile").value();
```

Full detail (typed helpers, partial PUT semantics and when to use `httpGet` / `httpPut`) in [docs/api/rest-client.md](../api/rest-client.md).

### Binary (Cougar TCP :60000)

```cpp
ItscamClient camera;
camera.connect("192.168.254.254");
camera.authenticate("1234");
camera.subscribeCaptures();

auto result = camera.captureSnapshot();
if (result) {
    const auto& shots = result.value();   // std::vector<CaptureResult>
    // shots[i].jpeg  -- JPEG bytes
    // shots[i].info  -- frame metadata
}

camera.onTriggerImage([](const TriggerImage& t) {
    // Runs on the SDK worker thread. Do not block.
});
```

Auto-reconnect, exposure groups and GPIO/serial events are documented in [docs/api/binary-client.md](../api/binary-client.md).

## Examples

Under [`src/examples/`](../../src/examples/):

| Program | Demonstrates |
| ------- | ------------ |
| [`itscam_sdk_example.cpp`](../../src/examples/itscam_sdk_example.cpp) | Binary client connect + capture + events. |
| [`itscam_rest_example.cpp`](../../src/examples/itscam_rest_example.cpp) | REST login + read configuration. |
| [`itscam_cgi_example.cpp`](../../src/examples/itscam_cgi_example.cpp) | CGI lastframe + snapshot + MJPEG (auth optional). |
| [`itscam_trigger_recorder.cpp`](../../src/examples/itscam_trigger_recorder.cpp) | Trigger-burst recorder to disk (binary client). |

Build with `make examples`; binaries in `src/examples/build/`.

## Step-by-step tutorial

To create a C++ project from scratch and save the first image to disk, see [First image with C++](../tutorials/first-image-cpp.md).
