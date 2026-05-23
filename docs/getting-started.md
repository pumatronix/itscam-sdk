# Getting Started

Five-minute path from a fresh checkout to a running C++ example.

## Prerequisites

- A C++14 compiler (GCC 5+, Clang 3.4+, MinGW-w64 for Windows builds).
- GNU `make`.
- Optional: `dotnet` (for the C# wrapper), `go` (for the Go wrapper),
  `python3` (for the Python wrapper), and `docker` (for the reproducible
  builder image).

Everything else (cpp-httplib, nlohmann/json, mbedTLS) is vendored under
[`src/core/3rdparty/`](../src/core/3rdparty), so no system packages are
required.

## Build the core library

```bash
git clone https://github.com/pumatronix/itscam-sdk.git
cd itscam-sdk

make lib            # build libitscam_sdk.{so,a} for Linux
make windows        # cross-compile itscam_sdk.dll with MinGW
make all            # build everything (Linux + Windows + wrappers)
make help           # list every target
```

Artefacts land under `src/core/build/<platform>/`.

## Build and run a C++ example

```bash
make examples       # builds src/examples/build/itscam_{sdk,rest,cgi}_example
                    # and itscam_trigger_recorder

# Run the binary client example (rpath is set to find libitscam_sdk.so)
./src/examples/build/itscam_sdk_example 192.168.254.254

# REST client example
./src/examples/build/itscam_rest_example 192.168.254.254 admin 1234

# CGI client example (auth optional; configCgi.blockAPI=false by default)
./src/examples/build/itscam_cgi_example 192.168.254.254
./src/examples/build/itscam_cgi_example 192.168.254.254 --https --insecure \
    --user admin --password 1234
```

## Link your own application

### Against the shared library (recommended)

```bash
# Compile (headers only -- no SDK sources)
g++ -std=c++17 -Isrc/core/ -Isrc/core/3rdparty/ -c your_app.cpp -o your_app.o

# Link against the shared library, baking in an rpath so libitscam_sdk.so.1
# resolves at runtime.
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

(When building statically you also need to compile and link the vendored
mbedTLS sources -- the top-level `make lib` already does this for you, so
prefer the shared-library workflow above unless you have a specific
reason to avoid it.)

## Docker

A reproducible toolchain ships in [`Dockerfile`](../Dockerfile).  It
provides GCC, MinGW-w64, .NET 8 SDK, Go 1.25, Wails and the libwebkit
dependencies needed for the GUI example.

```bash
make docker-build               # build the builder image
make docker-all                 # run `make all` inside the container
make docker-linux               # build only the Linux library
make docker-windows             # cross-compile for Windows
make docker-shell               # interactive shell inside the container
```

## Dependencies (vendored)

The repo is self-contained; nothing below needs to be installed on the
host.

| Dependency       | Version   | Location                              |
| ---------------- | --------- | ------------------------------------- |
| cpp-httplib      | 0.31.0    | `src/core/3rdparty/httplib.h`         |
| nlohmann/json    | single h  | `src/core/3rdparty/nlohmann/json.hpp` |
| mbedTLS          | 3.6.2 LTS | `src/core/3rdparty/mbedtls/`          |

See [`docs/https-tls.md`](https-tls.md) for the mbedTLS configuration
and upgrade procedure.
