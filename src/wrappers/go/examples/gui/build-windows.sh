#!/bin/bash
#
# Build script for ITSCAM Viewer - Windows (cross-compile from Linux)
#
# This script cross-compiles the Wails application for Windows from Linux.
# It can be run standalone or inside the SDK Docker container.
#
# Prerequisites (when running standalone):
#   - Go 1.21+
#   - Wails CLI (go install github.com/wailsapp/wails/v2/cmd/wails@latest)
#   - MinGW-w64 (for cross-compilation)
#   - ITSCAM SDK Windows library (itscam_sdk.dll)
#
# Usage:
#   ./build-windows.sh
#
# Or using Docker from SDK root:
#   make docker-go-gui-windows
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

echo "=== ITSCAM Viewer - Windows Cross-Compile Build ==="
echo "SDK root: $SDK_ROOT"
echo ""

# Check prerequisites
echo "Checking prerequisites..."

if ! command -v go &> /dev/null; then
    echo "Error: Go is not installed"
    exit 1
fi

if ! command -v wails &> /dev/null; then
    echo "Error: Wails CLI is not installed"
    echo "Install with: go install github.com/wailsapp/wails/v2/cmd/wails@latest"
    exit 1
fi

if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "Error: MinGW-w64 is not installed"
    echo "Install with: sudo apt install gcc-mingw-w64-x86-64"
    exit 1
fi

# Check for ITSCAM SDK Windows library
SDK_LIB_PATH="$SDK_ROOT/src/core/build/windows"
if [ ! -f "$SDK_LIB_PATH/itscam_sdk.dll" ]; then
    echo "Warning: ITSCAM SDK Windows library not found at $SDK_LIB_PATH"
    echo "Building SDK for Windows first..."
    (cd "$SDK_ROOT" && make windows)
fi

echo "Go version: $(go version)"
echo "Wails version: $(wails version 2>&1 | head -1)"
echo ""

# Download dependencies
echo "Downloading dependencies..."
go mod tidy

# Build the application for Windows
echo "Building application for Windows..."
mkdir -p build/bin

# Set environment for cross-compilation
export CGO_ENABLED=1
export GOOS=windows
export GOARCH=amd64
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
export CGO_LDFLAGS="-L$SDK_LIB_PATH"
export CGO_CFLAGS="-I$SDK_ROOT/src/core"

wails build -clean -platform windows/amd64 -o itscam-viewer.exe

# Copy the SDK DLL to the output directory
if [ -f "$SDK_LIB_PATH/itscam_sdk.dll" ]; then
    cp "$SDK_LIB_PATH/itscam_sdk.dll" build/bin/
    echo "Copied itscam_sdk.dll to build/bin/"
fi

echo ""
echo "=== Build complete ==="
echo "Binary: build/bin/itscam-viewer.exe"
echo ""
echo "Note: To run on Windows, ensure itscam_sdk.dll is in the same directory"
echo "as the executable or in the system PATH."
