#!/bin/bash
#
# Build script for ITSCAM Viewer - Ubuntu 20.04
#
# This script builds the Wails application for Linux.
# It can be run standalone or inside the SDK Docker container.
#
# Prerequisites (when running standalone):
#   - Go 1.21+
#   - Wails CLI (go install github.com/wailsapp/wails/v2/cmd/wails@latest)
#   - WebKit2GTK development libraries
#   - ITSCAM SDK native library (libitscam_sdk.so)
#
# Usage:
#   ./build-linux.sh
#
# Or using Docker from SDK root:
#   make docker-go-gui
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Determine SDK root path (either relative or from /sdk in container).
# All native source lives under $SDK_ROOT/src/core/.
if [ -d "/sdk/src/core" ]; then
    SDK_ROOT="/sdk"
else
    SDK_ROOT="$(cd ../../../../.. && pwd)"
fi

echo "=== ITSCAM Viewer - Linux Build ==="
echo "SDK root: $SDK_ROOT"
echo ""

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v go &> /dev/null; then
    echo "Error: Go is not installed"
    echo "Install with: sudo snap install go --classic"
    exit 1
fi

if ! command -v wails &> /dev/null; then
    echo "Error: Wails CLI is not installed"
    echo "Install with: go install github.com/wailsapp/wails/v2/cmd/wails@latest"
    exit 1
fi

# Check for WebKit2GTK (skip in container where it's pre-installed)
if ! pkg-config --exists webkit2gtk-4.0 2>/dev/null; then
    echo "Error: WebKit2GTK development libraries not found"
    echo "Install with: sudo apt install libwebkit2gtk-4.0-dev"
    exit 1
fi

# Check for ITSCAM SDK
SDK_LIB_PATH="$SDK_ROOT/src/core/build/linux"
if [ ! -f "$SDK_LIB_PATH/libitscam_sdk.so" ]; then
    echo "Warning: ITSCAM SDK library not found at $SDK_LIB_PATH"
    echo "Building SDK first..."
    (cd "$SDK_ROOT" && make linux)
fi

# Set library path for build
export CGO_LDFLAGS="-L$SDK_LIB_PATH"
export CGO_CFLAGS="-I$SDK_ROOT/src/core"
export LD_LIBRARY_PATH="$SDK_LIB_PATH:$LD_LIBRARY_PATH"

echo "Go version: $(go version)"
echo "Wails version: $(wails version 2>&1 | head -1)"
echo ""

# Download dependencies
echo "Downloading dependencies..."
go mod tidy

# Build the application
echo "Building application..."
mkdir -p build/bin

wails build -clean -o itscam-viewer

echo ""
echo "=== Build complete ==="
echo "Binary: build/bin/itscam-viewer"
echo ""
echo "To run, ensure the SDK library is in your library path:"
echo "  export LD_LIBRARY_PATH=$SDK_LIB_PATH:\$LD_LIBRARY_PATH"
echo "  ./build/bin/itscam-viewer"
