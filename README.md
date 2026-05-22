# ITSCAM Client SDK

Cross-platform C++14 client library for connecting to ITSCAM cameras.
Supports Linux and Windows with language bindings for Python and Go.

## Platform Support

- **Linux**: x86_64, ARMv7, ARM64 (native build)
- **Windows**: x86_64 (MinGW cross-compile)

## Directory Structure

```
sdk/
├── Makefile              # Top-level build orchestrator
├── README.md
├── core/                 # C/C++ core library
│   ├── Makefile
│   ├── itscam_client.h/.cpp
│   ├── itscam_rest_client.h/.cpp
│   ├── itscam_sdk_c.h/.cpp    # C API wrapper
│   ├── itscam_sdk_utils.h
│   └── impl/                   # Platform-specific implementations
├── examples/             # C++ example applications
│   ├── Makefile
│   ├── itscam_sdk_example.cpp
│   ├── itscam_rest_example.cpp
│   └── itscam_trigger_recorder.cpp
└── wrappers/
    ├── python/           # Python binding (ctypes)
    │   ├── itscam/
    │   └── examples/
    └── go/               # Go binding (cgo)
        ├── itscam/
        └── examples/
```

## Building

```bash
# Build core library and C++ examples (Linux)
make

# Build core library only
make lib

# Cross-compile for Windows
make windows

# Build everything (all platforms + wrapper examples)
make all

# Show all available targets
make help
```

## Language Bindings

| Language | Location | Technology |
|----------|----------|------------|
| Python | `wrappers/python/` | ctypes FFI |
| Go | `wrappers/go/` | cgo |

The SDK provides two client classes:

- **`ItscamClient`** -- Binary TCP protocol (Cougar) for real-time camera
  control: capture, events, streaming, serial I/O, trigger/exposure
  configuration.
- **`ItscamRestClient`** -- HTTP/JSON client for the webapp backend REST API:
  equipment configuration, image profiles, analytics, OCR, classifier,
  lanes, ITSCAM PRO server management.

## Quick Start -- ItscamClient (Binary TCP)

```cpp
#include "itscam_sdk.h"
#include <iostream>

int main() {
    using namespace itscam;
    ItscamClient camera;

    if (!camera.connect("192.168.254.254"))  return 1;
    if (!camera.authenticate("1234"))        return 1;

    // Subscribe to capture events using IMGPKG defaults.
    camera.subscribeCaptures();

    // Capture a snapshot (trigger + wait for image)
    auto result = camera.captureSnapshot();
    if (result) {
        // result.value().jpeg contains the JPEG data
        // result.value().info  contains frame metadata
    }
}
```

## Quick Start -- ItscamRestClient (HTTP/JSON)

```cpp
#include "itscam_sdk.h"
#include <iostream>

int main() {
    using namespace itscam;
    ItscamRestClient rest;

    rest.setBaseUrl("192.168.254.254", 80);

    // Authenticate (JWT)
    if (!rest.login("admin", "1234")) return 1;

    // Read image profiles
    auto profiles = rest.getProfiles();
    if (profiles)
        std::cout << profiles.value().dump(2) << std::endl;

    // Read volatile equipment info
    auto vol = rest.getVolatileInfo();

    // Read / write equipment configs
    auto ocr = rest.getOcrConfig();
    auto cls = rest.getClassifierConfig();

    // Generic escape hatch for unsupported endpoints
    auto custom = rest.httpGet("/api/some/other/endpoint");
}
```

## Building

### Shared library (recommended)

The examples Makefile builds the SDK as a shared library and links examples
against it.  Dependencies are downloaded automatically on first build:

```bash
cd sdk/examples/
make            # builds libitscam_sdk.so + all example binaries
make lib        # builds the shared library only

# Run an example (rpath is set to $ORIGIN so ./libitscam_sdk.so is found)
./itscam_sdk_example 192.168.254.254
./itscam_rest_example 192.168.254.254
```

The shared library is versioned:

| File                      | Purpose           |
|---------------------------|-------------------|
| `libitscam_sdk.so.1.0.0`  | Real library      |
| `libitscam_sdk.so.1`      | Soname symlink    |
| `libitscam_sdk.so`        | Linker symlink    |

### Linking your own application

```bash
# Compile (headers only, no SDK sources)
g++ -std=c++14 -Isdk/ -I<deps>/ -c your_app.cpp -o your_app.o

# Link against the shared library
g++ your_app.o -L<path-to-lib>/ -litscam_sdk -Wl,-rpath,'$ORIGIN' -o your_app
```

### Static compilation (alternative)

If you prefer a self-contained binary, compile the SDK sources directly:

```bash
g++ -std=c++14 \
    -Isdk/ \
    -I<path-to-nlohmann-json>/single_include/ \
    -I<path-to-cpp-httplib>/ \
    sdk/itscam_client.cpp \
    sdk/itscam_rest_client.cpp \
    your_app.cpp \
    -o your_app -lpthread
```

### Dependencies

- **C++14** compiler (GCC 5+, Clang 3.4+)
- **nlohmann/json** (header-only, single include)
- **POSIX sockets** (Linux) -- required by ItscamClient
- **cpp-httplib** v0.18+ (header-only, HTTP only) -- required by ItscamRestClient

## API Overview

### Error Handling: `Result<T>`

All synchronous methods return `Result<T>`. Use `operator bool()` to check
success, `.value()` to access the payload, `.error()` to get the error info.

```cpp
auto r = camera.authenticate("1234");
if (!r) {
    std::cerr << r.error().message << std::endl;
    return 1;
}
```

### Asynchronous Methods: `Future<T>`

Long-running operations have `*Async()` variants that return `Future<T>`:

```cpp
auto f = camera.captureSnapshotAsync(req);
// ... do other work ...
auto r = f.get();          // block until ready
auto r = f.get(5000);      // block up to 5 seconds
bool ready = f.isReady();  // non-blocking check
```

Use `waitAll()` to wait for multiple futures:

```cpp
auto f1 = camera.captureSnapshotAsync(req1);
auto f2 = camera.captureSnapshotAsync(req2);
if (!waitAll(f1, f2)) { /* at least one failed */ }
```

### Connection & Authentication

```cpp
// Basic connection
Result<void> connect(address, port = 60000, timeoutMs = 5000);

// Connection with auto-reconnect
AutoReconnectConfig reconnect;
reconnect.enabled    = true;
reconnect.intervalMs = 3000;   // retry every 3 seconds
reconnect.maxRetries = 0;      // 0 = unlimited
camera.connect(address, 60000, 5000, reconnect);

Result<void> authenticate(password, timeoutMs = 10000);
void disconnect();
bool isConnected();
```

### Auto-Reconnection

When `AutoReconnectConfig.enabled` is `true`, the SDK automatically attempts
to reconnect after an unexpected disconnection. On successful reconnect, the
previous session is fully restored (re-authenticate, re-apply JPEG config,
re-subscribe to events).

Monitor connection lifecycle with `onConnectionStateChanged`:

```cpp
camera.onConnectionStateChanged([](ConnectionState state, const std::string& reason) {
    switch (state) {
        case ConnectionState::Connected:    // initial connection
        case ConnectionState::Disconnected: // connection lost
        case ConnectionState::Reconnecting: // retry attempt starting
        case ConnectionState::Reconnected:  // reconnected + session restored
    }
});
```

Call `disconnect()` to stop auto-reconnection and cleanly shut down.
The `onDisconnect` callback is still supported as a convenience for
handling the `Disconnected` state only.

### Event Subscriptions

```cpp
auto ev = EventSubscription::captureResults();
ev.gpio    = true;
ev.serial1 = true;
camera.subscribe(ev);
```

For the common “trigger/snapshot images plus matching metadata” case, prefer:

```cpp
camera.subscribeCaptures();
```

### Image Configuration

```cpp
JpegConfig cfg = JpegConfig::imgpkgDefaults();
cfg.snapshotQuality       = 95;
cfg.imgpkg.embedSignature = 1;
camera.setJpegConfig(cfg);
```

`JpegConfig::imgpkgDefaults()` enables EXIF and embedded comments by default,
and leaves signature embedding opt-in.

### Capture

`captureSnapshot()` returns a `Result<std::vector<CaptureResult>>`.  For
single-exposure configurations the vector has one element; for multi-exposure
each element corresponds to one exposure step.

```cpp
// Combined trigger + wait for all exposure group frames
SnapshotRequest req;
req.overlays["TextOverlay"] = "Speed: 80km/h";
auto result = camera.captureSnapshot(req);
if (result) {
    auto& frames = result.value();
    for (size_t i = 0; i < frames.size(); ++i) {
        // frames[i].jpeg   -- JPEG data
        // frames[i].info   -- frame metadata (multiExpIndex, multiExpLength, etc.)
    }
    // Detect partial results on timeout:
    if (!frames.empty() && (int)frames.size() < frames[0].info.multiExpLength) {
        // Not all exposures arrived within the timeout
    }
}

// Just grab the latest frame (no trigger)
auto frame = camera.getLastFrame(80);  // quality = 80
```

### Multi-Exposure Configuration

Configure multiple exposure steps per trigger event using the typed API:

```cpp
// Read current multi-exposure configuration
auto meCfg = camera.getMultiExposureConfig();

// Enable 2-step multi-exposure on the active profile
MultiExposureConfig multiExp;
multiExp.enabled = true;

MultiExpStep step1;
step1.shutterPercent = 100;  // 100% of auto (normal)
step1.gainPercent    = 100;

MultiExpStep step2;
step2.shutterPercent = 50;   // 50% of auto (darker)
step2.gainPercent    = 100;

multiExp.steps = {step1, step2};
camera.setMultiExposureConfig(multiExp);

// Configure for a specific profile
camera.setMultiExposureConfig(multiExp, 0);

// Disable multi-exposure
MultiExposureConfig disable;
disable.enabled = false;
disable.steps = {MultiExpStep()};
camera.setMultiExposureConfig(disable);
```

### Multi-Exposure Groups

When multi-exposure is configured (via `setMultiExposureConfig` or per-snapshot
via `SnapshotRequest.multiExposure`), the camera produces multiple frames per
trigger/snapshot event, each sharing the same RID.

**Per-frame callbacks** (`onTriggerImage`, `onSnapshotImage`) fire once for each
individual exposure frame, regardless of how many exposures are in the group.

**Exposure group callbacks** fire once per complete group (all frames with the
same RID). If a group is incomplete after the internal timeout, partial data is
delivered.

```cpp
// Per-frame callback (fires for each individual exposure)
camera.onTriggerImage([](const CaptureResult& cr) {
    // cr.info.multiExpIndex = 0, 1, 2, ...
    // cr.info.multiExpLength = total exposures in group
});

// Grouped callback (fires once when all frames in the group arrive)
camera.onTriggerExposureGroup([](const std::vector<CaptureResult>& group) {
    // group.size() == multiExpLength  (if complete)
    for (auto& frame : group) { /* process each exposure */ }
});

// For snapshot images
camera.onSnapshotExposureGroup([](const std::vector<CaptureResult>& group) {
    // Same pattern as trigger exposure groups
});

// Tune the internal timeout for accumulating incomplete groups (default: 5000ms)
camera.setExposureGroupTimeout(3000);
```

**`onSnapshotImage` vs `captureSnapshot`**: The `captureSnapshot()` method
internally registers a pending capture for the RID and returns all frames of
the exposure group directly as `Result<std::vector<CaptureResult>>`. The
`onSnapshotImage` per-frame callback still fires independently for each frame
and is useful for monitoring externally-triggered snapshots or implementing
custom accumulation logic.

### Profile Management

The camera supports up to 4 profiles, each with independent trigger, exposure
and color settings. Profiles are identified by numeric IDs (default profile is 0).

```cpp
// List all profiles
auto profiles = camera.listProfiles();
for (auto& p : profiles.value()) {
    std::cout << "Profile " << p.id
              << " name=" << p.name
              << (p.active ? " [ACTIVE]" : "") << std::endl;
}

// Get / switch the active profile
auto id = camera.getActiveProfileId();       // returns Result<uint32_t>
camera.setActiveProfile(0);                  // switch to profile 0
```

### Trigger & Exposure Configuration

All trigger / exposure methods accept an optional `profileId` parameter.
Use `CURRENT_PROFILE` (the default) to target the active profile, or pass
a specific profile ID.

```cpp
// Read trigger config for the active profile (default)
auto trig = camera.getTriggerConfig();

// Read trigger config for a specific profile
auto trig0 = camera.getTriggerConfig(0);

// Enable trigger on port 1 (active profile)
TriggerConfig newTrig;
newTrig.enabled = 1;
newTrig.event = TriggerEvent::EdgeRising;
newTrig.port = 1;
camera.setTriggerConfig(newTrig);

// Set exposure on a specific profile
ExposureConfig exp;
exp.shutter.automatic = 1;
exp.shutter.maxValue = 10000;
exp.gain.automatic = 1;
exp.gain.maxValue = 800;
camera.setExposureConfig(exp, 0);  // target profile 0
```

Values of `-1` mean "unchanged" -- only set the fields you want to modify.

### Serial Communication

```cpp
SerialConfig cfg;
cfg.baudRate = 9600;
cfg.dataBits = 8;
cfg.parity   = 0;  // None
cfg.stopBits = 1;
camera.configureSerial(SerialPort::Serial1, cfg);

camera.sendSerialAscii(SerialPort::Serial1, "AT\r\n");

camera.onSerial([](const SerialData& sd) {
    if (sd.port == SerialPort::Serial1) {
        // process sd.data
    }
});
```

### Typed Equipment Configuration

```cpp
// Device info (read-only)
auto info = camera.getDeviceInfo();
if (info) {
    std::cout << info.value().cameraModel << " "
              << info.value().firmwareVersion << " "
              << info.value().sensorType << " "
              << info.value().imageSize.width << "x"
              << info.value().imageSize.height << std::endl;
}

// Scenario overlay and crop
camera.setScenarioOverlay(1, "Speed: {User_Speed}km/h");
camera.setScenarioCrop(1, {0, 0, 799, 599});
camera.setSnapshotCrop(true, {100, 100, 700, 500});
```

### Generic Configuration (Escape Hatch)

For any configuration path not covered by the typed methods:

```cpp
auto cfg = camera.getConfig("equip.autofocus");
camera.setConfig("equip.autofocus.run", 1);
```

### Event Callbacks

```cpp
// Per-frame image callbacks
camera.onTriggerImage([](const CaptureResult& cr) { /* ... */ });
camera.onSnapshotImage([](const CaptureResult& cr) { /* ... */ });
camera.onPreviewImage([](const CaptureResult& cr) { /* ... */ });

// Exposure group callbacks (complete multi-exposure groups)
camera.onTriggerExposureGroup([](const std::vector<CaptureResult>& group) { /* ... */ });
camera.onSnapshotExposureGroup([](const std::vector<CaptureResult>& group) { /* ... */ });

// Metadata-only and system callbacks
camera.onTriggerMetadata([](const FrameInfo& fi) { /* metadata only */ });
camera.onGpio([](const GpioEvent& ev) { /* ... */ });
camera.onSerial([](const SerialData& sd) { /* ... */ });
camera.onDisconnect([](const std::string& reason) { /* ... */ });
camera.onConnectionStateChanged([](ConnectionState s, const std::string& reason) { /* ... */ });
```

### System

```cpp
camera.reboot();
camera.setPingInterval(10);              // seconds
camera.setMaxPingFailures(10);           // consecutive unanswered pings before reconnect (default: 10, 0 = disable)
camera.setExposureGroupTimeout(3000);    // ms (default: 5000)
```

## ItscamRestClient API Reference

`ItscamRestClient` talks to the ITSCAM webapp backend over HTTP/JSON.
All methods return `Result<nlohmann::json>` (or `void` for setters).
The response JSON matches whatever the backend returns -- schemas are not
enforced by the SDK, so the client is forward-compatible with backend
changes.

### Setup & Authentication

```cpp
ItscamRestClient rest;
rest.setBaseUrl("192.168.254.254", 80);   // host + port
rest.setApiPrefix("/api");                 // default, usually no need to change

// Login: POST /api/auth -> stores JWT token internally
auto loginResult = rest.login("admin", "1234");

// Or set a pre-existing token
rest.setAuthToken("eyJ...");
rest.clearAuthToken();
```

### Image Profiles

```cpp
auto all     = rest.getProfiles();             // GET    /api/image/profiles
auto one     = rest.getProfile(0);             // GET    /api/image/profiles?id=0
auto created = rest.createProfile(json);       // POST   /api/image/profiles
auto updated = rest.updateProfile(json);       // PUT    /api/image/profiles
auto deleted = rest.deleteProfile(0);          // DELETE /api/image/profiles?id=0
```

### Equipment Configuration

```cpp
// Volatile info (read-only)
auto vol = rest.getVolatileInfo();             // GET /api/equipment/misc/readonly/volatile

// General
auto gen = rest.getGeneralConfig();            // GET /api/equipment/general
rest.setGeneralConfig(json);                   // PUT /api/equipment/general

// Analytics
auto ana = rest.getAnalyticsConfig();          // GET /api/equipment/analytics
rest.setAnalyticsConfig(json);                 // PUT /api/equipment/analytics

// OCR
auto ocr = rest.getOcrConfig();                // GET /api/equipment/ocr
rest.setOcrConfig(json);                       // PUT /api/equipment/ocr

// Classifier
auto cls = rest.getClassifierConfig();         // GET /api/equipment/classifier
rest.setClassifierConfig(json);                // PUT /api/equipment/classifier

// Lanes
auto lanes = rest.getLanesConfig();            // GET /api/equipment/lanes
rest.setLanesConfig(json);                     // PUT /api/equipment/lanes

// ITSCAM PRO server
auto pro    = rest.getItscamproConfig();       // GET /api/equipment/servers/itscampro
rest.setItscamproConfig(json);                 // PUT /api/equipment/servers/itscampro
auto status = rest.getItscamproStatus();       // GET /api/equipment/servers/itscampro/status
```

### Generic HTTP Methods (Escape Hatch)

For endpoints not covered by the typed helpers:

```cpp
auto r1 = rest.httpGet("/api/some/endpoint");
auto r2 = rest.httpPut("/api/some/endpoint", json);
auto r3 = rest.httpPost("/api/some/endpoint", json);
auto r4 = rest.httpDelete("/api/some/endpoint");
```

Note: the generic methods use the path **as-is** (no API prefix prepended).

### Error Handling

The REST client reuses the same `Result<T>` and `Error` types as
`ItscamClient`.  HTTP status codes are mapped as follows:

| HTTP Status | `Error::Code`         |
|-------------|-----------------------|
| 401         | `NotAuthenticated`    |
| 400, 422    | `InvalidParameter`    |
| 503         | `ConnectionFailed`    |
| 5xx         | `ServerError`         |
| Timeout     | `Timeout`             |
| Conn. fail  | `ConnectionFailed`    |

```cpp
auto r = rest.getOcrConfig();
if (!r) {
    std::cerr << "Error: " << r.error().message << std::endl;
    if (r.error().code == Error::NotAuthenticated)
        rest.login("admin", "1234");  // re-authenticate
}
```

### Logging

```cpp
rest.setLogHandler([](LogLevel lvl, const std::string& msg) {
    if (lvl == LogLevel::Error)
        std::cerr << "[REST ERROR] " << msg << std::endl;
    else
        std::cout << "[REST] " << msg << std::endl;
});
```

## Migration from CougarClient

| Old (CougarClient)                                | New (ItscamClient)                              |
|----------------------------------------------------|-------------------------------------------------|
| `cougar.start("ip")`                               | `camera.connect("ip")`                          |
| `genericSyncCall(GC_AUTHENTICATE, {{"pass","x"}})` | `camera.authenticate("x")`                      |
| `genericSyncCall(GC_SET_CALLBACKS, json)`           | `camera.subscribe(EventSubscription{...})`      |
| `genericSyncCall(GC_SET_JPEG_CFGS, json)`           | `camera.setJpegConfig(JpegConfig{...})`         |
| `genericSyncCall(GC_TRIGGER_SNAPSHOT, json)`        | `camera.captureSnapshot(SnapshotRequest{...})`  |
| `setMixedCallback(CBMX_IMGPKG_SNAPSHOT, fn)`       | `camera.onSnapshotImage(fn)`                    |
| `genericSyncCall(GC_SET_SERIAL_CFGS, json)`         | `camera.configureSerial(SerialPort::Serial1,.)` |
| `genericSyncCall(GC_SEND_SERIAL_DATA, json)`        | `camera.sendSerialAscii(SerialPort::Serial1,.)` |
| `genericSyncCall(GC_SET_EQUIP_CFGS, json)`          | `camera.setConfig(path, data)` or typed helper  |
| `genericSyncCall(GC_CMD_REBOOT, {})`                | `camera.reboot()`                               |
