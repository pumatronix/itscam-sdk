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
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
#  Install Go 1.25.6 (required for Wails)
# =============================================================================

RUN set -eux; \
    curl -fsSLo /tmp/go.tar.gz "https://go.dev/dl/go${GO_VERSION}.linux-amd64.tar.gz"; \
    echo "${GO_SHA256}  /tmp/go.tar.gz" | sha256sum -c -; \
    tar -C /usr/local -xzf /tmp/go.tar.gz; \
    rm -f /tmp/go.tar.gz

# =============================================================================
#  Environment setup
# =============================================================================

# Set Go environment (Go already installed at /usr/local/go)
ENV GOPATH=/go
ENV GO111MODULE=on
ENV PATH=/usr/local/go/bin:/go/bin:$PATH

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
