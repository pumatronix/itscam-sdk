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

FROM ubuntu:20.04

LABEL maintainer="Pumatronix"
LABEL description="Build environment for ITSCAM SDK"

ARG GO_VERSION=1.25.6
ARG GO_SHA256=f022b6aad78e362bcba9b0b94d09ad58c5a70c6ba3b7582905fababf5fe0181a
ARG WAILS_VERSION=v2.11.0
ARG NODE_VERSION=20.18.2
ARG NODE_SHA256=4e50f727ae09bdafecf2322c72faf7cd82bf3b8851a16b8bb63974e0d8d6eceb
ARG MAVEN_VERSION=3.9.9
ARG MAVEN_SHA512=a555254d6b53d267965a3404ecb14e53c3827c09c3b94b5678835887ab404556bfaf78dcfe03ba76fa2508649dca8531c74bca4d5846513522404d48e8c4ac8b

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# =============================================================================
#  System packages and build tools
# =============================================================================

RUN apt-get update && apt-get install -y --no-install-recommends \
    # Essential build tools
    build-essential \
    make \
    cmake \
    pkg-config \
    # MinGW for Windows cross-compilation
    mingw-w64 \
    g++-mingw-w64-x86-64 \
    g++-mingw-w64-i686 \
    # Python for Python wrapper
    python3 \
    python3-pip \
    python3-dev \
    # OpenJDK 11 for the Java wrapper (JNA-based; Maven installed below)
    openjdk-11-jdk-headless \
    # WebKit2GTK for Wails GUI (Go GUI example)
    libwebkit2gtk-4.0-dev \
    libgtk-3-dev \
    # Utilities
    curl \
    git \
    wget \
    gpg \
    ca-certificates \
    libicu66 \
    # Documentation generators (Doxygen for C/C++ API ref).
    # DocFX (C#), pdoc (Python), and gomarkdoc (Go) are installed below
    # alongside their respective toolchains.
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
#  Install .NET 8 SDK (for the C# wrapper package)
# =============================================================================

RUN set -eux; \
    wget -q https://packages.microsoft.com/config/ubuntu/20.04/packages-microsoft-prod.deb \
        -O /tmp/packages-microsoft-prod.deb; \
    dpkg -i /tmp/packages-microsoft-prod.deb; \
    rm -f /tmp/packages-microsoft-prod.deb; \
    apt-get update; \
    apt-get install -y --no-install-recommends dotnet-sdk-8.0; \
    rm -rf /var/lib/apt/lists/*

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
# Node lives under /opt/node-<ver> and is exposed system-wide via PATH so
# `make codegen` works for any user inside the container.  We do not rely on
# the distribution package because Ubuntu 20.04 ships an outdated Node.
RUN set -eux; \
    curl -fsSLo /tmp/node.tar.xz "https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-x64.tar.xz"; \
    echo "${NODE_SHA256}  /tmp/node.tar.xz" | sha256sum -c -; \
    tar -C /opt -xf /tmp/node.tar.xz; \
    mv /opt/node-v${NODE_VERSION}-linux-x64 /opt/node; \
    rm -f /tmp/node.tar.xz

# =============================================================================
#  Install Maven (for the Java wrapper build)
# =============================================================================
#
# Maven lives under /opt/maven and is exposed via PATH so `make java` and
# `make docker-java-pack` work for any user inside the container.  The
# distribution package on Ubuntu 20.04 is too old for our parent POM,
# so we install the official binary tarball.
RUN set -eux; \
    curl -fsSLo /tmp/maven.tar.gz \
        "https://archive.apache.org/dist/maven/maven-3/${MAVEN_VERSION}/binaries/apache-maven-${MAVEN_VERSION}-bin.tar.gz"; \
    echo "${MAVEN_SHA512}  /tmp/maven.tar.gz" | sha512sum -c -; \
    tar -C /opt -xzf /tmp/maven.tar.gz; \
    ln -sfn "/opt/apache-maven-${MAVEN_VERSION}" /opt/maven; \
    rm -f /tmp/maven.tar.gz

ENV JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
ENV M2_HOME=/opt/maven

# =============================================================================
#  Environment setup
# =============================================================================

# Set Go environment (Go already installed at /usr/local/go)
ENV GOPATH=/go
ENV GO111MODULE=on
ENV PATH=/usr/local/go/bin:/go/bin:/opt/node/bin:/opt/maven/bin:$PATH

# Install Wails CLI for Go GUI builds (install to shared /go/bin)
RUN GOBIN=/go/bin go install github.com/wailsapp/wails/v2/cmd/wails@${WAILS_VERSION}

# gomarkdoc renders the Go wrapper's package documentation as markdown
# so VitePress can serve it natively under /api-ref/go.
RUN GOBIN=/go/bin go install github.com/princjef/gomarkdoc/cmd/gomarkdoc@latest

# Refuse root when /sdk is bind-mounted (see tools/docker/entrypoint.sh).
COPY tools/docker/entrypoint.sh /usr/local/bin/itscam-docker-entrypoint.sh
RUN chmod +x /usr/local/bin/itscam-docker-entrypoint.sh

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
