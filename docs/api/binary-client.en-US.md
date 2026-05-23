# `ItscamClient` -- binary TCP API

[Português (Brasil)](binary-client.md) | [English (US)](binary-client.en-US.md)

The binary client speaks the Cougar protocol on TCP port **60000**. It is the lowest-latency surface and the only one that exposes real-time events (triggers, GPIO, serial, multi-exposure groups).

Header: [`src/core/itscam_client.h`](../../src/core/itscam_client.h). C++ example: [`src/examples/itscam_sdk_example.cpp`](../../src/examples/itscam_sdk_example.cpp).

> **Full per-method reference** (signatures, parameters, overloads): see the [generated Doxygen reference](/api-ref/cpp/classitscam_1_1ItscamClient.html). This page focuses on usage patterns, typical workflows, and gotchas; the API reference is generated from the header and cannot drift.

## Quick start

```cpp
#include "itscam_sdk.h"
#include <iostream>

int main() {
    using namespace itscam;
    ItscamClient camera;

    if (!camera.connect("192.168.254.254"))  return 1;
    if (!camera.authenticate("1234"))        return 1;

    camera.subscribeCaptures();

    auto result = camera.captureSnapshot();
    if (result) {
        // result.value() is std::vector<CaptureResult>:
        //   result.value()[i].jpeg  -- JPEG bytes
        //   result.value()[i].info  -- frame metadata
    }
}
```

## Connection & authentication

```cpp
Result<void> connect(address, port = 60000, timeoutMs = 5000);
Result<void> authenticate(password, timeoutMs = 10000);
void         disconnect();
bool         isConnected();
```

### Auto-reconnect

```cpp
AutoReconnectConfig reconnect;
reconnect.enabled    = true;
reconnect.intervalMs = 3000;
reconnect.maxRetries = 0;          // 0 = unlimited
camera.connect(address, 60000, 5000, reconnect);

camera.onConnectionStateChanged(
    [](ConnectionState state, const std::string& reason) {
        // Connected, Disconnected, Reconnecting, Reconnected
    });
```

On reconnect the SDK fully restores the previous session (re-authenticate, re-apply JPEG config, re-subscribe to events).

## Event subscriptions

```cpp
auto ev = EventSubscription::captureResults();
ev.gpio    = true;
ev.serial1 = true;
camera.subscribe(ev);

// Shortcut for the typical "trigger/snapshot images plus metadata" case:
camera.subscribeCaptures();
```

## Capture

`captureSnapshot()` is *trigger + wait*. It returns `Result<std::vector<CaptureResult>>`; one entry per exposure step.

```cpp
SnapshotRequest req;
req.overlays["TextOverlay"] = "Speed: 80km/h";
auto result = camera.captureSnapshot(req);

if (result) {
    auto& frames = result.value();
    for (size_t i = 0; i < frames.size(); ++i) {
        // frames[i].jpeg, frames[i].info.multiExpIndex, ...
    }
    // Detect partial groups on timeout:
    if (!frames.empty() &&
        (int)frames.size() < frames[0].info.multiExpLength) {
        // Not all exposures arrived within the timeout
    }
}

// Just grab the latest frame (no trigger)
auto frame = camera.getLastFrame(/*quality=*/80);
```

## Multi-exposure configuration

```cpp
auto meCfg = camera.getMultiExposureConfig();

MultiExposureConfig multiExp;
multiExp.enabled = true;

MultiExpStep step1; step1.shutterPercent = 100; step1.gainPercent = 100;
MultiExpStep step2; step2.shutterPercent =  50; step2.gainPercent = 100;
multiExp.steps = {step1, step2};

camera.setMultiExposureConfig(multiExp);          // active profile
camera.setMultiExposureConfig(multiExp, 0);       // profile 0

// Disable: one no-op step
MultiExposureConfig disable;
disable.enabled = false;
disable.steps   = {MultiExpStep()};
camera.setMultiExposureConfig(disable);
```

### Exposure groups

When multi-exposure is active the camera produces multiple frames per trigger event, all sharing the same RID.

- **Per-frame callbacks** (`onTriggerImage`, `onSnapshotImage`) fire once for each individual frame.
- **Group callbacks** fire once per complete group; partial groups are delivered after a timeout.

```cpp
camera.onTriggerImage([](const CaptureResult& cr) {
    // cr.info.multiExpIndex, cr.info.multiExpLength
});

camera.onTriggerExposureGroup([](const std::vector<CaptureResult>& group) {
    // group.size() == multiExpLength (when complete)
});

camera.onSnapshotExposureGroup([](const std::vector<CaptureResult>& group) {
    // same pattern for snapshot images
});

camera.setExposureGroupTimeout(3000);   // default: 5000 ms
```

## Image configuration

```cpp
JpegConfig cfg = JpegConfig::imgpkgDefaults();
cfg.snapshotQuality       = 95;
cfg.imgpkg.embedSignature = 1;
camera.setJpegConfig(cfg);
```

`JpegConfig::imgpkgDefaults()` enables EXIF and embedded comments by default; signature embedding is opt-in.

## Profiles

The camera supports up to 4 profiles. Each carries its own trigger, exposure and color settings. Profile 0 is the default.

```cpp
auto profiles = camera.listProfiles();
for (auto& p : profiles.value()) {
    std::cout << "Profile " << p.id << " name=" << p.name
              << (p.active ? " [ACTIVE]" : "") << '\n';
}

auto id = camera.getActiveProfileId();   // Result<uint32_t>
camera.setActiveProfile(0);
```

## Trigger & exposure

All trigger / exposure methods take an optional `profileId` argument. Use `CURRENT_PROFILE` (default) to target the active profile.

```cpp
auto trig  = camera.getTriggerConfig();
auto trig0 = camera.getTriggerConfig(0);

TriggerConfig newTrig;
newTrig.enabled = 1;
newTrig.event   = TriggerEvent::EdgeRising;
newTrig.port    = 1;
camera.setTriggerConfig(newTrig);

ExposureConfig exp;
exp.shutter.automatic = 1; exp.shutter.maxValue = 10000;
exp.gain.automatic    = 1; exp.gain.maxValue    = 800;
camera.setExposureConfig(exp, 0);
```

`-1` means "leave unchanged" -- only set the fields you want to modify.

## Serial I/O

```cpp
SerialConfig cfg;
cfg.baudRate = 9600;
cfg.dataBits = 8;
cfg.parity   = 0;
cfg.stopBits = 1;
camera.configureSerial(SerialPort::Serial1, cfg);

camera.sendSerialAscii(SerialPort::Serial1, "AT\r\n");
camera.onSerial([](const SerialData& sd) {
    if (sd.port == SerialPort::Serial1) {
        // process sd.data
    }
});
```

## Typed equipment helpers

```cpp
auto info = camera.getDeviceInfo();     // model, firmware, sensor, size, ...
camera.setScenarioOverlay(1, "Speed: {User_Speed}km/h");
camera.setScenarioCrop(1, {0, 0, 799, 599});
camera.setSnapshotCrop(true, {100, 100, 700, 500});
```

## Generic config (escape hatch)

```cpp
auto cfg = camera.getConfig("equip.autofocus");
camera.setConfig("equip.autofocus.run", 1);
```

## Event callbacks

```cpp
camera.onTriggerImage      ([](const CaptureResult& cr) { /* ... */ });
camera.onSnapshotImage     ([](const CaptureResult& cr) { /* ... */ });
camera.onPreviewImage      ([](const CaptureResult& cr) { /* ... */ });

camera.onTriggerExposureGroup ([](const std::vector<CaptureResult>& g) {});
camera.onSnapshotExposureGroup([](const std::vector<CaptureResult>& g) {});

camera.onTriggerMetadata   ([](const FrameInfo& fi) { /* metadata only */ });
camera.onGpio              ([](const GpioEvent& ev) { /* ... */ });
camera.onSerial            ([](const SerialData& sd) { /* ... */ });
camera.onDisconnect        ([](const std::string& reason) { /* ... */ });
camera.onConnectionStateChanged(
    [](ConnectionState s, const std::string& reason) { /* ... */ });
```

All callbacks run on the SDK's worker thread; **do not block** inside them.

## System

```cpp
camera.reboot();
camera.setPingInterval(10);             // seconds
camera.setMaxPingFailures(10);          // 0 = disable
camera.setExposureGroupTimeout(3000);   // ms (default: 5000)
```

## `captureSnapshot()` vs `onSnapshotImage`

`captureSnapshot()` internally registers a pending capture for the RID and returns the full exposure group as `Result<std::vector<CaptureResult>>`. `onSnapshotImage` still fires per-frame and is useful when triggers come from outside your process or when you need bespoke accumulation logic.
