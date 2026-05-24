# Node.js wrapper

[Português (Brasil)](nodejs.md) | [English (US)](nodejs.en-US.md)

The Node.js wrapper lives under [`src/wrappers/nodejs/`](../../src/wrappers/nodejs/) and uses **[koffi](https://koffi.dev/)** on top of the SDK's C ABI. It supports Node.js 16+ on Linux, Windows, and macOS. TypeScript declarations ship in [`types/index.d.ts`](../../src/wrappers/nodejs/types/index.d.ts) -- both pure JavaScript and TypeScript work out of the box.

> The wrapper exposes the SDK's three client surfaces:
>
> | Class | Native counterpart |
> | ----- | ------------------ |
> | `ItscamClient` | `src/core/itscam_client.h` |
> | `ItscamRestClient` | `src/core/itscam_rest_client.h` |
> | `ItscamCgiClient` | `src/core/itscam_cgi_client.h` |

## Installation

### From the pre-built SDK package (recommended)

The distribution archive (`itscam-sdk-<version>.tar.gz`) ships an npm tarball with native binaries embedded under `native/<platform>-<arch>/`:

```bash
tar xzf itscam-sdk-<version>.tar.gz
npm install $PWD/itscam-sdk-<version>/linux-x64/nodejs/pumatronix-itscam-sdk-<version>.tgz
```

Then in your project:

```js
const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');
```

The native library is resolved automatically; no `LD_LIBRARY_PATH` required.

### Building from source (advanced)

When working inside the SDK source tree:

```bash
make lib                        # build libitscam_sdk.so first
make nodejs                     # stage native libs + npm install
make nodejs-pack                # produces @pumatronix/itscam-sdk-x.y.z.tgz
make docker-nodejs              # same, inside the Docker builder
```

`make nodejs` does three things:

1. Builds `libitscam_sdk.so` if it isn't already present.
2. Runs `tools/packaging/stage-nodejs-natives.sh`, which copies the pre-built binaries from `src/core/build/<rid>/` into `src/wrappers/nodejs/native/<platform>-<arch>/`.
3. Runs `npm install` to fetch `koffi`.

`make nodejs-pack` adds an `npm pack` step, producing a self-contained `pumatronix-itscam-sdk-<version>.tgz` ready for `npm install ./that.tgz` or `npm publish`.

## Native library resolution

`src/native.js` searches for `libitscam_sdk` in the following order:

1. `process.env.ITSCAM_SDK_LIBRARY` (absolute path).
2. `native/<platform>-<arch>/<libname>` next to the wrapper.
3. `native/<libname>` (legacy fallback).
4. `src/core/build/linux/`, `src/core/build/win-x64/` (SDK dev tree).
5. `LD_LIBRARY_PATH` (Linux/macOS) / `PATH` (Windows).

`<platform>` is `process.platform` (`linux`, `darwin`, `win32`); `<arch>` is `process.arch` (`x64`, `arm64`, `arm`, `ia32`).

## Idiomatic surface

| Concern | What you get |
| ------- | ------------ |
| Async | Every blocking call has an `*Async` companion returning `Promise<T>`. |
| Lifetime | Every client has a `close()` method (and `Symbol.dispose` for `using` on Node 22+ / TS 5.2+). |
| Streaming | `startMjpegStream(callback)` invokes the callback on the SDK worker thread. |
| Errors | `Error` hierarchy rooted in `ItscamError` (`ItscamTimeoutError`, `ItscamAuthError`, ...). |
| TS | Full declarations in `types/index.d.ts` -- IntelliSense works in both JS and TS. |

## CGI usage (auth optional)

```js
const fs = require('fs');
const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');

const cgi = new ItscamCgiClient();
try {
    cgi.setBaseUrl('192.168.254.254', 80, 'http');
    // cgi.setBaseUrl('camera.example.com', 443, 'https');
    // cgi.setVerifyServerCertificate(false);
    // cgi.login('admin', '1234', 10000);   // only when blockAPI=true

    const last = cgi.getLastFrame();
    fs.writeFileSync('lastframe.jpg', last.data);

    const images = cgi.getSnapshot({ quality: 80 });
    images.forEach((img, i) => fs.writeFileSync(`snap-${i}.jpg`, img.data));

    cgi.startMjpegStream((frame) => {
        // Runs on the SDK worker thread; do not block.
    });
    await new Promise(r => setTimeout(r, 5000));
    cgi.stopMjpegStream();
} finally {
    cgi.close();
}
```

## REST usage (auth required)

```js
const { ItscamRestClient } = require('@pumatronix/itscam-sdk');

const rest = new ItscamRestClient();
try {
    rest.setBaseUrl('192.168.254.254', 80, 'http');
    rest.login('admin', '1234');

    const profiles = rest.get('/api/image/profiles');

    // Partial PUT -- preferred for image profiles
    rest.patchJson('/api/image/profiles/0',
                   { trigger: { enabled: false } });
} finally {
    rest.close();
}
```

* **Generic verbs** (preferred): `get`, `put`, `post`, `delete`, `patchJson`. They return parsed JSON (object / array) or raw string when the body is not JSON.
* **Typed convenience helpers**: `getProfiles`, `setOcrConfig`, etc. They also return parsed JSON. Typed POCOs / TS interfaces via codegen are a follow-up -- see [`docs/codegen.md`](../codegen.md) for status.
* **Partial PUT** -- `rest.patchJson(path, partialBody)` sends only changed fields. Mandatory for `PUT /api/image/profiles/{id}` (the camera rejects the full body with HTTP 500). See [`docs/api/rest-client.md`](../api/rest-client.md).

## Binary client usage

```js
const { ItscamClient } = require('@pumatronix/itscam-sdk');

const camera = new ItscamClient();
try {
    camera.connect('192.168.254.254', 60000, 10000, { enabled: true });
    camera.authenticate('1234');
    camera.subscribeCaptures();

    const frames = camera.captureSnapshot();
    for (const r of frames) {
        require('fs').writeFileSync(`snap-${r.info.requestId}.jpg`, r.jpeg);
    }
} finally {
    camera.close();
}
```

## TypeScript

```ts
import {
    ItscamCgiClient,
    SnapshotCgiRequest,
    CgiStreamFrame,
} from '@pumatronix/itscam-sdk';

const cgi = new ItscamCgiClient();
const images = cgi.getSnapshot({ quality: 80, mosaic: false });
```

The declarations cover every client, error type, dataclass (with JSDoc on the JS source) and JPEG helper.

## Examples

Under [`src/wrappers/nodejs/examples/`](../../src/wrappers/nodejs/examples/):

| Script | Demonstrates |
| ------ | ------------ |
| `capture-example.js <host> [pass]` | Binary client connect + authenticate + snapshot. |
| `rest-example.js <host> <user> <pass>` | REST login + reading configuration. |
| `cgi-snapshot-example.js <host> [...]` | `lastframe.cgi`, `snapshot.cgi`, MJPEG streaming. |

Run them with:

```bash
make nodejs
node src/wrappers/nodejs/examples/capture-example.js 192.168.254.254 1234
node src/wrappers/nodejs/examples/cgi-snapshot-example.js 192.168.254.254 --user admin --password 1234
```

## Step-by-step tutorial

For a from-scratch walkthrough (`npm init`, install, save the first image to disk), see [First image with Node.js](../tutorials/first-image-nodejs.md).
