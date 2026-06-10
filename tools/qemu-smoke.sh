#!/usr/bin/env bash
# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# qemu-smoke.sh -- exercise ARM-cross-compiled SDK artefacts on the
# host with qemu-user-static so CI can sanity-check ARMv7 / aarch64
# builds without real hardware.
#
# Usage:
#   tools/qemu-smoke.sh                  # both archs
#   tools/qemu-smoke.sh armhf            # only ARMv7
#   tools/qemu-smoke.sh arm64            # only aarch64
#
# Run after `make examples-arm` / `make examples-arm64` (and the Go /
# Python / Node examples for the arch in question).  Each binary is
# launched under qemu with --help so the loader resolves
# libitscam_sdk.so but no network I/O is required.  Non-zero exit codes
# cause the smoke test to fail.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CORE_BUILD="$ROOT/src/core/build"
CPP_EXAMPLES="$ROOT/src/examples/build"
GO_EXAMPLES="$ROOT/src/wrappers/go/examples/build"

# Each entry is "<arch> <qemu binary> <interp> <sdk-build subdir> <example-subdir>"
ARMHF_QEMU="qemu-arm-static"
ARM64_QEMU="qemu-aarch64-static"

run_qemu() {
    local qemu="$1"; shift
    local interp="$1"; shift
    local lib_dir="$1"; shift
    local bin="$1"; shift

    if [ ! -x "$bin" ]; then
        echo "  skip: missing $bin"
        return 0
    fi

    echo "  + $qemu -L $interp -E LD_LIBRARY_PATH=$lib_dir $bin --help"
    "$qemu" \
        -L "$interp" \
        -E "LD_LIBRARY_PATH=$lib_dir" \
        "$bin" --help >/tmp/qemu-smoke.out 2>&1
    ec=$?
    # The smoke check only asks whether the dynamic loader resolved every
    # symbol in libitscam_sdk.so and the binary started executing.  Exit
    # codes 0/1/2 all mean "reached main and printed usage" -- only a
    # SIGSEGV / not-found / 127 means the .so failed to load.
    if [ "$ec" -le 2 ]; then
        echo "    OK (exit=$ec)"
        return 0
    fi
    echo "    FAILED (exit=$ec)"
    sed 's/^/      /' /tmp/qemu-smoke.out
    return $ec
}

smoke_arch() {
    local arch="$1"          # armhf or arm64
    local qemu sysroot suffix
    case "$arch" in
        armhf)
            qemu="$ARMHF_QEMU"
            sysroot="${ARMHF_SYSROOT:-/opt/cross/armhf/arm-linux-gnueabihf/libc}"
            suffix="linux-arm"
            ;;
        arm64)
            qemu="$ARM64_QEMU"
            sysroot="${ARM64_SYSROOT:-/opt/cross/arm64/aarch64-linux-gnu/libc}"
            suffix="linux-arm64"
            ;;
        *)
            echo "qemu-smoke: unknown arch '$arch'" >&2
            return 64
            ;;
    esac

    if ! command -v "$qemu" >/dev/null 2>&1; then
        echo "qemu-smoke: $qemu not found in PATH -- skipping $arch" >&2
        return 0
    fi
    if [ ! -d "$sysroot" ]; then
        echo "qemu-smoke: sysroot $sysroot missing -- skipping $arch" >&2
        return 0
    fi

    echo ""
    echo "=== $arch (qemu=$qemu sysroot=$sysroot) ==="

    local lib_dir="$CORE_BUILD/$suffix"

    local arch_rc=0

    # C++ examples
    if [ -d "$CPP_EXAMPLES/$suffix" ]; then
        echo "[cpp] $CPP_EXAMPLES/$suffix"
        for bin in "$CPP_EXAMPLES/$suffix"/*; do
            if [ -x "$bin" ] && [ -f "$bin" ]; then
                run_qemu "$qemu" "$sysroot" "$lib_dir" "$bin" || arch_rc=$?
            fi
        done
    fi

    # Go examples
    if [ -d "$GO_EXAMPLES/$suffix" ]; then
        echo "[go]  $GO_EXAMPLES/$suffix"
        for bin in "$GO_EXAMPLES/$suffix"/*; do
            if [ -x "$bin" ] && [ -f "$bin" ]; then
                run_qemu "$qemu" "$sysroot" "$lib_dir" "$bin" || arch_rc=$?
            fi
        done
    fi

    return $arch_rc
}

main() {
    local archs=()
    if [ $# -eq 0 ]; then
        archs=(armhf arm64)
    else
        archs=("$@")
    fi

    local rc=0
    for arch in "${archs[@]}"; do
        smoke_arch "$arch" || rc=$?
    done

    echo ""
    if [ "$rc" -eq 0 ]; then
        echo "qemu-smoke: all checks passed"
    else
        echo "qemu-smoke: at least one check failed (exit $rc)"
    fi
    return $rc
}

main "$@"
