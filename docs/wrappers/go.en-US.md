# Go wrapper

[Português (Brasil)](go.md) | [English (US)](go.en-US.md)

The Go wrapper lives at
[`src/wrappers/go/`](../../src/wrappers/go/) and uses **cgo** against
the SDK's C API.  A `static` build tag toggles between dynamic linking
(default; relies on `libitscam_sdk.so` at runtime) and full static
linking (drops in the static archive plus pthread / stdc++ / m).

## Build & run

```bash
make lib                                  # build libitscam_sdk.so / .a
make go-example                           # binary client example
make go-rest-example                      # REST example
make go-cgi-example                       # CGI example

# Dynamic linking at runtime:
LD_LIBRARY_PATH=$PWD/src/core/build/linux \
    ./src/wrappers/go/examples/cgi_snapshot_example 192.168.254.254
```

Static linking:

```bash
cd src/wrappers/go/examples
CGO_CFLAGS="-I$PWD/../../../core" \
go build -tags static -o cgi_snapshot_example cgi_snapshot_example.go
./cgi_snapshot_example 192.168.254.254
```

The cgo `#cgo` directives in
[`src/wrappers/go/itscam/*.go`](../../src/wrappers/go/itscam/) already
know where the static archives live (`../../../core/build/<platform>/`)
relative to the package, so no extra flags are needed beyond
`CGO_CFLAGS` for the headers.

## Surface

```go
import "github.com/pumatronix/itscam-sdk-go/itscam"

// Three clients, all matching the C++ surface 1:1:
client, _ := itscam.NewClient()
rest,   _ := itscam.NewRestClient()
cgi,    _ := itscam.NewCgiClient()
```

Errors are returned as Go `error` values.  The underlying type carries
the SDK's `Error::Code`, so callers can map specific failure modes:

```go
if err := cgi.Login("admin", "1234", 10000); err != nil {
    if errors.Is(err, itscam.ErrTimeout) { /* ... */ }
}
```

## CGI usage (auth optional)

```go
cgi, _ := itscam.NewCgiClient()
defer cgi.Close()

_ = cgi.SetBaseUrl("192.168.254.254", 80, "http")
// cgi.SetBaseUrl("camera.example.com", 443, "https")
// cgi.SetVerifyServerCertificate(false)
// cgi.Login("admin", "1234", 10000)   // only when blockAPI=true

last, _ := cgi.GetLastFrame(10000)
_ = os.WriteFile("lastframe.jpg", last.Data, 0644)

req := itscam.NewSnapshotCgiRequest()
req.Quality = 80
imgs, _ := cgi.GetSnapshot(req, 15000)

_ = cgi.StartMjpegStream(func(f itscam.CgiStreamFrame) {
    // Runs on the SDK worker thread; do not block.
}, 10000)
time.Sleep(5 * time.Second)
cgi.StopMjpegStream()
```

## REST usage (auth required)

Two coexisting surfaces:

* **Typed helpers** (preferred) -- `rest.GetOcrConfig`, `rest.SetOcrConfig`,
  `rest.GetProfiles` etc. return Go structs generated from the camera's
  OpenAPI document.  See [`docs/codegen.md`](../codegen.md) for the
  maintainer / downstream refresh workflow.
* **Generic verbs** (escape hatch) -- `rest.Get`, `rest.Put`, `rest.Post`,
  `rest.Delete` return the raw JSON response body as a string.

* **Partial PUT** -- `rest.PatchJSON(path, patch)` sends only the fields
  being changed.  Required for image profiles and recommended for most
  configuration updates.  See [`docs/api/rest-client.md`](../api/rest-client.md).

```go
rest, _ := itscam.NewRestClient()
defer rest.Close()

_ = rest.SetBaseUrl("192.168.254.254", 80, "http")
_, _ = rest.Login("admin", "1234", 10000)

// Read-only: typed struct is convenient.
profiles, _ := rest.GetProfiles(10000)

// Write: partial PUT (do not round-trip the full profile object).
_, _ = rest.PatchJSON("/api/image/profiles/0", map[string]interface{}{
    "trigger": map[string]interface{}{"enabled": false},
}, 10000)

// Generic verbs:
body, _ := rest.Get("/api/equipment/misc/readonly/constants", 10000)
fmt.Println(body)
```

The generated structs live in
[`src/wrappers/go/itscam/rest_types.go`](../../src/wrappers/go/itscam/rest_types.go);
optional fields are modelled as pointers so `nil` round-trips
cleanly via `encoding/json`.

## Examples

Under [`src/wrappers/go/examples/`](../../src/wrappers/go/examples/):

| Program                       | Demonstrates                                          |
| ----------------------------- | ----------------------------------------------------- |
| `capture_example.go`          | Binary client connect + capture loop.                 |
| `rest_example.go`             | REST login + read configuration.                      |
| `cgi_snapshot_example.go`     | CGI lastframe + snapshot + MJPEG (auth optional).     |
| `gui/`                        | Wails-based desktop viewer; see `gui/README.md`.      |

## Wails GUI example

The `gui/` example is a static Wails application that exercises the SDK
from a WebView UI.  Build with:

```bash
make go-gui                # Linux
make go-gui-windows        # Windows cross-compile
make docker-go-gui         # inside the SDK builder image
```

See [`src/wrappers/go/examples/gui/README.md`](../../src/wrappers/go/examples/gui/README.md)
for prerequisites (Go 1.21+, Wails CLI, WebKit2GTK).
