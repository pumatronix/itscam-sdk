# Tests

Run by `make verify` ([`tools/verify/verify.sh`](../tools/verify/verify.sh)); [`docs/compatibility.en-US.md`](../docs/compatibility.en-US.md) explains what each step proves.

| Path | What |
| ---- | ---- |
| `standin/server.js` | A stand-in for the camera's web API (Node.js 16+, no dependencies): REST login, profiles, a CGI snapshot with plate metadata, the system telemetry and its WebSocket stream. Its `FIXTURES` are what the smoke tests expect. |
| `smoke/` | One smoke test per language: `smoke.cpp`, `smoke.c`, `smoke.py`, `go/`, `csharp/`, `java/`, `nodejs/`. |
| `consumer/` | Builds that must fail, with the SDK's message. |
| `cmake/consumer/` | An application using the SDK through CMake (`tools/verify/cmake-matrix.sh`). |
| `CMakeLists.txt` | The C and C++ smoke tests for `ctest`, with `-DITSCAM_SDK_BUILD_TESTS=ON`. |

Every smoke test checks the same things, and a new SDK feature adds its check to the stand-in and to every language in the same pull request:

1. the SDK version is reported;
2. REST login with the stand-in's credentials;
3. a typed REST read: profile 3, named `Câmera São João` (encoding and struct layout);
4. a REST 404 maps to the language's "invalid parameter" error;
5. a CGI snapshot, and the plate `ABC1D23` in its JPEG comment;
6. the binary client fails fast on a closed port and cleans up;
7. the system telemetry: health `warning` with one issue, `storageFull`;
8. the telemetry stream delivers documents, and stops.

By hand, against the stand-in:

```bash
PORT=18080 node tests/standin/server.js &
LD_LIBRARY_PATH=src/core/build/linux PYTHONPATH=src/wrappers/python python3 tests/smoke/smoke.py
```
