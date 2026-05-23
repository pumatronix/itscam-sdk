# ITSCAM SDK Build Environment
#
# Dockerfile for building the SDK core library (Linux/Windows)
# and the supported wrapper examples (Python and Go).
#
# Usage:
#   docker build -t itscam-sdk-builder .
#   docker run --rm -v $(pwd):/sdk itscam-sdk-builder make all
#   docker run --rm -v $(pwd):/sdk itscam-sdk-builder make windows
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
#  Environment setup
# =============================================================================

# Set Go environment (Go already installed at /usr/local/go)
ENV GOPATH=/go
ENV GO111MODULE=on
ENV PATH=/usr/local/go/bin:/go/bin:/opt/node/bin:$PATH

# Install Wails CLI for Go GUI builds (install to shared /go/bin)
RUN GOBIN=/go/bin go install github.com/wailsapp/wails/v2/cmd/wails@${WAILS_VERSION}

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

# =============================================================================
#  Build targets
# =============================================================================

# Default command - build everything
CMD ["make", "all"]
