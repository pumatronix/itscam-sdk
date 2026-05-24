# Getting started

[Português (Brasil)](getting-started.md) | [English (US)](getting-started.en-US.md)

Five-minute path from the pre-compiled SDK package to a running application against the camera. If you need to build the SDK from scratch (contributors, platform customization), there is an [advanced section](#building-the-sdk-from-source) at the bottom.

## Prerequisites

- An ITSCAM camera (ITSCAM450 / ITSCAM600) reachable over the network.
- The distribution package `itscam-sdk-<version>.tar.gz` (download from the [releases page](https://github.com/pumatronix/itscam-sdk/releases)).
- For C++ / C: a C++17 compiler (GCC 7+, Clang 5+) and `make`.
- For C# / .NET: .NET SDK 6.0+.
- For Python: Python 3.7+.
- For Go: Go 1.21+ with cgo enabled.

## Pre-compiled package layout

Extracting `itscam-sdk-<version>.tar.gz` gives you the following structure:

```
itscam-sdk-<version>/
├── VERSION.json
├── README.txt
├── csharp/                          # Multi-RID NuGet (linux-x64 + win-x64 + win-x86)
│   └── Pumatronix.Itscam.Sdk.<version>.nupkg
├── linux-x64/
│   ├── cpp/
│   │   ├── include/                 # Public headers (.h, .hpp)
│   │   └── lib/                     # libitscam_sdk.so + symlinks
│   ├── c/
│   │   ├── include/c_api/           # C API headers
│   │   └── lib/                     # same .so files
│   ├── python/
│   │   └── itscam-<version>.whl     # wheel with native lib bundled
│   └── go/
│       └── itscam-sdk-go/           # Go module with native lib in native/
├── win-x64/                         # same sub-structure (.dll + .a)
└── win-x86/                         # same, 32-bit
```

## Extract the SDK

```bash
tar xzf itscam-sdk-<version>.tar.gz
cd itscam-sdk-<version>
```

In the examples below, `$SDK` points to the extracted root directory (e.g. `/opt/itscam-sdk-1.0.0`).

## Integration by language

### C++ / C

```bash
# Compile and link against the pre-compiled shared library:
g++ -std=c++17 \
    -I$SDK/linux-x64/cpp/include \
    -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L$SDK/linux-x64/cpp/lib -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN' \
    -o your_app

# Copy the shared library next to your binary (or configure rpath / LD_LIBRARY_PATH):
cp $SDK/linux-x64/cpp/lib/libitscam_sdk.so* ./
```

Umbrella header: `#include "itscam_sdk.h"` pulls in the entire public surface.

For the C API (FFI from other languages): use the headers under `$SDK/linux-x64/c/include/c_api/`.

### C# / .NET

The NuGet package already includes native binaries for all supported platforms:

```bash
# Create a project and add the NuGet from the SDK directory:
dotnet new console -n MyApp -o my-app && cd my-app
dotnet add package Pumatronix.Itscam.Sdk \
    --source $SDK/csharp
```

The MSBuild target file in the NuGet automatically copies the correct native binary to the build output.

### Python

```bash
pip install $SDK/linux-x64/python/itscam-*.whl
python -c "import itscam; print(itscam.get_version())"
```

The wheel bundles `libitscam_sdk.so` -- no `LD_LIBRARY_PATH` configuration is needed.

### Go

The Go module inside the SDK already includes the native lib and headers with pre-configured cgo directives:

```bash
# In your project's go.mod, point to the local SDK module:
go mod edit -require=github.com/pumatronix/itscam-sdk-go@v0.0.0
go mod edit -replace=github.com/pumatronix/itscam-sdk-go=$SDK/linux-x64/go/itscam-sdk-go

# Dynamic linking (simplest):
LD_LIBRARY_PATH=$SDK/linux-x64/go/itscam-sdk-go/native \
    go run main.go 192.168.254.254

# Static linking (no runtime dependency):
CGO_CFLAGS="-I$SDK/linux-x64/go/itscam-sdk-go/include" \
    go build -tags static -o my-app main.go
```

## Run a quick example

The step-by-step tutorials create a project from scratch and save the first camera image to disk:

- [C++](tutorials/first-image-cpp.md)
- [C# / .NET](tutorials/first-image-csharp.md)
- [Python](tutorials/first-image-python.md)
- [Go](tutorials/first-image-go.md)

## Runtime dependencies

The SDK has no external runtime dependencies. **mbedTLS 3.6 LTS** is statically linked into the native libraries, so HTTPS works out of the box without OpenSSL or any other system TLS library.

| Dependency       | Version   | Status                                 |
| ---------------- | --------- | -------------------------------------- |
| cpp-httplib      | 0.31.0    | Embedded in the binary (build-time)    |
| nlohmann/json    | single h  | Embedded in the binary (build-time)    |
| mbedTLS          | 3.6.2 LTS | Statically linked in `.so` / `.dll`    |

See [`docs/https-tls.md`](https-tls.md) for production TLS configuration.

---

## Building the SDK from source (advanced)

If you need to **build the SDK from scratch** -- for example, to contribute to the project, cross-compile for a different platform, or debug the native core -- follow the instructions below.

### Build prerequisites

- Docker Engine (recommended path).
- Alternatively, for native builds: a C++17 compiler (GCC 7+, Clang 5+, MinGW-w64 for Windows) and GNU `make`.
- Optional: `dotnet` (C# wrapper), `go` (Go wrapper), `python3` (Python wrapper) -- only required on the host if you are **not** using the Docker builder.

All C++ dependencies (cpp-httplib, nlohmann/json, mbedTLS) are vendored under [`src/core/3rdparty/`](../src/core/3rdparty), so no system packages are required.

### Build via Docker (recommended)

```bash
git clone https://github.com/pumatronix/itscam-sdk.git
cd itscam-sdk

make docker-all        # everything: Linux + Windows cross + wrappers
make docker-linux      # libitscam_sdk.{so,a} for Linux only
make docker-windows    # cross-compile itscam_sdk.dll with MinGW
make docker-shell      # interactive shell inside the builder image
make help              # list every target (docker-* and native)
```

Artefacts land under `src/core/build/<platform>/` and `src/examples/build/`, mounted on the host via a volume.

### Native build

If you already have GCC/Clang locally and prefer to skip Docker:

```bash
make lib            # build libitscam_sdk.{so,a} for Linux
make windows        # cross-compile itscam_sdk.dll (MinGW)
make examples       # build the four C++ example binaries
make all            # everything (Linux + Windows + wrappers)
```

### Generate the distribution package

After building everything, package the pre-compiled SDK:

```bash
make sdk-dist           # produces dist/itscam-sdk-<version>.tar.gz
make docker-sdk-dist    # same, inside Docker
```

### Run examples from the source tree

```bash
# Binary client (rpath finds libitscam_sdk.so)
./src/examples/build/itscam_sdk_example 192.168.254.254

# REST client (login required)
./src/examples/build/itscam_rest_example 192.168.254.254 admin 1234

# CGI client (auth optional; configCgi.blockAPI=false by default)
./src/examples/build/itscam_cgi_example 192.168.254.254
./src/examples/build/itscam_cgi_example 192.168.254.254 --https --insecure \
    --user admin --password 1234
```

### Link against the source tree

If you are developing inside the repository checkout, you can link directly against the build artefacts:

#### Against the shared library

```bash
g++ -std=c++17 -Isrc/core/ -Isrc/core/3rdparty/ -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L./src/core/build/linux/ -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN:./src/core/build/linux' \
    -o your_app
```

#### Static, self-contained binary

```bash
g++ -std=c++17 \
    -Isrc/core/ -Isrc/core/3rdparty/ \
    src/core/itscam_client.cpp \
    src/core/itscam_rest_client.cpp \
    src/core/itscam_cgi_client.cpp \
    src/core/impl/*.cpp \
    your_app.cpp \
    -o your_app -lpthread
```

(For static builds you must also compile and link the vendored mbedTLS sources. The top-level `make lib` already handles that, so prefer the shared-library workflow unless you have a specific reason to avoid it.)
