# Migração do CougarClient

[Português (Brasil)](migration-cougar.md) | [English (US)](migration-cougar.en-US.md)

O legacy `CougarClient` é substituído pelo `ItscamClient`. A nova API mantém o mesmo on-wire protocol mas expõe métodos tipados em vez de envelopes JSON genéricos.

| Antigo (`CougarClient`)                             | Novo (`ItscamClient`)                             |
| --------------------------------------------------- | ------------------------------------------------- |
| `cougar.start("ip")`                                | `camera.connect("ip")`                            |
| `genericSyncCall(GC_AUTHENTICATE, {{"pass","x"}})`  | `camera.authenticate("x")`                        |
| `genericSyncCall(GC_SET_CALLBACKS, json)`           | `camera.subscribe(EventSubscription{...})`        |
| `genericSyncCall(GC_SET_JPEG_CFGS, json)`           | `camera.setJpegConfig(JpegConfig{...})`           |
| `genericSyncCall(GC_TRIGGER_SNAPSHOT, json)`        | `camera.captureSnapshot(SnapshotRequest{...})`    |
| `setMixedCallback(CBMX_IMGPKG_SNAPSHOT, fn)`        | `camera.onSnapshotImage(fn)`                      |
| `genericSyncCall(GC_SET_SERIAL_CFGS, json)`         | `camera.configureSerial(SerialPort::Serial1, ...)`|
| `genericSyncCall(GC_SEND_SERIAL_DATA, json)`        | `camera.sendSerialAscii(SerialPort::Serial1, ...)`|
| `genericSyncCall(GC_SET_EQUIP_CFGS, json)`          | `camera.setConfig(path, data)` ou typed helper    |
| `genericSyncCall(GC_CMD_REBOOT, {})`                | `camera.reboot()`                                 |

O que você ganha em cima da API legacy:

- `Result<T>` / `Future<T>` para error handling explícito e tipado.
- Callback de lifecycle `ConnectionState` (`Connected`, `Disconnected`, `Reconnecting`, `Reconnected`).
- Auto-reconnect com restore completo da session.
- Acumulador de exposure-group com callbacks por grupo.
- Helpers de trigger / exposure cientes do profile ativo.
- Escape hatch genérico `setConfig(path, data)` quando ainda não existe typed helper.

Veja a [referência do binary client](api/binary-client.md) para a superfície completa.
