#!/usr/bin/env bash
#
# Stage pre-built native binaries from src/core/build/<rid>/ into the
# Node.js wrapper's native/<platform>-<arch>/ subdirectory so that
# `npm pack` includes them in the published tarball.
#
# Layout produced:
#   src/wrappers/nodejs/native/
#       linux-x64/libitscam_sdk.so
#       linux-arm64/libitscam_sdk.so
#       linux-arm/libitscam_sdk.so
#       win32-x64/itscam_sdk.dll
#       win32-ia32/itscam_sdk.dll
#       darwin-x64/libitscam_sdk.dylib
#       darwin-arm64/libitscam_sdk.dylib
#
# Naming follows process.platform / process.arch so the koffi loader
# finds the right binary at runtime without any config.
#
# Copyright (c) 2026 Pumatronix

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/src/core/build"
DST="$ROOT/src/wrappers/nodejs/native"

stage() {
    local from="$1"
    local to="$2"
    if [ -e "$from" ]; then
        mkdir -p "$(dirname "$to")"
        cp -L "$from" "$to"
        echo "  staged $(basename "$to") -> $(realpath --relative-to="$ROOT" "$to")"
    fi
}

rm -rf "$DST"
mkdir -p "$DST"

# Linux x64
for sofile in "$SRC"/linux/libitscam_sdk.so*; do
    [ -e "$sofile" ] && stage "$sofile" "$DST/linux-x64/libitscam_sdk.so" && break
done

# Linux ARMv7 (process.arch === 'arm')
for sofile in "$SRC"/linux-arm/libitscam_sdk.so*; do
    [ -e "$sofile" ] && stage "$sofile" "$DST/linux-arm/libitscam_sdk.so" && break
done

# Linux ARM64 (process.arch === 'arm64')
for sofile in "$SRC"/linux-arm64/libitscam_sdk.so*; do
    [ -e "$sofile" ] && stage "$sofile" "$DST/linux-arm64/libitscam_sdk.so" && break
done

# Windows x64 (process.platform === 'win32', process.arch === 'x64')
stage "$SRC/win-x64/itscam_sdk.dll" "$DST/win32-x64/itscam_sdk.dll"
# Windows x86 (process.arch === 'ia32')
stage "$SRC/win-x86/itscam_sdk.dll" "$DST/win32-ia32/itscam_sdk.dll"

echo "Node.js wrapper natives staged under $DST"
