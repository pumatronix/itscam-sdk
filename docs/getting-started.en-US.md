# Getting started

[Português (Brasil)](getting-started.md) | [English (US)](getting-started.en-US.md)

Five-minute path from a fresh checkout to a running C++ example. The
recommended workflow uses the **Docker builder** because it already
ships GCC, MinGW-w64, .NET 8 SDK, Go 1.25, Python 3, Wails, and the
full cross-compile toolchain.

## Prerequisites

- Docker Engine (recommended path).
- Alternatively, for native builds: a C++17 compiler (GCC 7+, Clang 5+,
  MinGW-w64 for Windows) and GNU `make`.
- Optional: `dotnet` (C# wrapper), `go` (Go wrapper), `python3` (Python
  wrapper) -- only required on the host if you are **not** using the
  Docker builder.

Everything else (cpp-httplib, nlohmann/json, mbedTLS) is vendored under
[`src/core/3rdparty/`](../src/core/3rdparty), so no system packages are
required.

## Build via Docker (recommended)

```bash
git clone https://github.com/pumatronix/itscam-sdk.git
cd itscam-sdk

make docker-all        # everything: Linux + Windows cross + wrappers
make docker-linux      # libitscam_sdk.{so,a} for Linux only
make docker-windows    # cross-compile itscam_sdk.dll with MinGW
make docker-shell      # interactive shell inside the builder image
make help              # list every target (docker-* and native)
```

Artefacts land under `src/core/build/<platform>/` and
`src/examples/build/`, mounted on the host via a volume.

## Native build (optional)

If you already have GCC/Clang locally and prefer to skip Docker:

```bash
make lib            # build libitscam_sdk.{so,a} for Linux
make windows        # cross-compile itscam_sdk.dll (MinGW)
make examples       # build the four C++ example binaries
make all            # everything (Linux + Windows + wrappers)
```

## Run a C++ example

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

## Link your own application

### Against the shared library (recommended)

```bash
g++ -std=c++17 -Isrc/core/ -Isrc/core/3rdparty/ -c your_app.cpp -o your_app.o

g++ your_app.o \
    -L./src/core/build/linux/ -litscam_sdk \
    -lpthread \
    -Wl,-rpath,'$ORIGIN:./src/core/build/linux' \
    -o your_app
```

### Static, self-contained binary

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

(For static builds you must also compile and link the vendored
mbedTLS sources. The top-level `make lib` already handles that, so
prefer the shared-library workflow unless you have a specific reason
to avoid it.)

## Dependencies (vendored)

The repo is self-contained -- nothing in the table below needs to be
installed on the host.

| Dependency       | Version   | Location                              |
| ---------------- | --------- | ------------------------------------- |
| cpp-httplib      | 0.31.0    | `src/core/3rdparty/httplib.h`         |
| nlohmann/json    | single h  | `src/core/3rdparty/nlohmann/json.hpp` |
| mbedTLS          | 3.6.2 LTS | `src/core/3rdparty/mbedtls/`          |

See [`docs/https-tls.md`](https-tls.md) for the mbedTLS configuration
and upgrade procedure.
