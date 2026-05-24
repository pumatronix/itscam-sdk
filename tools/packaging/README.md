# SDK distribution packaging

`make sdk-dist` builds a consumer-ready multi-platform tarball under `dist/`. Published releases are attached to [GitHub Releases](https://github.com/pumatronix/itscam-sdk/releases).

```
dist/itscam-sdk-<version>.tar.gz
  VERSION.json
  README-sdk.md        Tarball layout and per-language install notes (PT-BR)
  README-sdk.en-US.md  Same, in English (US)
  README-repo.md       Navigation hub from the GitHub repo (Portuguese)
  README.en-US.md      Same hub, in English (US)
  AGENTS.md
  docs/                Chapter-style guides (getting started, API clients, wrappers, ...)
  csharp/              Pumatronix.Itscam.Sdk NuGet (linux-x64 + win-x64 + win-x86)
  examples/            Example source (all languages)
    cpp/               C++ sources + consumer Makefile (Linux x64)
    go/                Go CLI sources + Wails GUI source
    csharp/            C# example projects (PackageReference to bundled NuGet)
    java/              Java example sources
    python/            Python scripts
    nodejs/            Node.js scripts
    bin/
      linux-x64/itscam-viewer          pre-built Wails GUI
      win-x64/itscam-viewer.exe        pre-built Wails GUI
  linux-x64/
    cpp/include/ + cpp/lib/libitscam_sdk.so*
    c/include/c_api/ + c/lib/
    python/itscam-*.whl
    go/itscam-sdk-go/
    java/itscam-sdk-*.jar
    nodejs/pumatronix-itscam-sdk-*.tgz
  win-x64/ ...
  win-x86/ ...
```

## Targets

| Target | Description |
| ------ | ----------- |
| `make sdk-dist-examples` | Build pre-built Wails GUI (linux-x64 + win-x64) |
| `make sdk-dist` | Build natives, wrappers, example source, Wails GUI, tar.gz |
| `make docker-sdk-dist` | Same, inside the Docker builder image |
| `make docker-sdk-dist-examples` | Build Wails GUI only, inside Docker |
| `make sdk-dist-clean` | Remove `dist/` staging and archives |

## Prerequisites

Built by `make sdk-dist` automatically:

- `make lib` — Linux `libitscam_sdk.so.<tag-version>`
- `make windows` — Windows `itscam_sdk.dll` + import library (MinGW win32 threading, static libgcc/libstdc++)
- `make csharp-pack` — multi-RID NuGet
- `make java-pack` / `make nodejs-pack` — Java JAR and npm tarball
- `make sdk-dist-examples` — pre-built Wails GUI (`itscam-viewer` / `itscam-viewer.exe`)
- `python3` — platform-specific wheels

Example **source code** is copied into `examples/` during staging. Only the Wails GUI is shipped as pre-built binaries.

Use `make docker-sdk-dist` when the host lacks toolchains.

Version metadata comes from `make version` (git tag, commit SHA, build date).
See [`tools/version/README.md`](../version/README.md).
