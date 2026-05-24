# `ItscamClient` -- binary TCP API

[Português (Brasil)](binary-client.md) | [English (US)](binary-client.en-US.md)

O binary client fala o protocolo Cougar na porta TCP **60000**. É a superfície de menor latência e a única que expõe eventos em real time (triggers, GPIO, serial, multi-exposure groups).

Header: [`src/core/itscam_client.h`](../../src/core/itscam_client.h). Example C++: [`src/examples/itscam_sdk_example.cpp`](../../src/examples/itscam_sdk_example.cpp).

> **Referência completa de cada método** (assinatura, parâmetros, overloads): veja a [referência Doxygen do C++](/api-ref/cpp/classitscam_1_1ItscamClient.html). Esta página foca em padrões de uso, fluxos típicos e gotchas; a referência API é gerada do header e nunca fica fora de sincronia.

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
        // result.value() é std::vector<CaptureResult>:
        //   result.value()[i].jpeg  -- bytes do JPEG
        //   result.value()[i].info  -- metadata do frame
    }
}
```

## Conexão e autenticação

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
reconnect.maxRetries = 0;          // 0 = ilimitado
camera.connect(address, 60000, 5000, reconnect);

camera.onConnectionStateChanged(
    [](ConnectionState state, const std::string& reason) {
        // Connected, Disconnected, Reconnecting, Reconnected
    });
```

No reconnect, o SDK restaura totalmente a session anterior (re-autentica, re-aplica JPEG config, re-subscribe nos eventos).

## Event subscriptions

```cpp
auto ev = EventSubscription::captureResults();
ev.gpio    = true;
ev.serial1 = true;
camera.subscribe(ev);

// Atalho para o caso típico "trigger/snapshot images com metadata":
camera.subscribeCaptures();
```

## Capture

`captureSnapshot()` é *trigger + wait*. Devolve `Result<std::vector<CaptureResult>>`; um entry por exposure step.

```cpp
SnapshotRequest req;
req.overlays["TextOverlay"] = "Speed: 80km/h";
auto result = camera.captureSnapshot(req);

if (result) {
    auto& frames = result.value();
    for (size_t i = 0; i < frames.size(); ++i) {
        // frames[i].jpeg, frames[i].info.multiExpIndex, ...
    }
    // Detecta partial groups em timeout:
    if (!frames.empty() &&
        (int)frames.size() < frames[0].info.multiExpLength) {
        // Nem todas as exposures chegaram dentro do timeout
    }
}

// Pega só o último frame (sem trigger)
auto frame = camera.getLastFrame(/*quality=*/80);
```

## Configuration de multi-exposure

```cpp
auto meCfg = camera.getMultiExposureConfig();

MultiExposureConfig multiExp;
multiExp.enabled = true;

MultiExpStep step1; step1.shutterPercent = 100; step1.gainPercent = 100;
MultiExpStep step2; step2.shutterPercent =  50; step2.gainPercent = 100;
multiExp.steps = {step1, step2};

camera.setMultiExposureConfig(multiExp);          // profile ativo
camera.setMultiExposureConfig(multiExp, 0);       // profile 0

// Desabilita: um step no-op
MultiExposureConfig disable;
disable.enabled = false;
disable.steps   = {MultiExpStep()};
camera.setMultiExposureConfig(disable);
```

### Exposure groups

Quando multi-exposure está ativo, a câmera produz múltiplos frames por trigger, todos compartilhando o mesmo RID.

- **Callbacks por frame** (`onTriggerImage`, `onSnapshotImage`) disparam uma vez para cada frame individual.
- **Callbacks por grupo** disparam uma vez por grupo completo; partial groups são entregues após um timeout.

```cpp
camera.onTriggerImage([](const CaptureResult& cr) {
    // cr.info.multiExpIndex, cr.info.multiExpLength
});

camera.onTriggerExposureGroup([](const std::vector<CaptureResult>& group) {
    // group.size() == multiExpLength (quando completo)
});

camera.onSnapshotExposureGroup([](const std::vector<CaptureResult>& group) {
    // mesmo padrão para snapshot images
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

`JpegConfig::imgpkgDefaults()` habilita EXIF e embedded comments por default; signature embedding é opt-in.

## Profiles

A câmera suporta até 4 profiles. Cada um carrega seus próprios trigger, exposure e color settings. O profile 0 é o default.

```cpp
auto profiles = camera.listProfiles();
for (auto& p : profiles.value()) {
    std::cout << "Profile " << p.id << " name=" << p.name
              << (p.active ? " [ACTIVE]" : "") << '\n';
}

auto id = camera.getActiveProfileId();   // Result<uint32_t>
camera.setActiveProfile(0);
```

## Trigger e exposure

Todos os métodos de trigger / exposure aceitam um argumento opcional `profileId`. Use `CURRENT_PROFILE` (default) para mirar no profile ativo.

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

`-1` significa "deixa como está" -- só preenche os fields que você quer modificar.

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
        // processa sd.data
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

## Config genérica (escape hatch)

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

camera.onTriggerMetadata   ([](const FrameInfo& fi) { /* só metadata */ });
camera.onGpio              ([](const GpioEvent& ev) { /* ... */ });
camera.onSerial            ([](const SerialData& sd) { /* ... */ });
camera.onDisconnect        ([](const std::string& reason) { /* ... */ });
camera.onConnectionStateChanged(
    [](ConnectionState s, const std::string& reason) { /* ... */ });
```

Todos os callbacks rodam na worker thread do SDK; **não bloqueie** dentro deles.

## System

```cpp
camera.reboot();
camera.setPingInterval(10);             // segundos
camera.setMaxPingFailures(10);          // 0 = desabilita
camera.setExposureGroupTimeout(3000);   // ms (default: 5000)
```

## `captureSnapshot()` vs `onSnapshotImage`

`captureSnapshot()` internamente registra um pending capture para o RID e devolve o exposure group completo como `Result<std::vector<CaptureResult>>`. `onSnapshotImage` continua disparando por frame e é útil quando os triggers vêm de fora do seu processo ou quando você precisa de uma lógica de acumulação custom.
