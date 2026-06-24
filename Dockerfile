# ITSCAM SDK Build Environment
#
# Dockerfile for building the SDK core library (Linux/Windows)
# and the supported wrapper examples (Python, Go, C# / .NET, Java, Node.js).
#
# Usage:
#   docker build -t itscam-sdk-builder .
#   make docker-all          # recommended (passes -u automatically)
#   docker run --rm -v "$(pwd)":/sdk -w /sdk \
#     -u "$(id -u):$(id -g)" itscam-sdk-builder make all
#
# Never run without -u on a bind mount -- files would be root-owned on the host.
#
# Copyright (c) 2026 Pumatronix

FROM ubuntu:18.04

LABEL maintainer="Pumatronix"
LABEL description="Build environment for ITSCAM SDK"

# Anchoring the base on ubuntu:18.04 caps libitscam_sdk.so.* glibc usage
# at GLIBC_2.27 so binaries built here run on every reasonably modern
# Linux distribution (Ubuntu 18.04+, Debian 10+, RHEL 8+, the ITSCAM
# camera images, etc.).  The check is enforced by tools/check-glibc.sh.

ARG GO_VERSION=1.25.6
ARG GO_SHA256=f022b6aad78e362bcba9b0b94d09ad58c5a70c6ba3b7582905fababf5fe0181a
ARG WAILS_VERSION=v2.11.0
ARG NODE_VERSION=20.18.2
# Ubuntu 18.04 ships glibc 2.27, but the official nodejs.org binaries for
# Node 18+ require GLIBC_2.28. Use the nodejs/unofficial-builds glibc-217
# variant, which is built against glibc 2.17 and runs on every glibc >= 2.17.
ARG NODE_SHA256=51d7eb3a14d0e148ced83771495a0b8baf9de634b8de22dd48efdd0633a08822
ARG MAVEN_VERSION=3.9.9
ARG MAVEN_SHA512=a555254d6b53d267965a3404ecb14e53c3827c09c3b94b5678835887ab404556bfaf78dcfe03ba76fa2508649dca8531c74bca4d5846513522404d48e8c4ac8b
ARG ZULU_JDK7_VERSION=7.56.0.11
ARG ZULU_JDK7_JAVA_VERSION=7.0.352
ARG ZULU_JDK7_SHA256=8a7387c1ed151474301b6553c6046f865dc6c1e1890bcf106acc2780c55727c8

# Arm GNU-A 8.3-2019.03 cross-toolchains.  Pinning to this release gives
# the lowest glibc floor among maintained Arm-signed binaries:
#   - GLIBC_2.28 on both armhf and aarch64 targets
#   - kernel headers 4.19
#   - GCC 8.3.0 (full C++17 support)
# Matches the ITSCAM450 firmware toolchain exactly; runs on ITSCAM600 too.
ARG ARM_TOOLCHAIN_VERSION=8.3-2019.03
ARG ARMHF_TOOLCHAIN_SHA256=d4f6480ecaa99e977e3833cc8a8e1263f9eecd1ce2d022bb548a24c4f32670f5
ARG ARM64_TOOLCHAIN_SHA256=8ce3e7688a47d8cd2d8e8323f147104ae1c8139520eca50ccf8a7fa933002731

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# =============================================================================
#  System packages and build tools
# =============================================================================

RUN set -eux; \
    for attempt in 1 2 3 4 5; do \
        apt-get clean; \
        rm -rf /var/lib/apt/lists/*; \
        if apt-get \
            -o Acquire::Retries=5 \
            -o Acquire::http::No-Cache=true \
            -o Acquire::http::Pipeline-Depth=0 \
            update; then \
            break; \
        fi; \
        if [ "$attempt" = 5 ]; then exit 1; fi; \
    done; \
    apt-get install -y --no-install-recommends \
    # Essential build tools
    build-essential \
    make \
    cmake \
    pkg-config \
    # MinGW for Windows cross-compilation
    mingw-w64 \
    g++-mingw-w64-x86-64 \
    g++-mingw-w64-i686 \
    # Python for Python wrapper (bionic 3.6).  A newer interpreter is
    # layered on top via the deadsnakes PPA in the next RUN -- the 3.6
    # binaries here remain available as fallback for any tool that
    # explicitly invokes /usr/bin/python3.6.
    python3 \
    python3-pip \
    python3-dev \
    # OpenJDK 11 builder for Maven; JDK 7 is installed below for compiler verification
    openjdk-11-jdk-headless \
    # WebKit2GTK for Wails GUI (Go GUI example)
    libwebkit2gtk-4.0-dev \
    libgtk-3-dev \
    # ARM-on-x86 emulation (used by tools/qemu-smoke.sh and `make check`)
    qemu-user-static \
    binfmt-support \
    # Utilities
    curl \
    git \
    git-lfs \
    wget \
    gpg \
    xz-utils \
    ca-certificates \
    libicu60 \
    # Documentation generators (Doxygen for C/C++ API ref).
    # DocFX (C#), pdoc (Python), and gomarkdoc (Go) are installed below
    # alongside their respective toolchains.
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
#  Install JDK 7 (for Java wrapper compatibility verification)
# =============================================================================
#
# Maven 3.9.x runs on the OpenJDK 11 installed above, but the Java wrapper's
# minimum supported runtime is JDK 7.  Keep a real JDK 7 toolchain available so
# `make java-jdk7-check` can fork /opt/jdk7/bin/javac and catch accidental usage
# of newer Java APIs or bytecode.
RUN set -eux; \
    curl -fsSLo /tmp/zulu7.tar.gz \
        "https://cdn.azul.com/zulu/bin/zulu${ZULU_JDK7_VERSION}-ca-jdk${ZULU_JDK7_JAVA_VERSION}-linux_x64.tar.gz"; \
    echo "${ZULU_JDK7_SHA256}  /tmp/zulu7.tar.gz" | sha256sum -c -; \
    mkdir -p /opt/jdk7; \
    tar -C /opt/jdk7 --strip-components=1 -xzf /tmp/zulu7.tar.gz; \
    rm -f /tmp/zulu7.tar.gz; \
    /opt/jdk7/bin/java -version; \
    /opt/jdk7/bin/javac -version

ENV JDK7_HOME=/opt/jdk7
ENV JAVA7_HOME=/opt/jdk7

# =============================================================================
#  Install .NET 8 SDK (for the C# wrapper package)
# =============================================================================
#
# Microsoft does not ship dotnet-sdk-8.0 apt packages for Ubuntu 18.04 (bionic);
# the official advice for older glibcs is the portable dotnet-install.sh
# script, which lays down a self-contained SDK under /usr/share/dotnet.
# Runtime requirement is glibc >= 2.23 -- 18.04's 2.27 is fine.

ARG DOTNET_SDK_VERSION=8.0.404
ARG DOTNET_INSTALL_SHA256=082f7685e156738a1b2e2ed8381a621870d4ce8e8c59278034556f05c186eb2e

RUN set -eux; \
    wget -q https://dot.net/v1/dotnet-install.sh -O /tmp/dotnet-install.sh; \
    echo "${DOTNET_INSTALL_SHA256}  /tmp/dotnet-install.sh" | sha256sum -c -; \
    chmod +x /tmp/dotnet-install.sh; \
    /tmp/dotnet-install.sh \
        --version "${DOTNET_SDK_VERSION}" \
        --install-dir /usr/share/dotnet \
        --no-path; \
    ln -s /usr/share/dotnet/dotnet /usr/local/bin/dotnet; \
    rm -f /tmp/dotnet-install.sh

ENV DOTNET_ROOT=/usr/share/dotnet
ENV DOTNET_CLI_TELEMETRY_OPTOUT=1
ENV DOTNET_NOLOGO=1

# DocFX is the C# / .NET API reference generator. Installed under a
# shared tool path so the builder user (created below) can run it too.
RUN dotnet tool install --tool-path /usr/local/dotnet-tools docfx
ENV PATH=/usr/local/dotnet-tools:$PATH

# pdoc generates the Python wrapper's API reference. The package is
# installed system-wide so any user can run it inside the container.
RUN python3 -m pip install --no-cache-dir pdoc

# =============================================================================
#  Install Go 1.25.6 (required for Wails)
# =============================================================================

RUN set -eux; \
    curl -fsSLo /tmp/go.tar.gz "https://go.dev/dl/go${GO_VERSION}.linux-amd64.tar.gz"; \
    echo "${GO_SHA256}  /tmp/go.tar.gz" | sha256sum -c -; \
    tar -C /usr/local -xzf /tmp/go.tar.gz; \
    rm -f /tmp/go.tar.gz

# =============================================================================
#  Install Node.js (for the OpenAPI -> typed REST helpers code generator)
# =============================================================================
#
# Node lives under /opt/node and is exposed system-wide via PATH so
# `make codegen` (and docs-sync) work for any user inside the container.
# We pull from unofficial-builds.nodejs.org because the official builds
# require GLIBC_2.28 (see NODE_SHA256 comment above) and would fail at
# runtime on this bionic base with:
#   node: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.28' not found
RUN set -eux; \
    curl -fsSLo /tmp/node.tar.xz "https://unofficial-builds.nodejs.org/download/release/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-x64-glibc-217.tar.xz"; \
    echo "${NODE_SHA256}  /tmp/node.tar.xz" | sha256sum -c -; \
    tar -C /opt -xf /tmp/node.tar.xz; \
    mv /opt/node-v${NODE_VERSION}-linux-x64-glibc-217 /opt/node; \
    rm -f /tmp/node.tar.xz

# =============================================================================
#  Install Maven (for the Java wrapper build)
# =============================================================================
#
# Maven lives under /opt/maven and is exposed via PATH so `make java` and
# `make docker-java-pack` work for any user inside the container.  The
# distribution package on Ubuntu 20.04 is too old for our parent POM,
# so we install the official binary tarball. Maven Central mirrors the
# Apache release artifact and is faster/more reliable than archive.apache.org.
RUN set -eux; \
    curl -fsSLo /tmp/maven.tar.gz \
        "https://repo.maven.apache.org/maven2/org/apache/maven/apache-maven/${MAVEN_VERSION}/apache-maven-${MAVEN_VERSION}-bin.tar.gz"; \
    echo "${MAVEN_SHA512}  /tmp/maven.tar.gz" | sha512sum -c -; \
    tar -C /opt -xzf /tmp/maven.tar.gz; \
    ln -sfn "/opt/apache-maven-${MAVEN_VERSION}" /opt/maven; \
    rm -f /tmp/maven.tar.gz

ENV JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
ENV M2_HOME=/opt/maven

# =============================================================================
#  Install Arm GNU-A 8.3-2019.03 cross-toolchains (armhf + aarch64)
# =============================================================================
#
# Tarballs land under /opt/cross/{armhf,arm64} so they match the
# ARMHF_TOOLCHAIN_PATH / ARM64_TOOLCHAIN_PATH defaults in
# src/core/Makefile (`/opt/cross/<arch>/bin`).  Both toolchains share
# the same release so the glibc / kernel-header floor is identical
# (GLIBC_2.28, kernel 4.19) across armhf and aarch64.
#
# Note on the download URL: the canonical link
#   https://developer.arm.com/-/media/Files/downloads/gnu-a/<ver>/binrel/<file>
# is a 302 redirect to the actual Azure blob below.  We hit the blob
# directly so the build does not depend on the developer.arm.com redirect
# tier staying up (it has gone away in the past).
#
# Each toolchain is downloaded in its own RUN so Docker caches them
# independently -- a Ctrl-C during the aarch64 download still keeps
# the (already much larger) armhf layer cached on the next attempt.
RUN set -eux; \
    base="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/${ARM_TOOLCHAIN_VERSION}/binrel"; \
    curl -fsSLo /tmp/armhf.tar.xz \
        "$base/gcc-arm-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-linux-gnueabihf.tar.xz"; \
    echo "${ARMHF_TOOLCHAIN_SHA256}  /tmp/armhf.tar.xz" | sha256sum -c -; \
    mkdir -p /opt/cross/armhf; \
    tar -C /opt/cross/armhf --strip-components=1 -xf /tmp/armhf.tar.xz; \
    rm -f /tmp/armhf.tar.xz

RUN set -eux; \
    base="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/${ARM_TOOLCHAIN_VERSION}/binrel"; \
    curl -fsSLo /tmp/arm64.tar.xz \
        "$base/gcc-arm-${ARM_TOOLCHAIN_VERSION}-x86_64-aarch64-linux-gnu.tar.xz"; \
    echo "${ARM64_TOOLCHAIN_SHA256}  /tmp/arm64.tar.xz" | sha256sum -c -; \
    mkdir -p /opt/cross/arm64; \
    tar -C /opt/cross/arm64 --strip-components=1 -xf /tmp/arm64.tar.xz; \
    rm -f /tmp/arm64.tar.xz

ENV ARMHF_TOOLCHAIN_PATH=/opt/cross/armhf/bin
ENV ARM64_TOOLCHAIN_PATH=/opt/cross/arm64/bin

# =============================================================================
#  Environment setup
# =============================================================================

# Set Go environment (Go already installed at /usr/local/go)
ENV GOPATH=/go
ENV GO111MODULE=on
ENV PATH=/usr/local/go/bin:/go/bin:/opt/node/bin:/opt/maven/bin:/opt/cross/armhf/bin:/opt/cross/arm64/bin:$PATH

# Install Wails CLI for Go GUI builds (install to shared /go/bin)
RUN GOBIN=/go/bin go install github.com/wailsapp/wails/v2/cmd/wails@${WAILS_VERSION}

# gomarkdoc renders the Go wrapper's package documentation as markdown
# so VitePress can serve it natively under /api-ref/go.
RUN GOBIN=/go/bin go install github.com/princjef/gomarkdoc/cmd/gomarkdoc@latest

# Refuse root when /sdk is bind-mounted (see tools/docker/entrypoint.sh).
COPY tools/docker/entrypoint.sh /usr/local/bin/itscam-docker-entrypoint.sh
RUN chmod +x /usr/local/bin/itscam-docker-entrypoint.sh

# =============================================================================
#  Install Python 3.11 (built from source on top of bionic)
# =============================================================================
#
# Bionic's stock python3 is 3.6, which lacks `from __future__ import
# annotations` (3.7+) and `subprocess(..., text=True)` (3.7+) used by
# tools/version/gen-version.sh and our codegen post-processors.
# Deadsnakes no longer ships runtime packages for bionic, so we compile
# CPython 3.11 from the upstream source tarball and link it against
# bionic's system libraries (OpenSSL 1.1, zlib, libffi, sqlite3, ...).
# The result lives under /usr/local/{bin,lib,...} and overrides the
# stock python3 via PATH order.
#
# This layer lives at the end of the image (after the heavy .NET, Go,
# Node, Maven and Arm cross-toolchain downloads) so a future change to
# the Python install does not invalidate the multi-hundred-MB cache
# layers above.
ARG PYTHON_VERSION=3.11.10
ARG PYTHON_SHA256=07a4356e912900e61a15cb0949a06c4a05012e213ecd6b4e84d0f67aabbee372

RUN set -eux; \
    # Build-time deps for a featureful CPython (ssl, hashlib, lzma,
    # sqlite3, ctypes, readline, tkinter-free).  Headers stay in the
    # final image so site-packages with C extensions (the SDK's wheel,
    # the codegen scripts' deps) can compile against this interpreter.
    for attempt in 1 2 3 4 5; do \
        apt-get clean; \
        rm -rf /var/lib/apt/lists/*; \
        if apt-get \
            -o Acquire::Retries=5 \
            -o Acquire::http::No-Cache=true \
            -o Acquire::http::Pipeline-Depth=0 \
            update; then \
            break; \
        fi; \
        if [ "$attempt" = 5 ]; then exit 1; fi; \
    done; \
    apt-get install -y --no-install-recommends \
        libssl-dev \
        libffi-dev \
        zlib1g-dev \
        libbz2-dev \
        liblzma-dev \
        libsqlite3-dev \
        libreadline-dev \
        libncursesw5-dev \
        uuid-dev \
        libgdbm-dev; \
    rm -rf /var/lib/apt/lists/*; \
    \
    curl -fsSLo /tmp/python.tar.xz \
        "https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tar.xz"; \
    echo "${PYTHON_SHA256}  /tmp/python.tar.xz" | sha256sum -c -; \
    mkdir -p /tmp/python-build; \
    tar -C /tmp/python-build --strip-components=1 -xf /tmp/python.tar.xz; \
    rm -f /tmp/python.tar.xz; \
    cd /tmp/python-build; \
    ./configure \
        --prefix=/usr/local \
        --enable-optimizations \
        --with-lto \
        --with-ensurepip=install \
        --with-system-ffi \
        --enable-shared \
        LDFLAGS="-Wl,-rpath,/usr/local/lib"; \
    make -j"$(nproc)"; \
    make altinstall; \
    cd /; \
    rm -rf /tmp/python-build; \
    # `make altinstall` installs only python3.11 / pip3.11; wire the
    # generic names under /usr/local/bin (first on $PATH) so every
    # `python3` / `pip3` invocation in the build picks up 3.11.
    ln -sf /usr/local/bin/python3.11 /usr/local/bin/python3; \
    ln -sf /usr/local/bin/python3.11 /usr/local/bin/python; \
    ln -sf /usr/local/bin/pip3.11    /usr/local/bin/pip3; \
    ln -sf /usr/local/bin/pip3.11    /usr/local/bin/pip; \
    ldconfig; \
    python3 --version; \
    pip3 --version

# Re-install pdoc against the newer interpreter (the earlier `pip install
# pdoc` ran against bionic's 3.6; the swap above hid those site-packages).
# `wheel` is needed by tools/packaging/make-sdk-dist.sh to produce the
# manylinux wheels for the Python wrapper distribution.
RUN python3 -m pip install --no-cache-dir pdoc wheel

# =============================================================================
#  Non-root user setup
# =============================================================================

# Create a non-root user with configurable UID/GID
# Default to 1000:1000, can be overridden at build time
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid ${USER_GID} builder \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m builder \
    && mkdir -p /sdk /go \
    && chown -R builder:builder /sdk /go

# Switch to non-root user
USER builder

# Working directory
WORKDIR /sdk

ENTRYPOINT ["/usr/local/bin/itscam-docker-entrypoint.sh"]

# =============================================================================
#  Build targets
# =============================================================================

# Default command - build everything
CMD ["make", "all"]
