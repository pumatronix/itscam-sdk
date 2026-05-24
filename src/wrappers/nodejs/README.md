# ITSCAM SDK - Node.js Wrapper

Idiomatic Node.js bindings for the Pumatronix ITSCAM Client SDK, backed
by the native `libitscam_sdk.{so,dll,dylib}` shared library through
[koffi](https://koffi.dev/) (the modern, maintained replacement for
`ffi-napi`).

The wrapper exposes the same three client classes as the C++, C#,
Python, Go and Java wrappers:

| Class | Use it for |
| ----- | ---------- |
| `ItscamClient` | Real-time triggers, GPIO/serial, exposure groups (binary TCP 60000). |
| `ItscamRestClient` | Equipment / daemon configuration over HTTP/HTTPS. **Always** requires `login()` first. |
| `ItscamCgiClient` | `snapshot.cgi`, `lastframe.cgi`, `mjpegvideo.cgi`. CGI auth is opt-in. |

## Layout

```
src/wrappers/nodejs/
  package.json
  src/
    index.js                  # public entry point
    native.js                 # koffi loader + C-API signatures
    errors.js                 # ItscamError hierarchy
    utils.js                  # shared FFI helpers
    itscam-client.js          # binary client (Cougar TCP 60000)
    itscam-rest-client.js     # REST client (login required)
    itscam-cgi-client.js      # CGI client (auth opt-in)
    jpeg-utils.js             # COM marker parser (pure JS)
  types/index.d.ts            # TypeScript declarations
  native/                     # populated by `make nodejs-pack`
  examples/
    capture-example.js
    rest-example.js
    cgi-snapshot-example.js
```

## Build / install

```bash
make nodejs                # populate native binaries + run `npm install`
make nodejs-pack           # produce a publish-ready @pumatronix/itscam-sdk-x.y.z.tgz

# Reproducible Docker build:
make docker-nodejs
make docker-nodejs-pack
```

`make nodejs-pack` stages the pre-built native binaries from
`src/core/build/<rid>/` into `native/<platform>-<arch>/`, then runs
`npm pack`.  Consumers can install the resulting tarball with
`npm install <path-or-url-to-tarball>` -- no native compilation
required at install time.

## Native library resolution

The koffi loader (`src/native.js`) probes for `libitscam_sdk` in this
order:

1. `process.env.ITSCAM_SDK_LIBRARY` (absolute path).
2. `native/<platform>-<arch>/<libname>` next to the wrapper.
3. `native/<libname>` (legacy fallback).
4. `src/core/build/linux/`, `src/core/build/win-x64/` (dev tree).
5. `LD_LIBRARY_PATH` (Linux/macOS) or `PATH` (Windows).

`<platform>` matches `process.platform` (`linux`, `darwin`, `win32`),
and `<arch>` matches `process.arch` (`x64`, `arm64`, `arm`, `ia32`).

## Quick start

```js
const { ItscamClient } = require('@pumatronix/itscam-sdk');

const cam = new ItscamClient();
try {
    cam.connect('192.168.254.254', 60000, 10000);
    cam.authenticate('1234', 10000);
    cam.subscribeCaptures();
    const frames = cam.captureSnapshot(15000);
    require('fs').writeFileSync('snapshot.jpg', frames[0].jpeg);
} finally {
    cam.close();
}
```

```js
const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');

const cgi = new ItscamCgiClient();
try {
    cgi.setBaseUrl('192.168.254.254', 80, 'http');
    const last = cgi.getLastFrame();
    require('fs').writeFileSync('lastframe.jpg', last.data);
} finally {
    cgi.close();
}
```

```js
const { ItscamRestClient } = require('@pumatronix/itscam-sdk');

const rest = new ItscamRestClient();
try {
    rest.setBaseUrl('192.168.254.254', 80, 'http');
    rest.login('admin', '1234');

    const profiles = rest.get('/api/image/profiles');

    // Partial PUT -- preferred for image profiles
    rest.patchJson('/api/image/profiles/0', { trigger: { enabled: false } });
} finally {
    rest.close();
}
```

## TypeScript

The package ships ambient declarations at `types/index.d.ts`. With the
package installed, `import { ItscamClient }` from
`@pumatronix/itscam-sdk` Just Works.

## Async surface

Every blocking call has an `*Async` counterpart returning a `Promise`:

```js
const frames = await cam.captureSnapshotAsync();
const profiles = await rest.getAsync('/api/image/profiles');
```

The wrappers internally call the synchronous native function on the
event-loop thread; if you need to keep the loop fully responsive,
spin up a `worker_threads` Worker around the import.

## Errors

The wrapper raises a hierarchy rooted at `ItscamError`:

| Native code | JS class |
| ----------- | -------- |
| `Timeout` | `ItscamTimeoutError` |
| `NotAuthenticated` | `ItscamAuthError` |
| `ConnectionFailed`, `Disconnected` | `ItscamConnectionError` |
| `InvalidParameter` | `ItscamInvalidParameterError` |
| `ServerError` | `ItscamServerError` |

Every instance carries a numeric `code` and a string `codeName` for
discrimination without parsing the message.

## Examples

| File | Demonstrates |
| ---- | ------------ |
| `examples/capture-example.js` | Binary client connect + authenticate + snapshot. |
| `examples/rest-example.js` | REST login + read configuration. |
| `examples/cgi-snapshot-example.js` | `lastframe.cgi`, `snapshot.cgi`, MJPEG streaming. |

Run them with:

```bash
make nodejs
node src/wrappers/nodejs/examples/capture-example.js 192.168.254.254 1234
node src/wrappers/nodejs/examples/cgi-snapshot-example.js 192.168.254.254
```

## License

Copyright (c) 2026 Pumatronix Equipamentos Eletronicos. Proprietary.
