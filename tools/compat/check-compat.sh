#!/usr/bin/env bash
# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# check-compat.sh -- compare the public API and ABI of the working tree with
# the last release (compat/baseline), in every language and on every Linux
# architecture.  `make compat-check`; part of `make verify`.
#
#   tools/compat/check-compat.sh [<baseline git ref>]
#
# Both trees are built the same way, in the same images:
#   ABI  libitscam_sdk.so for x64, ARMv7 and ARM64 with debug info, compared
#        by abidiff over the public headers (C and C++ functions, variables,
#        and the layout of every type they reach).
#   API  a listing per wrapper (Python, Go, Node.js typings, C#, Java).
#
# Fails on an incompatible change unless it was accepted on purpose:
#   ABI  a libabigail suppression in compat/abi.suppr
#   API  the removed line, prefixed by its surface, in
#        compat/accepted-api-breaks.txt
# See docs/compatibility.md for when that is allowed and what else it takes.
#
# Environment: COMPAT_ARCHES (default "x64 arm arm64"), and the images of
# tools/verify/common.sh.  Output: build/verify/compat/.

set -euo pipefail
source "$(dirname "$0")/../verify/common.sh"

BASELINE="${1:-$(grep -v '^#' "$ROOT/compat/baseline" | head -1)}"
ARCHES="${COMPAT_ARCHES:-x64 arm arm64}"
OUT="$VERIFY_DIR/compat"
ACCEPTED="$ROOT/compat/accepted-api-breaks.txt"
SUPPR="$ROOT/compat/abi.suppr"

if ! git -C "$ROOT" rev-parse --verify --quiet "$BASELINE^{commit}" >/dev/null; then
    echo "check-compat: baseline '$BASELINE' not found; fetch the tags (git fetch --tags)" >&2
    exit 1
fi

core_dir() { case "$1" in x64) echo linux ;; arm) echo linux-arm ;; arm64) echo linux-arm64 ;; esac; }

# --- Sources -----------------------------------------------------------------

echo "check-compat: $BASELINE ($(git -C "$ROOT" rev-parse --short "$BASELINE")) vs the working tree"
case "$OUT" in "$ROOT"/build/verify/compat) ;; *) echo "unexpected OUT=$OUT" >&2; exit 1 ;; esac
rm -rf "$OUT"
mkdir -p "$OUT/baseline" "$OUT/current"
git -C "$ROOT" archive "$BASELINE" src tools | tar -x -C "$OUT/baseline"
# The working tree as it would be committed: tracked and new files, not ignored ones
(cd "$ROOT" && git ls-files -z --cached --others --exclude-standard -- src tools |
    python3 -c 'import os, sys
sys.stdout.write("".join(p + "\0" for p in sys.stdin.read().split("\0") if p and os.path.lexists(p)))' |
    tar --null -c -T -) | tar -x -C "$OUT/current"

# Version metadata the builds need (wrapper version files, version header)
for side in baseline current; do
    docker_run "$BUILDER_IMAGE" -- bash -c "cd build/verify/compat/$side && tools/version/gen-version.sh >/dev/null"
done

# --- ABI ---------------------------------------------------------------------

DEBUG_FLAGS='-std=c++17 -O2 -fPIC -Wall -Wextra -g'
for side in baseline current; do
    targets=""
    for arch in $ARCHES; do targets="$targets build/$(core_dir "$arch")/libitscam_sdk.so"; done
    echo "--- building the $side libraries:$targets"
    docker_run "$CORE_IMAGE" -- bash -c "make -s -j\$(nproc) -C build/verify/compat/$side/src/core \
        CXXFLAGS_COMMON='$DEBUG_FLAGS' $targets 2>&1 | grep -v -E '^(Compiling|Linking|Creating)' || true; \
        for t in $targets; do test -f build/verify/compat/$side/src/core/\$t || { echo missing \$t; exit 1; }; done"
    # The headers a consumer gets (make-sdk-dist.sh): the rest is private
    inc="$OUT/$side/public-include"
    mkdir -p "$inc/c_api" "$inc/3rdparty"
    cp "$OUT/$side"/src/core/*.h "$inc/"
    cp "$OUT/$side"/src/core/c_api/*.h "$inc/c_api/"
    cp -r "$OUT/$side/src/core/3rdparty/nlohmann" "$OUT/$side/src/core/3rdparty/nonstd" "$inc/3rdparty/"
done

# abi-private.suppr: code no application can reference (permanent);
# abi.suppr: breaks accepted since the baseline (emptied at each release)
suppr_args="--suppressions /sdk/compat/abi-private.suppr"
if grep -q -v -E '^\s*(#|$)' "$SUPPR" 2>/dev/null; then suppr_args="$suppr_args --suppressions /sdk/compat/abi.suppr"; fi
abi_rc=0
for arch in $ARCHES; do
    lib="src/core/build/$(core_dir "$arch")/libitscam_sdk.so"
    set +e
    # --no-unreferenced-symbols: an ELF symbol no debug info describes isn't
    # declared by any header (lambdas' typeinfo, third-party internals).
    # --no-added-syms: additions are compatible, so whatever abidiff still
    # reports is a removal or a change, and breaks callers
    docker_run "$COMPAT_IMAGE" -- abidiff --no-show-locs --drop-private-types --no-unreferenced-symbols \
        --no-added-syms $suppr_args \
        --headers-dir1 build/verify/compat/baseline/public-include \
        --headers-dir2 build/verify/compat/current/public-include \
        "build/verify/compat/baseline/$lib" "build/verify/compat/current/$lib" > "$OUT/abi-$arch.txt" 2>&1
    rc=$?
    set -e
    sed 's/^/  /' "$OUT/abi-$arch.txt" | head -60 || true
    # abidiff sets 8 for removals only: a changed layout (a struct gaining a
    # field) is 4, and breaks callers just the same
    if (( rc & 3 )); then
        echo "abi $arch: FAIL abidiff error $rc"
        abi_rc=1
    elif (( rc & 12 )); then
        echo "abi $arch: FAIL removed or changed (see $OUT/abi-$arch.txt)"
        abi_rc=1
    else
        echo "abi $arch: OK   nothing removed or changed"
    fi
done

# --- API ---------------------------------------------------------------------

for side in baseline current; do
    s="build/verify/compat/$side"
    echo "--- listing the $side wrapper APIs"
    docker_run "$COMPAT_IMAGE" -- bash -c "
        python3 tools/compat/api_python.py $s/src/wrappers/python/itscam > $s/api-python.txt &&
        python3 tools/compat/api_dts.py $s/src/wrappers/nodejs/types/index.d.ts > $s/api-nodejs.txt"
    docker_run "$BUILDER_IMAGE" -- bash -c "
        set -e
        (cd tools/compat/api_go && go run . /sdk/$s/src/wrappers/go/itscam) > $s/api-go.txt
        dotnet build -nologo -v q -c Release -o $s/csharp-out $s/src/wrappers/csharp/Itscam.Sdk/Itscam.Sdk.csproj > $s/csharp-build.log 2>&1 ||
            { tail -20 $s/csharp-build.log; exit 1; }
        dotnet run -c Release --project tools/compat/api_csharp -- /sdk/$s/csharp-out/Itscam.Sdk.dll > $s/api-csharp.txt
        (cd $s/src/wrappers/java/itscam-sdk && mvn -q -B \$MAVEN_OPTS -DskipTests -Drevision=0.0.0 \
            -Dmaven.javadoc.skip=true -Dmaven.source.skip=true package > ../../../../java-build.log 2>&1) ||
            { tail -20 $s/java-build.log; exit 1; }
        python3 tools/compat/api_java.py $s/src/wrappers/java/itscam-sdk/target/itscam-sdk-0.0.0.jar > $s/api-java.txt"
done

api_rc=0
for surface in python go nodejs csharp java; do
    python3 "$ROOT/tools/compat/diff_api.py" "$surface" \
        "$OUT/baseline/api-$surface.txt" "$OUT/current/api-$surface.txt" "$ACCEPTED" \
        > "$OUT/api-$surface.diff" || api_rc=1
    head -40 "$OUT/api-$surface.diff" | grep -v -E '^\S' | sed 's/^/ /' || true
    tail -1 "$OUT/api-$surface.diff"
done

if [ "$abi_rc" -ne 0 ] || [ "$api_rc" -ne 0 ]; then
    echo "check-compat: FAILED -- an incompatible change needs a release that may break"
    echo "  callers, and an entry in compat/ (docs/compatibility.md, \"Breaking changes\")"
    exit 1
fi
echo "check-compat: OK"
