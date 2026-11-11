#!/usr/bin/env bash
# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# cmake-matrix.sh -- build the SDK with CMake the ways docs/cmake.md offers,
# and an application using it each way.  Runs inside the builder image
# (tools/verify/verify.sh starts it); x64 builds also run, against the
# stand-in at ITSCAM_TEST_HOST:ITSCAM_TEST_PORT.
#
#   tools/verify/cmake-matrix.sh [variant...]    (default: all)

set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/build/verify/cmake"
CONSUMER="$ROOT/tests/cmake/consumer"
TOOLCHAINS="$ROOT/cmake/toolchains"
JOBS="$(nproc)"
failed=()

run() {   # run <variant> <command...>
    local name="$1"; shift
    echo "--- cmake $name"
    if ! "$@" > "$OUT/$name.log" 2>&1; then
        tail -25 "$OUT/$name.log"
        echo "cmake $name: FAIL (build/verify/cmake/$name.log)"
        failed+=("$name")
        return 1
    fi
    grep -E "^-- itscam_sdk |consumer: OK|tests passed|FAIL" "$OUT/$name.log" | sed 's/^/  /'
    echo "cmake $name: OK"
}

configure_build() {   # configure_build <build dir> <cmake args...>
    local dir="$1"; shift
    # cmake -S/-B need 3.13: the floor is 3.10
    mkdir -p "$dir" &&
        (cd "$dir" && cmake "${SRC:-$CONSUMER}" -DCMAKE_BUILD_TYPE=Release "$@") &&
        cmake --build "$dir" -- -j"$JOBS"
}

# The SDK on its own: shared library, examples and smoke tests, then ctest
# against the stand-in, then install for the package variant
standalone() {
    SRC="$ROOT" configure_build "$OUT/standalone" \
        -DITSCAM_SDK_BUILD_EXAMPLES=ON -DITSCAM_SDK_BUILD_TESTS=ON \
        -DCMAKE_INSTALL_PREFIX="$OUT/install" &&
    (cd "$OUT/standalone" && ctest --output-on-failure) &&
    cmake --build "$OUT/standalone" --target install &&
    "$ROOT/tools/check-glibc.sh" "$(readlink -f "$OUT/install/lib/libitscam_sdk.so")" 2.27 --no-cxx-runtime
}

# A parent project's defaults: static, everything, C++17 application
subproject_static() {
    configure_build "$OUT/subproject-static" && "$OUT/subproject-static/consumer"
}

# The smallest build: binary client only, sources and application in C++14
subproject_binary_cxx14() {
    configure_build "$OUT/subproject-binary-cxx14" -DCONSUMER_CXX_STANDARD=14 \
        -DITSCAM_SDK_CXX_STANDARD=14 -DITSCAM_SDK_WITH_REST=OFF -DITSCAM_SDK_WITH_CGI=OFF \
        -DITSCAM_SDK_WITH_TLS=OFF -DITSCAM_SDK_WITH_C_API=OFF &&
    "$OUT/subproject-binary-cxx14/consumer"
}

subproject_shared() {
    configure_build "$OUT/subproject-shared" -DITSCAM_SDK_SHARED=ON -DCONSUMER_CXX_STANDARD=14 &&
    "$OUT/subproject-shared/consumer"
}

# find_package() against the standalone install
package() {
    configure_build "$OUT/package" -DCONSUMER_MODE=package -DCMAKE_PREFIX_PATH="$OUT/install" &&
    "$OUT/package/consumer"
}

cross_arm() {
    configure_build "$OUT/cross-arm" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAINS/arm-linux-gnueabihf.cmake" \
        -DITSCAM_SDK_SHARED=ON &&
    readelf -h "$OUT/cross-arm/consumer" | grep -q "Machine: *ARM$" &&
    "$ROOT/tools/check-glibc.sh" "$(readlink -f "$OUT/cross-arm/itscam-sdk/src/core/libitscam_sdk.so")" 2.28 --no-cxx-runtime
}

cross_arm64() {
    configure_build "$OUT/cross-arm64" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAINS/aarch64-linux-gnu.cmake" &&
    readelf -h "$OUT/cross-arm64/consumer" | grep -q "Machine: *AArch64"
}

mingw_x64() {
    configure_build "$OUT/mingw-x64" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAINS/x86_64-w64-mingw32.cmake" \
        -DITSCAM_SDK_SHARED=ON &&
    x86_64-w64-mingw32-objdump -f "$OUT/mingw-x64/consumer_c.exe" | grep -q "pei-x86-64"
}

mingw_x86_static() {
    configure_build "$OUT/mingw-x86-static" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAINS/i686-w64-mingw32.cmake" &&
    i686-w64-mingw32-objdump -f "$OUT/mingw-x86-static/consumer.exe" | grep -q "pei-i386"
}

ALL="standalone subproject_static subproject_binary_cxx14 subproject_shared package cross_arm cross_arm64 mingw_x64 mingw_x86_static"
mkdir -p "$OUT"
echo "cmake $(cmake --version | head -1 | awk '{print $3}'), $(c++ --version | head -1)"
for variant in ${@:-$ALL}; do
    run "${variant//_/-}" "$variant"
done

if [ "${#failed[@]}" -ne 0 ]; then
    echo "cmake-matrix: FAILED: ${failed[*]}"
    exit 1
fi
echo "cmake-matrix: OK"
