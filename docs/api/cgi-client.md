# `ItscamCgiClient` -- CGI / multipart API

The CGI client targets the legacy camera-daemon CGI endpoints that the
webapp backend proxies under `/api/*.cgi`.  It is the recommended way
to obtain images over HTTP / HTTPS without taking on the binary client.

Header: [`src/core/itscam_cgi_client.h`](../../src/core/itscam_cgi_client.h).
C++ example: [`src/examples/itscam_cgi_example.cpp`](../../src/examples/itscam_cgi_example.cpp).

## Quick start

```cpp
#include "itscam_sdk.h"
#include <fstream>

int main() {
    using namespace itscam;
    ItscamCgiClient cgi;
    cgi.setBaseUrl("192.168.254.254", 80);

    // Authentication is optional on the camera's defaults -- see below.

    auto last = cgi.getLastFrame();
    std::ofstream("lastframe.jpg", std::ios::binary)
        .write(reinterpret_cast<const char*>(last.value().data.data()),
               static_cast<std::streamsize>(last.value().data.size()));

    SnapshotCgiRequest req; req.quality = 80;
    auto images = cgi.getSnapshot(req).value();
    // images.size() > 1 when multi-exposure is active (one per step,
    // unless mosaic = true).

    cgi.startMjpegStream([](const CgiStreamFrame& f) {
        // Called on the SDK's worker thread.  Do not block.
        // f.data is the encoded JPEG payload.
    });
    std::this_thread::sleep_for(std::chrono::seconds(5));
    cgi.stopMjpegStream();
}
```

## Authentication is optional

The camera daemon's CGI proxy is gated by the `configCgi.blockAPI`
Redis flag in the webapp backend.  It **defaults to `false`** on every
shipped camera, which means anonymous requests are accepted.  Only call
the credential helpers when the operator has explicitly opted in:

```cpp
cgi.login("admin", "1234");                // POST /api/auth, store JWT
cgi.setAuthToken("eyJ...");                // pre-existing JWT
cgi.setBasicAuth("admin", "1234");         // HTTP Basic
cgi.clearAuthToken();
cgi.clearBasicAuth();
```

The four example programs (C++, Python, Go, C#) all run anonymously by
default and accept opt-in `--user` / `--password` flags.

## Endpoints

| Method                                            | Endpoint                | Notes                                                          |
| ------------------------------------------------- | ----------------------- | -------------------------------------------------------------- |
| `getLastFrame(timeoutMs = 10000)`                 | `GET /api/lastframe.cgi`| Returns the most recent preview JPEG.                           |
| `getSnapshot(SnapshotCgiRequest&, timeoutMs)`     | `GET /api/snapshot.cgi` | Trigger + capture.  Multipart on multi-exposure.                |
| `getSnapshotAsync(SnapshotCgiRequest&, timeoutMs)`| `GET /api/snapshot.cgi` | Same but returns a `Future`.                                    |
| `startMjpegStream(callback, timeoutMs)`           | `GET /api/mjpegvideo.cgi`| Continuous `multipart/x-mixed-replace` stream.                  |
| `stopMjpegStream()`                               | (cancel worker)         | Joins the worker thread.                                        |
| `isMjpegStreamRunning()`                          | --                      |                                                                |
| `forceTrigger(timeoutMs = 10000)`                 | `GET /api/trigger.cgi?force=1` | One-shot trigger.                                       |
| `reboot(timeoutMs = 10000)`                       | `GET /api/reboot.cgi`   | Restart the camera-daemon process.                              |
| `httpGetRaw(path, headers, timeoutMs)`            | `GET <path>`            | Generic escape hatch returning `Result<CgiResponse>`.           |

> `trigger.cgi` is a non-standard streaming protocol (back-to-back HTTP
> responses on one socket) that cpp-httplib cannot parse continuously.
> Use `ItscamClient::onTriggerImage` for live trigger events.

## `SnapshotCgiRequest`

Mirrors the URL parameters accepted by the daemon's `snapshot.cgi`
plugin:

```cpp
struct SnapshotCgiRequest {
    std::vector<int> shutters;                       // -> s=
    std::vector<int> gains;                          // -> g=
    int              quality   = -1;                 // -> q=, -1 = server default
    bool             mosaic    = false;              // -> m=1
    std::string      format;                         // -> fmt= ("", "png")
    int              scenario  = -1;                 // -> c=
    std::string      crop;                           // -> r= "x0,y0,x1,y1"
    std::string      textOverlay;                    // -> t=
    std::map<std::string, std::string> userMetadata; // -> User_<key>=<value>
};
```

The return type is **always** `std::vector<CgiImage>`:

| Server response                              | Vector size                              |
| -------------------------------------------- | ---------------------------------------- |
| `image/jpeg` (single exposure)               | 1                                        |
| `image/png` (single exposure, `fmt=png`)     | 1                                        |
| `image/jpeg` mosaic (mosaic = true)          | 1                                        |
| `multipart/related; boundary=snapshot`       | N (one entry per exposure step)          |

`CgiImage` carries `mimeType`, `data` (bytes) and a `headers` map so
applications can recover per-part `Content-Type`, `X-Frame-Index`, etc.

## Streaming MJPEG

`startMjpegStream` spins up a worker thread that decodes the
`multipart/x-mixed-replace; boundary=MjpegBoundary` body frame-by-frame
and invokes the callback for each `CgiStreamFrame`.  Stop with
`stopMjpegStream()`, which cancels the in-flight request and joins the
worker.

```cpp
cgi.startMjpegStream(
    [](const CgiStreamFrame& f) {
        // f.data       -- JPEG bytes
        // f.mimeType   -- e.g. "image/jpeg"
        // f.headers    -- per-part headers
        // f.sequence   -- monotonically increasing frame counter
    },
    /*timeoutMs=*/ 10000);
```

The callback runs on the SDK's worker thread; offload heavy work
(decoding, disk I/O) to your own queue.

## Logging

```cpp
cgi.setLogHandler([](LogLevel lvl, const std::string& msg) {
    std::cout << "[CGI:" << (lvl == LogLevel::Error ? "ERR " : "INFO")
              << "] " << msg << '\n';
});
```

See [`docs/error-handling.md`](../error-handling.md) for the shared
`Result<T>` / `Error` model and [`docs/https-tls.md`](../https-tls.md)
for HTTPS configuration.
