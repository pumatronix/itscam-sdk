# Using the SDK from CMake

The repository is a CMake project: an application builds the SDK from source with its own toolchain, as a subproject or from an installed package, and links one target, `itscam::sdk`. Português: [`cmake.md`](cmake.md).

The release artefacts are still built by `make` ([`compatibility.en-US.md`](compatibility.en-US.md)). Both build systems compile the same sources with the same flags, and `make verify` builds both.

## As a subproject

With the SDK in a subdirectory (a git submodule, a copy):

```cmake
cmake_minimum_required(VERSION 3.10)
project(app CXX)

set(ITSCAM_SDK_WITH_CGI OFF)        # options before add_subdirectory()
add_subdirectory(third_party/itscam-sdk)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE itscam::sdk)
```

With FetchContent (CMake 3.14+):

```cmake
include(FetchContent)
FetchContent_Declare(itscam_sdk
    GIT_REPOSITORY https://github.com/pumatronix/itscam-sdk.git
    GIT_TAG v0.9.0)
FetchContent_MakeAvailable(itscam_sdk)
target_link_libraries(app PRIVATE itscam::sdk)
```

As a subproject, the SDK builds a static library by default, installs nothing, and writes nothing into its source tree. The version header is generated in the build tree from `git describe`.

## As an installed package

```bash
mkdir -p build/cmake && cd build/cmake
cmake ../.. -DCMAKE_INSTALL_PREFIX=/opt/itscam-sdk
cmake --build . --target install
```

```cmake
find_package(itscam_sdk 0.9 CONFIG REQUIRED)   # CMAKE_PREFIX_PATH=/opt/itscam-sdk
target_link_libraries(app PRIVATE itscam::sdk)
```

Headers go to `include/itscam` (`#include "itscam_sdk.h"`, as with the release tarball), the library to `lib`, and the package files to `lib/cmake/itscam_sdk`. While the major version is 0, `find_package(itscam_sdk 0.9)` accepts 0.9.x only: a minor release may break callers. The package sets `itscam_sdk_SHARED`, `itscam_sdk_WITH_REST`, `itscam_sdk_WITH_CGI`, `itscam_sdk_WITH_TLS` and `itscam_sdk_WITH_C_API` to the options it was built with.

## Options

| Option | Default | Effect |
| ------ | ------- | ------ |
| `ITSCAM_SDK_SHARED` | `BUILD_SHARED_LIBS` if set, otherwise ON standalone and OFF as a subproject | Shared or static `libitscam_sdk`. |
| `ITSCAM_SDK_WITH_REST` | ON | The REST client (`ItscamRestClient`). |
| `ITSCAM_SDK_WITH_CGI` | ON | The CGI client (`ItscamCgiClient`). |
| `ITSCAM_SDK_WITH_TLS` | ON | HTTPS for REST and CGI, with the vendored mbedTLS linked in. |
| `ITSCAM_SDK_WITH_C_API` | ON | The C API, which the language wrappers need. |
| `ITSCAM_SDK_CXX_STANDARD` | 17 | Standard the SDK sources compile with: 14 or 17. The headers need C++14. |
| `ITSCAM_SDK_STATIC_RUNTIME` | ON | Shared library: link libstdc++ and libgcc in, so it needs only glibc. |
| `ITSCAM_SDK_INSTALL` | ON standalone, OFF as a subproject | Install rules and the `itscam_sdk` package. |
| `ITSCAM_SDK_BUILD_EXAMPLES` | OFF | The C++ examples of `src/examples`. |
| `ITSCAM_SDK_BUILD_TESTS` | OFF | The C and C++ smoke tests, run by `ctest` against [`tests/standin`](../tests/standin). |

The binary client (`ItscamClient`) is always built. The smallest build, the binary client alone in C++14, is the one an embedded daemon needs:

```cmake
set(ITSCAM_SDK_CXX_STANDARD 14)
set(ITSCAM_SDK_WITH_REST OFF)
set(ITSCAM_SDK_WITH_CGI OFF)
set(ITSCAM_SDK_WITH_TLS OFF)
set(ITSCAM_SDK_WITH_C_API OFF)
add_subdirectory(itscam-sdk)
```

## Cross-compiling

Pass your toolchain as usual. [`cmake/toolchains`](../cmake/toolchains) has files for the compilers in the builder image, which also show the shape a toolchain file needs:

| File | Target |
| ---- | ------ |
| `arm-linux-gnueabihf.cmake` | Linux ARMv7 hard-float (ITSCAM450) |
| `aarch64-linux-gnu.cmake` | Linux ARM64 (ITSCAM600, ITSCAM450b) |
| `x86_64-w64-mingw32.cmake`, `i686-w64-mingw32.cmake` | Windows x64, x86 |

```bash
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/aarch64-linux-gnu.cmake
```

Any Linux toolchain works whose glibc is 2.23 or newer. A toolchain without `libstdc++.a` builds the shared library with `-DITSCAM_SDK_STATIC_RUNTIME=OFF`, and then needs libstdc++ on the target.

On Windows, the DLL exports the C API only, like the release DLL. C++ applications link the static library (`ITSCAM_SDK_SHARED=OFF`).

## What `make verify` checks

[`tools/verify/cmake-matrix.sh`](../tools/verify/cmake-matrix.sh) builds an application ([`tests/cmake/consumer`](../tests/cmake/consumer)) each of these ways, with CMake 3.10:

- standalone shared build with examples and smoke tests, `ctest`, install;
- subproject, static, C++17;
- subproject, binary client only, sources and application in C++14;
- subproject, shared;
- `find_package` from the install;
- ARMv7 and ARM64 with the Arm GNU-A toolchains;
- Windows x64 (DLL, C application) and x86 (static, C++ application).
