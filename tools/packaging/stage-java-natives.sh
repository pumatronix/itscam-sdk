#!/usr/bin/env bash
#
# Stage pre-built native binaries from src/core/build/<rid>/ into the
# Java wrapper's resources tree so `mvn package` embeds them into the JAR.
#
# Layout produced:
#   src/wrappers/java/itscam-sdk/src/main/resources/META-INF/native/
#       linux-x86_64/libitscam_sdk.so
#       linux-aarch64/libitscam_sdk.so
#       linux-arm/libitscam_sdk.so
#       windows-x86_64/itscam_sdk.dll
#       windows-x86/itscam_sdk.dll
#
# The Java NativeLibrary loader extracts the first match for the
# running JVM's os/arch.  Missing source binaries are silently skipped
# so partial builds (Linux only, no Windows cross-compile, etc.) still
# produce a usable JAR.
#
# Copyright (c) 2026 Pumatronix

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/src/core/build"
DST="$ROOT/src/wrappers/java/itscam-sdk/src/main/resources/META-INF/native"

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

# Linux x86_64
for sofile in "$SRC"/linux/libitscam_sdk.so*; do
    if [ -e "$sofile" ]; then
        stage "$sofile" "$DST/linux-x86_64/libitscam_sdk.so"
        break
    fi
done

# Linux ARMv7 (32-bit)
for sofile in "$SRC"/linux-arm/libitscam_sdk.so*; do
    if [ -e "$sofile" ]; then
        stage "$sofile" "$DST/linux-arm/libitscam_sdk.so"
        break
    fi
done

# Linux ARM64
for sofile in "$SRC"/linux-arm64/libitscam_sdk.so*; do
    if [ -e "$sofile" ]; then
        stage "$sofile" "$DST/linux-aarch64/libitscam_sdk.so"
        break
    fi
done

# Windows x86_64
stage "$SRC/win-x64/itscam_sdk.dll" "$DST/windows-x86_64/itscam_sdk.dll"
# Windows x86 (32-bit)
stage "$SRC/win-x86/itscam_sdk.dll" "$DST/windows-x86/itscam_sdk.dll"

echo "Java wrapper natives staged under $DST"
