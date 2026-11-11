# Compatibility and verification

What the SDK promises to the applications built on it, how each promise is checked, and what a change must do before it can break one. Português: [`compatibility.md`](compatibility.md).

## Supported platforms

| Platform | Release artefact | Requires on the target | Applications build with |
| -------- | ---------------- | ---------------------- | ----------------------- |
| Linux x86_64 | `libitscam_sdk.so`, `.a` | glibc 2.23 or newer (Ubuntu 16.04+) | any GCC or Clang from GCC 5, C++14 or C++17, or C99 |
| Linux ARMv7 hard-float (ITSCAM450) | same | glibc 2.23 or newer | same, e.g. Ubuntu's `arm-linux-gnueabihf` or Arm GNU-A |
| Linux ARM64 (ITSCAM600, ITSCAM450b) | same | glibc 2.23 or newer | same, `aarch64-linux-gnu` |
| Windows x64 and x86 | `itscam_sdk.dll` (C API), `libitscam_sdk_static.a` | nothing (no MinGW DLLs) | MinGW-w64: C against the DLL, C++ against the static library |

The Linux shared library carries its own C++ runtime: libstdc++ and libgcc are linked in, with their symbols kept local, so it needs only glibc. It links with any toolchain, whatever libstdc++ that has, including ones whose libstdc++ has no symbol versioning (Arm GNU-A 8.3). It exports only the C API and the `itscam` namespace ([`src/core/itscam_sdk.map`](../src/core/itscam_sdk.map)). Its copies of libstdc++, nlohmann::json, cpp-httplib and mbedTLS never interpose on, or bind to, an application's own copies of the same code.

Consequences for C++ applications:

- Use the libstdc++ "new" ABI (`_GLIBCXX_USE_CXX11_ABI=1`, the default since GCC 5). `std::string` and other standard types cross the C++ API.
- Objects the library returns are freed by the application, and the other way round. Both sides allocate through `malloc`. An application that replaces the global `operator new` with an allocator that doesn't use `malloc` must link the static library instead, or build the shared one with `STATIC_RUNTIME=0`.
- Include `itscam_sdk.h` before anything that sets optional-lite to the standard `std::optional`. Otherwise the build stops with an error saying so: the REST types must have the layout the library was built with.

Language wrappers, oldest version supported and tested:

| Wrapper | Oldest supported | Checked by `make verify` with |
| ------- | ---------------- | ----------------------------- |
| C | C99 | GCC 5.4, GCC 7.5, Arm GNU-A 8.3, MinGW-w64 |
| C++ | C++14 | the same, as C++14 and C++17 |
| Python | 3.7 | Python 3.7 on x64, ARMv7 and ARM64 |
| Node.js | 16; on ARMv7, glibc 2.34 or newer (koffi's prebuilt binary) | Node.js 16 on x64 and ARM64, Node.js 20 on ARMv7 (Debian 12) and on x64 |
| Go | the `go` directive of `src/wrappers/go/go.mod` | the builder's Go, on x64, ARMv7 and ARM64 |
| C# / .NET | netstandard2.0 | .NET 8 on x64 |
| Java | Java 7 | compiled with JDK 7; run on Java 11 (x64), also with a non-UTF-8 default charset |
| CMake | 3.10 | CMake 3.10 ([`cmake.md`](cmake.en-US.md)) |

## `make verify`

One command builds everything the SDK ships and checks it the way an application uses it, on every platform above. It runs in the repository's Docker images, the same ones CI uses, so a green run on your machine means a green CI run. Run it before every pull request. CI runs it on every pull request too ([`.github/workflows/verify.yml`](../.github/workflows/verify.yml)).

```bash
make verify                                  # everything, ~30-60 min the first time
VERIFY_STEPS="cores consumers run" make verify   # some steps
VERIFY_ARCHES=arm64 make qemu-smoke          # the ARM part only
make compat-check                            # API/ABI against the last release only
```

| Step | What it proves |
| ---- | -------------- |
| `cores` | The Linux libraries build for x64, ARMv7 and ARM64 at the glibc 2.23 floor, need no symbol newer than that floor, and don't depend on libstdc++ or libgcc (`tools/check-glibc.sh --no-cxx-runtime`). |
| `windows` | The Windows DLL and static libraries build, and the DLL needs no MinGW runtime DLL. |
| `consumers` | The C and C++ smoke tests ([`tests/smoke`](../tests/smoke)) build against the release libraries with every supported compiler: GCC 5.4 (the floor), GCC 7.5, Arm GNU-A 8.3, as C++14 and C++17 and C99, and with MinGW. Including the standard optional before the SDK fails to compile, as it must. |
| `wrappers` | The Go, Node.js, Java and C# smoke tests build. |
| `run` | Every smoke test runs against [`tests/standin`](../tests/standin), a stand-in for the camera's web API. C and C++ run on Ubuntu 16.04 (the floor) and on their compiler's own runtime, and Go on its compiler's own runtime, on x64, ARMv7 and ARM64 (ARM under qemu). Python 3.7 and Node.js run on all three. C# and Java run on x64, Java also with an ASCII default charset. |
| `cmake` | [`tools/verify/cmake-matrix.sh`](../tools/verify/cmake-matrix.sh): standalone build with its tests, subproject with several option sets (down to the C++14 binary client alone), installed package, ARM and Windows toolchains. |
| `compat` | [`tools/compat/check-compat.sh`](../tools/compat/check-compat.sh): API and ABI against the last release (below). |
| `codegen` | The generated REST types match [`tools/codegen/spec`](../tools/codegen). |
| `java7` | The Java wrapper compiles with a real JDK 7. |

The smoke tests check the same things in every language: the version, REST login, a typed REST read (a non-ASCII string and an integer, which catches both encoding and struct layout mismatches), a 404 mapped to the "invalid parameter" error, a CGI snapshot with its JPEG plate metadata, and a binary-client connection that fails fast. A feature added to the SDK adds its check to the stand-in and to every language's smoke test, in the same pull request.

## The compatibility contract

Compared on every pull request with the release in [`compat/baseline`](../compat/baseline):

- **ABI** of `libitscam_sdk.so` on x64, ARMv7 and ARM64, by `abidiff` (libabigail) over the public headers: the C functions and every C++ function, variable and type layout the headers expose. Adding is fine. Removing a symbol, or changing a signature or the layout of a type the API passes, breaks applications built against the release without a rebuild.
- **API** of each wrapper: a listing of the public declarations of the Python, Go, Node.js (typings), C# and Java wrappers. Adding is fine. A removed or changed declaration breaks callers' source.

What counts as breaking, beyond what the tools see:

- Behaviour a caller relies on: an error code for a given failure, a default value, a callback's thread or ordering, a unit.
- A new requirement on the target: a newer glibc, a library the target must provide, a newer language runtime than the tables above.

## Breaking changes

While the major version is 0, a minor release may break callers. A patch release never does. Every break must:

1. **Be accepted in [`compat/`](../compat)**, so the reviewer sees it:
   - ABI: a libabigail suppression in [`compat/abi.suppr`](../compat/abi.suppr), with a comment saying what changed and what callers must do.
   - API: the removed line, as `check-compat.sh` printed it, in [`compat/accepted-api-breaks.txt`](../compat/accepted-api-breaks.txt), prefixed by its surface (`python`, `go`, `nodejs`, `csharp`, `java`).
2. **Ship in the next minor release** (0.x.0), never in a patch.
3. **Have a release note** saying what breaks and what callers do about it, e.g. "C++ applications that call `getRestApiClientConfig` must be rebuilt".

After a release, its commit moves `compat/baseline` to the new tag and empties `abi.suppr` and `accepted-api-breaks.txt` (keep their header comments).

The shared library's soname is `libitscam_sdk.so.0` for every 0.x release, so the dynamic loader can't tell a 0.x release that breaks callers from one that doesn't. Applications must be rebuilt when a release note says so.

## Guard-rails for changes

- C API ([`src/core/c_api`](../src/core/c_api)): never remove or change a function, enum value or struct field. Add new functions instead.
- C++ API: a struct the API passes by value or returns (the REST types, `Result<T>`) changes layout when a field is added. That is a break: see above.
- A new feature reaches every wrapper in the same pull request ([`adding-a-new-wrapper.en-US.md`](adding-a-new-wrapper.en-US.md)), along with its smoke checks.
- A new build requirement (compiler, glibc, runtime) updates the tables above, the Docker images and `make verify` in the same pull request.
- Build systems: the Makefile builds the release artefacts and CMake builds for consumers. Both use the same sources and flags, and `make verify` builds both.
