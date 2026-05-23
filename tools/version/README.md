# SDK versioning

Version metadata is generated from **git tags**, **commit SHA**, and **UTC
build date** before every library build.

```bash
make version    # regenerate all version artefacts
```

## Sources

| Input | Usage |
| ----- | ----- |
| Latest `v*` git tag | Base semver (`v0.0.1` → `0.0.1`) and `libitscam_sdk.so.M.m.p` name |
| Commits since tag | Prerelease suffix (`0.0.1-dev.N`) |
| Dirty working tree | Appends `+dirty.<sha>` |
| `git rev-parse HEAD` | Full SHA embedded in metadata |
| Build time (UTC) | ISO-8601 timestamp in `VERSION.json` and runtime string |

## Generated files (gitignored)

| File | Consumers |
| ---- | --------- |
| `tools/version/sdk-version.mk` | Top-level / core Makefiles |
| `src/core/itscam_sdk_version.h` | C/C++ core, `ITSCAM_getVersion()` |
| `src/wrappers/python/itscam/_version.py` | `setup.py`, `itscam.__version__` |
| `src/wrappers/csharp/Version.props` | NuGet `Version` / `InformationalVersion` |
| `src/wrappers/go/itscam/version.go` | `GetSDKVersion()` / `GetSDKVersionFull()` |
| `VERSION.json` | CI, release tooling, sdk-dist archive |

## Runtime API

- C/C++: `ITSCAM_getVersion()` → `"0.0.1+<sha> (YYYY-MM-DD)"`
- Python: `itscam.__version__`, `itscam.__version_full__`, `itscam.get_version()`
- C#: `AssemblyInformationalVersion` on `Itscam.Sdk`
- Go: `itscam.GetSDKVersion()`, `itscam.GetSDKVersionFull()`

Tag a release before shipping packages:

```bash
git tag v1.0.0
make sdk-dist
```
