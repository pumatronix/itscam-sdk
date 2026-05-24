# Python wrapper

[Português (Brasil)](python.md) | [English (US)](python.en-US.md)

The Python wrapper lives at [`src/wrappers/python/`](../../src/wrappers/python/) and uses **ctypes** on top of the SDK's C API. It supports Python 3.7+ on Linux and Windows.

> **Full class and method reference** (generated from source): [pdoc for the `itscam` module](/api-ref/python/itscam.html). This page covers installation, usage patterns, and examples.

## Install

### From the pre-compiled SDK package (recommended)

The distribution package (`itscam-sdk-<version>.tar.gz`) includes a Python wheel with the native lib bundled. Download from the [releases page](https://github.com/pumatronix/itscam-sdk/releases):

```bash
tar xzf itscam-sdk-<version>.tar.gz
pip install itscam-sdk-<version>/linux-x64/python/itscam-*.whl
python -c "import itscam; print(itscam.get_version())"
```

The wheel already bundles `libitscam_sdk.so` -- no `LD_LIBRARY_PATH` configuration is needed.

### From source (advanced)

If you are developing inside the SDK source tree:

```bash
make lib                         # build libitscam_sdk.so first
cd src/wrappers/python
pip install -e .                 # editable install
```

In this case, ctypes locates the shared library by:

1. The directory containing `bindings.py`, then its parent.
2. `<parent>/build/` (handy when bundled with a build output).
3. System paths (`/usr/local/lib`, `/usr/lib`).
4. Every entry in `LD_LIBRARY_PATH`.

For ad-hoc usage without `pip install`, point `LD_LIBRARY_PATH` at the build directory and prepend the wrapper to `PYTHONPATH`:

```bash
export LD_LIBRARY_PATH=$PWD/src/core/build/linux:$LD_LIBRARY_PATH
export PYTHONPATH=$PWD/src/wrappers/python:$PYTHONPATH
python -c "import itscam; print(itscam.get_version())"
```

## Surface

```python
from itscam import (
    ItscamClient, ItscamRestClient, ItscamCgiClient,
    SnapshotCgiRequest, CgiImage, CgiStreamFrame,
    SnapshotRequest, CaptureResult, FrameInfo,
    ItscamError, ItscamTimeoutError, ItscamConnectionError, ItscamAuthError,
    LogLevel, ConnectionState,
)
```

Each client is a context-manager-friendly Python class that mirrors its C++ counterpart 1:1. Errors are raised as exceptions instead of returned as `Result<T>`.

## CGI usage (auth optional)

```python
from itscam import ItscamCgiClient, SnapshotCgiRequest

with ItscamCgiClient() as cgi:
    cgi.set_base_url("192.168.254.254", 80)
    # cgi.set_base_url("camera.example.com", 443, "https")
    # cgi.set_verify_server_certificate(False)
    # cgi.login("admin", "1234")   # only when blockAPI=true

    with open("lastframe.jpg", "wb") as f:
        f.write(cgi.get_last_frame().data)

    images = cgi.get_snapshot(SnapshotCgiRequest(quality=80))
    for i, img in enumerate(images):
        with open(f"snap-{i}.jpg", "wb") as f:
            f.write(img.data)

    def on_frame(frame: CgiStreamFrame) -> None:
        ...  # runs on the SDK worker thread; do not block
    cgi.start_mjpeg_stream(on_frame)
    time.sleep(5)
    cgi.stop_mjpeg_stream()
```

## REST usage (auth required)

The REST client exposes two coexisting surfaces:

* **Typed helpers** (preferred) -- `rest.get_ocr_config()`, `rest.set_ocr_config(cfg)`, `rest.get_profiles()` etc. return dataclasses generated from the camera's OpenAPI document. See [`docs/codegen.md`](../codegen.md) for the maintainer / downstream refresh workflow.
* **Generic verbs** (escape hatch) -- `rest.get(path)`, `rest.put(path, body)`, `rest.post`, `rest.delete` return parsed JSON (`dict` / `list`).

* **Partial PUT** -- `rest.patch_json(path, patch)` sends only the fields being changed. Required for image profiles and recommended for most configuration updates. See [`docs/api/rest-client.md`](../api/rest-client.md).

```python
from itscam import ItscamRestClient, ProfileConfig

with ItscamRestClient() as rest:
    rest.set_base_url("192.168.254.254", 80)
    rest.login("admin", "1234")

    # Read-only: typed POCO is convenient.
    profiles = rest.get_profiles()

    # Write: partial PUT (do not round-trip the full profile object).
    rest.patch_json("/api/image/profiles/0",
                    {"trigger": {"enabled": False}})

    # Generic verbs:
    print(rest.get("/api/equipment/misc/readonly/constants"))
```

The full POCO module is also available as `itscam.rest_types` for explicit imports.

## Binary client usage

```python
from itscam import ItscamClient

camera = ItscamClient()
camera.connect("192.168.254.254")
camera.authenticate("1234")
camera.subscribe_captures()
result = camera.capture_snapshot()
# result is a list of CaptureResult (one per exposure step)
```

## Examples

Ready-to-run scripts under [`src/wrappers/python/examples/`](../../src/wrappers/python/examples/):

| Script                                   | Purpose                            |
| ---------------------------------------- | ---------------------------------- |
| `capture_example.py <host>`              | Binary client capture loop.        |
| `rest_example.py <host> <user> <pass>`   | REST login + read configuration.   |
| `cgi_snapshot_example.py <host> [...]`   | CGI lastframe + snapshot + MJPEG.  |

The CGI example accepts optional `--user`/`--password` flags; without them it talks to the camera anonymously.
