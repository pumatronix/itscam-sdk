# SDK distribution packaging

`make sdk-dist` builds a consumer-ready multi-platform tarball under `dist/`:

```
dist/itscam-sdk-<version>.tar.gz
  VERSION.json
  README.txt
  csharp/              Pumatronix.Itscam.Sdk NuGet (linux-x64 + win-x64 + win-x86)
  linux-x64/
    cpp/include/ + cpp/lib/libitscam_sdk.so*
    c/include/c_api/ + c/lib/
    python/itscam-*.whl
    go/itscam-sdk-go/
  win-x64/
    cpp/include/ + cpp/bin/itscam_sdk.dll + libitscam_sdk.a
    c/include/c_api/ + c/bin/
    python/itscam-*.whl
    go/itscam-sdk-go/
  win-x86/
    cpp/include/ + cpp/bin/itscam_sdk.dll + libitscam_sdk.a
    c/include/c_api/ + c/bin/
    python/itscam-*.whl
    go/itscam-sdk-go/
```

## Targets

| Target | Description |
| ------ | ----------- |
| `make sdk-dist` | Build Linux + Windows natives, NuGet, wheels, stage tree, tar.gz |
| `make docker-sdk-dist` | Same, inside the Docker builder image |
| `make sdk-dist-clean` | Remove `dist/` staging and archives |

## Prerequisites

Built by `make sdk-dist` automatically:

- `make lib` — Linux `libitscam_sdk.so.<tag-version>`
- `make windows` — Windows `itscam_sdk.dll` + import library (MinGW win32 threading, static libgcc/libstdc++)
- `make csharp-pack` — multi-RID NuGet
- `python3` — platform-specific wheels

Use `make docker-sdk-dist` when the host lacks toolchains.

Version metadata comes from `make version` (git tag, commit SHA, build date).
See [`tools/version/README.md`](../version/README.md).
