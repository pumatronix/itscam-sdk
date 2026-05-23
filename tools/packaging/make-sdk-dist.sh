#!/usr/bin/env bash
#
# Stage a consumer-ready SDK distribution and pack it as tar.gz.
#
# Layout (under dist/itscam-sdk-<version>-<rid>/):
#   cpp/     Public C++ headers + libitscam_sdk.so
#   c/       C API headers + libitscam_sdk.so
#   csharp/  Pumatronix.Itscam.Sdk NuGet package
#   python/  itscam wheel with bundled native library
#   go/      Standalone Go module source + native/ + include/
#
# Copyright (c) 2026 Pumatronix

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CORE="$ROOT/src/core"
PY="$ROOT/src/wrappers/python"
CS="$ROOT/src/wrappers/csharp"
GO="$ROOT/src/wrappers/go"
VERSION_JSON="$ROOT/VERSION.json"
VERSION_MK="$ROOT/tools/version/sdk-version.mk"

load_version_metadata() {
    if [ -f "$VERSION_JSON" ]; then
        eval "$(python3 - "$VERSION_JSON" <<'PY'
import json, shlex, sys
data = json.load(open(sys.argv[1], encoding="utf-8"))
fields = {
    "SDK_VERSION": data["version"],
    "SDK_VERSION_FULL": data["versionFull"],
    "SDK_LIB_VERSION": data["libVersion"],
    "SDK_GIT_SHA": data["gitSha"],
    "SDK_GIT_SHA_SHORT": data["gitShaShort"],
    "SDK_BUILD_DATE": data["buildDate"],
}
for key, value in fields.items():
    print(f"{key}={shlex.quote(str(value))}")
PY
)"
        return
    fi
    if [ -f "$VERSION_MK" ]; then
        set -a
        # shellcheck disable=SC1091
        . "$VERSION_MK"
        set +a
        SDK_VERSION_FULL="${SDK_VERSION_FULL:-$SDK_VERSION}"
        SDK_LIB_VERSION="${SDK_LIB_VERSION:-$SDK_VERSION}"
        return
    fi
    die "missing VERSION.json (run 'make version' first)"
}

SDK_RID="${SDK_RID:-linux-x64}"

DIST_ROOT="$ROOT/dist"

die() { echo "make-sdk-dist: $*" >&2; exit 1; }

require_file() {
    [ -f "$1" ] || die "missing $1 (run 'make lib' first)"
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

stage_cpp_headers() {
    local dest="$STAGING/cpp/include"
    mkdir -p "$dest/c_api" "$dest/3rdparty/nlohmann"

    local headers=(
        itscam_sdk.h
        itscam_sdk_version.h
        itscam_client.h
        itscam_rest_client.h
        itscam_cgi_client.h
        itscam_types.h
        itscam_os.h
        itscam_jpeg_utils.h
        itscam_sdk_utils.h
        itscam_rest_types.hpp
    )
    for h in "${headers[@]}"; do
        cp "$CORE/$h" "$dest/"
    done

    cp "$CORE/c_api/itscam_sdk_c.h" \
       "$CORE/c_api/itscam_rest_client_c.h" \
       "$CORE/c_api/itscam_cgi_client_c.h" \
       "$dest/c_api/"

    cp "$CORE/3rdparty/nlohmann/json.hpp" "$dest/3rdparty/nlohmann/"
}

stage_native_lib() {
    local dest="$1"
    mkdir -p "$dest"
    cp "$LIB_REAL" "$dest/libitscam_sdk.so.${SDK_LIB_VERSION}"
    ln -sf "libitscam_sdk.so.${SDK_LIB_VERSION}" "$dest/libitscam_sdk.so.$(echo "$SDK_LIB_VERSION" | cut -d. -f1)"
    ln -sf "libitscam_sdk.so.${SDK_LIB_VERSION}" "$dest/libitscam_sdk.so"
}

stage_c_headers() {
    local dest="$STAGING/c/include/c_api"
    mkdir -p "$dest"
    cp "$CORE/c_api/itscam_sdk_c.h" \
       "$CORE/c_api/itscam_rest_client_c.h" \
       "$CORE/c_api/itscam_cgi_client_c.h" \
       "$dest/"
}

stage_csharp_nupkg() {
    local dest="$STAGING/csharp"
    mkdir -p "$dest"
    shopt -s nullglob
    local pkgs=("$CS/nupkg"/*.nupkg)
    shopt -u nullglob
    [ "${#pkgs[@]}" -gt 0 ] || die "no NuGet package in $CS/nupkg (run 'make csharp-pack' first)"
    cp "${pkgs[@]}" "$dest/"
}

stage_python_wheel() {
    local dest="$STAGING/python"
    mkdir -p "$dest"

    require_cmd python3

    cp "$LIB_REAL" "$PY/itscam/libitscam_sdk.so"
    trap 'rm -f "$PY/itscam/libitscam_sdk.so"' RETURN

    if python3 -m build --help >/dev/null 2>&1; then
        python3 -m build --wheel --outdir "$dest" "$PY"
    else
        (cd "$PY" && python3 setup.py bdist_wheel --dist-dir "$dest")
    fi

    rm -f "$PY/itscam/libitscam_sdk.so"
    rm -rf "$PY/build" "$PY/itscam.egg-info"
    trap - RETURN

    shopt -s nullglob
    local wheels=("$dest"/*.whl)
    shopt -u nullglob
    [ "${#wheels[@]}" -gt 0 ] || die "python wheel build produced no .whl in $dest"
}

patch_go_cgo_block() {
    local file="$1"
    local header="$2"
    python3 - "$file" "$header" <<'PY'
import pathlib
import re
import sys

path = pathlib.Path(sys.argv[1])
header = sys.argv[2]
text = path.read_text()
match = re.search(r"/\*[^*]*#cgo[\s\S]*?\*/", text)
if not match:
    raise SystemExit(f"no cgo block in {path}")

old = match.group(0)
body_lines = []
for line in old.splitlines():
    stripped = line.strip()
    if stripped.startswith("#cgo") or stripped.startswith("#include"):
        continue
    if stripped in ("/*", "*/"):
        continue
    if stripped:
        body_lines.append(line)

body = "\n".join(body_lines)
body_suffix = f"\n{body}" if body else ""
replacement = f'''/*
#cgo LDFLAGS: -L${{SRCDIR}}/../native -litscam_sdk
#cgo CFLAGS: -I${{SRCDIR}}/../include
#include <stdlib.h>
#include <stdbool.h>
#include "{header}"{body_suffix}
*/'''

text = text[: match.start()] + replacement + text[match.end() :]
path.write_text(text)
PY
}

stage_go_module() {
    local dest="$STAGING/go/itscam-sdk-go"
    mkdir -p "$dest/itscam" "$dest/native" "$dest/include"

    cp "$GO/go.mod" "$dest/"
    cp "$GO"/itscam/*.go "$dest/itscam/"
    cp "$GO/README.md" "$dest/" 2>/dev/null || true

    cp "$LIB_REAL" "$dest/native/libitscam_sdk.so"
    ln -sf libitscam_sdk.so "$dest/native/libitscam_sdk.so.1"

    cp "$CORE/c_api/itscam_sdk_c.h" \
       "$CORE/c_api/itscam_rest_client_c.h" \
       "$CORE/c_api/itscam_cgi_client_c.h" \
       "$dest/include/"

    patch_go_cgo_block "$dest/itscam/client.go" "itscam_sdk_c.h"
    patch_go_cgo_block "$dest/itscam/callbacks.go" "itscam_sdk_c.h"
    patch_go_cgo_block "$dest/itscam/rest_client.go" "itscam_rest_client_c.h"
    patch_go_cgo_block "$dest/itscam/cgi_client.go" "itscam_cgi_client_c.h"
}

write_readme() {
    cat >"$STAGING/README.txt" <<EOF
ITSCAM Client SDK ${SDK_VERSION_FULL} (${SDK_RID})
============================================

Consumer bundle produced by: make sdk-dist
Git commit: ${SDK_GIT_SHA}
Build date: ${SDK_BUILD_DATE}

See VERSION.json for machine-readable metadata.

C / C++
-------
  Headers: cpp/include/   (-I path for g++/clang++)
  Library: cpp/lib/       (-L path, link -litscam_sdk)

  Example:
    g++ -std=c++17 -Icpp/include -c your_app.cpp
    g++ your_app.o -Lcpp/lib -litscam_sdk -lpthread \\
        -Wl,-rpath,'\$ORIGIN/cpp/lib' -o your_app

C API
-----
  Headers: c/include/c_api/
  Library: c/lib/

  Example:
    gcc -std=c11 -Ic/include/c_api -c your_app.c
    gcc your_app.o -Lc/lib -litscam_sdk -lpthread \\
        -Wl,-rpath,'\$ORIGIN/c/lib' -o your_app

C# / .NET
---------
  dotnet add package Pumatronix.Itscam.Sdk --source \$(pwd)/csharp

  Or add a local NuGet source pointing at the csharp/ directory.

Python
------
  pip install python/itscam-*.whl

Go
--
  Copy go/itscam-sdk-go into your project or add it as a module replace.

  export LD_LIBRARY_PATH=\$(pwd)/go/itscam-sdk-go/native:\$LD_LIBRARY_PATH
  go build -C go/itscam-sdk-go ./...

  See go/itscam-sdk-go/README.md for module import path details.
EOF
}

main() {
    load_version_metadata

    local lib_dir="$CORE/build/linux"
    local lib_real="$lib_dir/libitscam_sdk.so.${SDK_LIB_VERSION}"
    local staging="$DIST_ROOT/sdk-staging/itscam-sdk-${SDK_VERSION}-${SDK_RID}"
    local archive="$DIST_ROOT/itscam-sdk-${SDK_VERSION}-${SDK_RID}.tar.gz"

    LIB_DIR="$lib_dir"
    LIB_REAL="$lib_real"
    STAGING="$staging"
    ARCHIVE="$archive"

    require_file "$LIB_REAL"

    echo "=== Staging SDK distribution ${SDK_VERSION} (${SDK_RID}) ==="
    rm -rf "$STAGING"
    mkdir -p "$STAGING"

    stage_cpp_headers
    stage_native_lib "$STAGING/cpp/lib"
    stage_c_headers
    stage_native_lib "$STAGING/c/lib"
    stage_csharp_nupkg
    stage_python_wheel
    stage_go_module
    cp "$ROOT/VERSION.json" "$STAGING/VERSION.json"
    write_readme

    mkdir -p "$DIST_ROOT"
    rm -f "$ARCHIVE"
    tar -C "$DIST_ROOT/sdk-staging" -czf "$ARCHIVE" "itscam-sdk-${SDK_VERSION}-${SDK_RID}"

    echo "=== SDK archive ready ==="
    echo "$ARCHIVE"
    tar -tzf "$ARCHIVE" >"$DIST_ROOT/.sdk-dist-list"
    sed -n '1,20p' "$DIST_ROOT/.sdk-dist-list"
    echo "... ($(wc -l <"$DIST_ROOT/.sdk-dist-list") entries total)"
    rm -f "$DIST_ROOT/.sdk-dist-list"
}

main "$@"
