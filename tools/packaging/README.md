# SDK distribution packaging

`make sdk-dist` builds a consumer-ready tarball under `dist/`:

```
dist/itscam-sdk-<version>-<rid>.tar.gz
  cpp/      C++ headers + libitscam_sdk.so
  c/        C API headers + libitscam_sdk.so
  csharp/   Pumatronix.Itscam.Sdk NuGet package
  python/   itscam wheel (native library bundled)
  go/       Standalone Go module + native/ + include/
  README.txt
```

## Targets

| Target | Description |
| ------ | ----------- |
| `make sdk-dist` | Build library, NuGet, wheel, stage tree, create tar.gz |
| `make docker-sdk-dist` | Same, inside the Docker builder image |
| `make sdk-dist-clean` | Remove `dist/` staging and archives |

## Variables

| Variable | Default | Purpose |
| -------- | ------- | ------- |
| `SDK_VERSION` | from `itscam_sdk.h` | Archive version segment |
| `SDK_RID` | `linux-x64` | Platform tag in the archive name |

## Prerequisites

- `make lib` artefacts (`src/core/build/linux/libitscam_sdk.so.1.0.0`)
- `dotnet` for `make csharp-pack`
- `python3` (+ `build` module or setuptools) for the wheel

Use `make docker-sdk-dist` when the host lacks toolchains.
