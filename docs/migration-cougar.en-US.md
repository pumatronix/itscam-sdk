# Migration from CougarClient

[Português (Brasil)](migration-cougar.md) | [English (US)](migration-cougar.en-US.md)

The legacy `CougarClient` is replaced by `ItscamClient`. The new API keeps the same on-wire protocol but exposes typed methods instead of generic JSON envelopes.

| Old (`CougarClient`)                                | New (`ItscamClient`)                              |
| --------------------------------------------------- | ------------------------------------------------- |
| `cougar.start("ip")`                                | `camera.connect("ip")`                            |
| `genericSyncCall(GC_AUTHENTICATE, {{"pass","x"}})`  | `camera.authenticate("x")`                        |
| `genericSyncCall(GC_SET_CALLBACKS, json)`           | `camera.subscribe(EventSubscription{...})`        |
| `genericSyncCall(GC_SET_JPEG_CFGS, json)`           | `camera.setJpegConfig(JpegConfig{...})`           |
| `genericSyncCall(GC_TRIGGER_SNAPSHOT, json)`        | `camera.captureSnapshot(SnapshotRequest{...})`    |
| `setMixedCallback(CBMX_IMGPKG_SNAPSHOT, fn)`        | `camera.onSnapshotImage(fn)`                      |
| `genericSyncCall(GC_SET_SERIAL_CFGS, json)`         | `camera.configureSerial(SerialPort::Serial1, ...)`|
| `genericSyncCall(GC_SEND_SERIAL_DATA, json)`        | `camera.sendSerialAscii(SerialPort::Serial1, ...)`|
| `genericSyncCall(GC_SET_EQUIP_CFGS, json)`          | `camera.setConfig(path, data)` or typed helper    |
| `genericSyncCall(GC_CMD_REBOOT, {})`                | `camera.reboot()`                                 |

What you get on top of the legacy API:

- `Result<T>` / `Future<T>` for explicit, typed error handling.
- A `ConnectionState` lifecycle callback (`Connected`, `Disconnected`, `Reconnecting`, `Reconnected`).
- Auto-reconnect with full session restore.
- Exposure-group accumulator with per-group callbacks.
- Profile-aware trigger / exposure helpers.
- Generic `setConfig(path, data)` escape hatch when no typed helper exists yet.

See the [binary client reference](api/binary-client.md) for the full surface.
