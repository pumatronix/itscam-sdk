# mbedTLS (vendored)

ITSCAM SDK statically links a minimal client-only build of [mbedTLS]
(https://github.com/Mbed-TLS/mbedtls) to provide HTTPS support for
`ItscamRestClient` and `ItscamCgiClient` via `cpp-httplib`'s
`CPPHTTPLIB_MBEDTLS_SUPPORT` backend.

The entire upstream source tree (`include/`, `library/`, `3rdparty/`
and `LICENSE`) is **vendored and committed** to this repository, so the
SDK builds with HTTPS support out-of-the-box -- no extra setup step.

## Version

| Field            | Value                                                                                   |
| ---------------- | --------------------------------------------------------------------------------------- |
| Vendored release | **mbedTLS 3.6.2** (LTS)                                                                 |
| Upstream tag     | [v3.6.2](https://github.com/Mbed-TLS/mbedtls/releases/tag/v3.6.2)                       |
| License          | Apache-2.0 (see `LICENSE` in this directory)                                            |
| Supported range  | mbedTLS 3.6.x LTS; other 3.x releases are expected to work, 2.x is **not** supported    |

`cpp-httplib`'s mbedTLS backend depends on the 3.x PSA crypto API, so
2.x releases will not build.

## Layout

```
core/3rdparty/mbedtls/
    include/mbedtls/...           <- upstream public headers
    include/psa/...
    library/*.c                   <- upstream C source files
    3rdparty/...                  <- upstream-bundled deps (everest, p256-m)
    LICENSE                       <- upstream license (Apache-2.0)
    itscam_mbedtls_config.h       <- SDK overlay config (tracked)
    mbedtls.cmake                 <- OBJECT-library fragment (tracked)
    README.md                     <- this file
```

The custom build configuration lives at
`itscam_mbedtls_config.h` (next to the sources).  It enables only the
cipher suites, X.509 features and crypto primitives needed for a
TLS 1.2 / 1.3 client; the resulting static archive contributes ~600 KB
on ARM64 release builds.

## Build integration

The library is compiled directly into `libitscam_sdk` (both the shared
and the static variants) by the SDK's
[core/Makefile](../../Makefile) and
[core/CMakeLists.txt](../../CMakeLists.txt):

- `MBEDTLS_USER_CONFIG_FILE="itscam_mbedtls_config.h"` is forced on every
  mbedTLS translation unit so we never rely on the upstream default
  config.
- `CPPHTTPLIB_MBEDTLS_SUPPORT` is defined for SDK translation units that
  include `httplib.h` so that `httplib::SSLClient` lights up.
- mbedTLS symbols are kept private to the SDK with
  `-Wl,--exclude-libs,ALL` on Linux and `-fvisibility=hidden` on
  shared-library builds.

If the vendored source tree is pruned, the SDK build falls back to a
plain HTTP build (a CMake / Makefile guard handles the missing files)
and a warning is emitted.

## Upgrading

To upgrade the vendored release, replace the contents of `include/`,
`library/`, `3rdparty/` and `LICENSE` with the corresponding files from
a newer mbedTLS 3.x source tarball (the
[upstream releases page](https://github.com/Mbed-TLS/mbedtls/releases)
publishes them as `mbedtls-vX.Y.Z.tar.gz`).  Then rebuild the SDK and
verify the resulting `libitscam_sdk` still satisfies the SDK's HTTPS
integration tests.  Do not edit `itscam_mbedtls_config.h` unless the
upstream config layout actually changes.
