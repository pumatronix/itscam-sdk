#!/usr/bin/env bash
#
# Stage a consumer-ready multi-platform SDK distribution and pack it as tar.gz.
#
# Layout (under dist/itscam-sdk-<version>/):
#   VERSION.json, README-sdk.md, README-sdk.en-US.md, README-repo.md, README.en-US.md, AGENTS.md, docs/
#   csharp/              NuGet (linux-x64 + win-x64 + win-x86 native runtimes)
#   linux-x64/cpp|c|python|go|java|nodejs/
#   win-x64/ ...
#   win-x86/ ...
#   examples/            Example source (all languages); pre-built Wails GUI in examples/bin/
#
# Copyright (c) 2026 Pumatronix

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
CORE="$ROOT/src/core"
PY="$ROOT/src/wrappers/python"
CS="$ROOT/src/wrappers/csharp"
GO="$ROOT/src/wrappers/go"
JAVA="$ROOT/src/wrappers/java"
NODEJS="$ROOT/src/wrappers/nodejs"
EXAMPLES_CPP="$ROOT/src/examples"
GO_EXAMPLES="$ROOT/src/wrappers/go/examples"
GO_GUI_BIN="$GO_EXAMPLES/gui/build/bin"
CSHARP_EXAMPLES="$ROOT/src/wrappers/csharp/examples"
JAVA_EXAMPLES_SRC="$ROOT/src/wrappers/java/examples"
PY_EXAMPLES="$ROOT/src/wrappers/python/examples"
NODEJS_EXAMPLES="$ROOT/src/wrappers/nodejs/examples"
VERSION_JSON="$ROOT/VERSION.json"
VERSION_MK="$ROOT/tools/version/sdk-version.mk"

LINUX_LIB_DIR="$CORE/build/linux"
LINUX_ARM_LIB_DIR="$CORE/build/linux-arm"
LINUX_ARM64_LIB_DIR="$CORE/build/linux-arm64"
WIN_X64_LIB_DIR="$CORE/build/win-x64"
WIN_X86_LIB_DIR="$CORE/build/win-x86"
LINUX_LIB="$LINUX_LIB_DIR/libitscam_sdk.so"
LINUX_ARM_LIB="$LINUX_ARM_LIB_DIR/libitscam_sdk.so"
LINUX_ARM64_LIB="$LINUX_ARM64_LIB_DIR/libitscam_sdk.so"
WIN_X64_DLL="$WIN_X64_LIB_DIR/itscam_sdk.dll"
WIN_X64_IMPLIB="$WIN_X64_LIB_DIR/libitscam_sdk.a"
WIN_X86_DLL="$WIN_X86_LIB_DIR/itscam_sdk.dll"
WIN_X86_IMPLIB="$WIN_X86_LIB_DIR/libitscam_sdk.a"

DIST_ROOT="$ROOT/dist"

die() { echo "make-sdk-dist: $*" >&2; exit 1; }

require_file() {
    [ -f "$1" ] || die "missing $1 (run 'make lib' / 'make windows' first)"
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

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
    "NUGET_VERSION": data.get("nugetVersion", data["version"]),
    "MAVEN_VERSION": data.get("mavenVersion", data["version"]),
    "NPM_VERSION": data.get("npmVersion", data["version"]),
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
        NUGET_VERSION="${NUGET_VERSION:-$SDK_VERSION}"
        MAVEN_VERSION="${MAVEN_VERSION:-$SDK_VERSION}"
        return
    fi
    die "missing VERSION.json (run 'make version' first)"
}

stage_cpp_headers() {
    local dest="$1"
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
        itscam_rest_types.h
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

stage_linux_libs() {
    local cpp_lib="$1/cpp/lib"
    local c_lib="$2/c/lib"
    local lib_real="$LINUX_LIB_DIR/libitscam_sdk.so.${SDK_LIB_VERSION}"

    mkdir -p "$cpp_lib" "$c_lib"
    cp "$lib_real" "$cpp_lib/libitscam_sdk.so.${SDK_LIB_VERSION}"
    ln -sf "libitscam_sdk.so.${SDK_LIB_VERSION}" "$cpp_lib/libitscam_sdk.so.$(echo "$SDK_LIB_VERSION" | cut -d. -f1)"
    ln -sf "libitscam_sdk.so.${SDK_LIB_VERSION}" "$cpp_lib/libitscam_sdk.so"
    cp -a "$cpp_lib/." "$c_lib/"
}

# Stage a cross-compiled Linux .so under cpp/lib + c/lib for the given
# arch dir under $STAGING/.  Used by stage_linux_arm_platform and
# stage_linux_arm64_platform.
stage_linux_libs_from() {
    local cpp_lib="$1/cpp/lib"
    local c_lib="$2/c/lib"
    local src_dir="$3"
    local lib_real="$src_dir/libitscam_sdk.so.${SDK_LIB_VERSION}"

    mkdir -p "$cpp_lib" "$c_lib"
    cp "$lib_real" "$cpp_lib/libitscam_sdk.so.${SDK_LIB_VERSION}"
    ln -sf "libitscam_sdk.so.${SDK_LIB_VERSION}" "$cpp_lib/libitscam_sdk.so.$(echo "$SDK_LIB_VERSION" | cut -d. -f1)"
    ln -sf "libitscam_sdk.so.${SDK_LIB_VERSION}" "$cpp_lib/libitscam_sdk.so"
    cp -a "$cpp_lib/." "$c_lib/"
}

stage_windows_libs() {
    local cpp_bin="$1/cpp/bin"
    local c_bin="$2/c/bin"
    local dll="$3"
    local implib="$4"

    mkdir -p "$cpp_bin" "$c_bin"
    cp "$dll" "$cpp_bin/itscam_sdk.dll"
    cp "$implib" "$cpp_bin/libitscam_sdk.a"
    cp "$dll" "$c_bin/itscam_sdk.dll"
    cp "$implib" "$c_bin/libitscam_sdk.a"
}

stage_c_headers() {
    local dest="$1/c/include/c_api"
    mkdir -p "$dest"
    cp "$CORE/c_api/itscam_sdk_c.h" \
       "$CORE/c_api/itscam_rest_client_c.h" \
       "$CORE/c_api/itscam_cgi_client_c.h" \
       "$dest/"
}

stage_csharp_nupkg() {
    local dest="$1/csharp"
    mkdir -p "$dest"
    shopt -s nullglob
    local pkgs=("$CS/nupkg"/*.nupkg)
    shopt -u nullglob
    [ "${#pkgs[@]}" -gt 0 ] || die "no NuGet package in $CS/nupkg (run 'make csharp-pack' first)"
    cp "${pkgs[@]}" "$dest/"
}

stage_python_wheel_linux() {
    local dest="$1/python"
    local lib_real="$LINUX_LIB_DIR/libitscam_sdk.so.${SDK_LIB_VERSION}"
    mkdir -p "$dest"

    require_cmd python3

    rm -f "$PY/itscam/itscam_sdk.dll"
    cp "$lib_real" "$PY/itscam/libitscam_sdk.so"
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
    [ "${#wheels[@]}" -gt 0 ] || die "linux python wheel build produced no .whl in $dest"
}

# Build a Linux wheel for a non-native arch (ARMv7 or aarch64) by
# tagging it with the matching manylinux2014 platform tag.  Both target
# arches were the lowest practical baseline before manylinux_2_28 ;
# pairing them with the Arm GNU 8.3-2019.03 (glibc 2.28) toolchain means
# the wheel can declare manylinux2014 cleanly.  We don't run auditwheel
# because the C extension is not loaded by Python -- ctypes loads the
# .so directly at import time.
stage_python_wheel_linux_arch() {
    local dest="$1/python"
    local src_dir="$2"     # e.g. $LINUX_ARM_LIB_DIR
    local plat_name="$3"   # e.g. manylinux2014_armv7l
    local lib_real="$src_dir/libitscam_sdk.so.${SDK_LIB_VERSION}"
    mkdir -p "$dest"

    require_cmd python3
    [ -f "$lib_real" ] || die "missing $lib_real for python wheel ($plat_name)"

    rm -f "$PY/itscam/itscam_sdk.dll"
    cp "$lib_real" "$PY/itscam/libitscam_sdk.so"
    trap 'rm -f "$PY/itscam/libitscam_sdk.so"' RETURN

    (cd "$PY" && python3 setup.py bdist_wheel --plat-name "$plat_name" --dist-dir "$dest")

    rm -f "$PY/itscam/libitscam_sdk.so"
    rm -rf "$PY/build" "$PY/itscam.egg-info"
    trap - RETURN

    shopt -s nullglob
    local wheels=("$dest"/*.whl)
    shopt -u nullglob
    [ "${#wheels[@]}" -gt 0 ] || die "linux python wheel ($plat_name) produced no .whl in $dest"
}

stage_python_wheel_windows() {
    local dest="$1/python"
    local dll="$2"
    local plat_name="$3"
    mkdir -p "$dest"

    require_cmd python3

    rm -f "$PY/itscam/libitscam_sdk.so"
    cp "$dll" "$PY/itscam/itscam_sdk.dll"
    trap 'rm -f "$PY/itscam/itscam_sdk.dll"' RETURN

    (cd "$PY" && python3 setup.py bdist_wheel --plat-name "$plat_name" --dist-dir "$dest")

    rm -f "$PY/itscam/itscam_sdk.dll"
    rm -rf "$PY/build" "$PY/itscam.egg-info"
    trap - RETURN

    shopt -s nullglob
    local wheels=("$dest"/*.whl)
    shopt -u nullglob
    [ "${#wheels[@]}" -gt 0 ] || die "windows python wheel ($plat_name) build produced no .whl in $dest"
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
    local platform_root="$1"
    local dll="$2"
    local implib="$3"
    local linux_lib_dir="${4:-$LINUX_LIB_DIR}"
    local dest="$platform_root/go/itscam-sdk-go"
    mkdir -p "$dest/itscam" "$dest/native" "$dest/include"

    cp "$GO/go.mod" "$dest/"
    cp "$GO"/itscam/*.go "$dest/itscam/"
    cp "$GO/README.md" "$dest/" 2>/dev/null || true

    cp "$CORE/c_api/itscam_sdk_c.h" \
       "$CORE/c_api/itscam_rest_client_c.h" \
       "$CORE/c_api/itscam_cgi_client_c.h" \
       "$dest/include/"

    patch_go_cgo_block "$dest/itscam/client.go" "itscam_sdk_c.h"
    patch_go_cgo_block "$dest/itscam/callbacks.go" "itscam_sdk_c.h"
    patch_go_cgo_block "$dest/itscam/rest_client.go" "itscam_rest_client_c.h"
    patch_go_cgo_block "$dest/itscam/cgi_client.go" "itscam_cgi_client_c.h"

    if [[ "$platform_root" == *linux* ]]; then
        cp "$linux_lib_dir/libitscam_sdk.so.${SDK_LIB_VERSION}" "$dest/native/libitscam_sdk.so"
        ln -sf libitscam_sdk.so "$dest/native/libitscam_sdk.so.0"
        ln -sf libitscam_sdk.so "$dest/native/libitscam_sdk.so.1"
    else
        cp "$dll" "$dest/native/itscam_sdk.dll"
        cp "$implib" "$dest/native/libitscam_sdk.a"
    fi
}

stage_java_jar() {
    # Java JAR is platform-agnostic (native binaries are embedded inside it
    # under META-INF/native/<os>-<arch>/), so we copy the same JAR to every
    # platform directory for consumer convenience.
    local dest="$1/java"
    mkdir -p "$dest"

    shopt -s nullglob
    local jars=("$JAVA/itscam-sdk/target"/itscam-sdk-*.jar)
    shopt -u nullglob

    if [ "${#jars[@]}" -eq 0 ]; then
        echo "make-sdk-dist: no Java JAR in $JAVA/itscam-sdk/target -- run 'make java-pack' (or 'make docker-java-pack') first to include Java in the dist; skipping." >&2
        return 0
    fi

    cp "${jars[@]}" "$dest/"

    local jna_version gson_version
    jna_version="$(sed -n 's:.*<jna.version>\(.*\)</jna.version>.*:\1:p' "$JAVA/itscam-sdk/pom.xml" | head -n 1)"
    gson_version="$(sed -n 's:.*<gson.version>\(.*\)</gson.version>.*:\1:p' "$JAVA/itscam-sdk/pom.xml" | head -n 1)"

    local dep_src="$JAVA/itscam-sdk/target/dependency"
    local dep_dir="$dest/lib"
    local jna_jar="$dep_src/jna-$jna_version.jar"
    local gson_jar="$dep_src/gson-$gson_version.jar"

    if [ ! -f "$jna_jar" ] || [ ! -f "$gson_jar" ]; then
        local m2_repo="${M2_REPO:-${HOME:-}/.m2/repository}"
        jna_jar="$m2_repo/net/java/dev/jna/jna/$jna_version/jna-$jna_version.jar"
        gson_jar="$m2_repo/com/google/code/gson/gson/$gson_version/gson-$gson_version.jar"
    fi

    [ -f "$jna_jar" ] || die "missing Java runtime dependency jna-$jna_version.jar (run 'make java-pack' first)"
    [ -f "$gson_jar" ] || die "missing Java runtime dependency gson-$gson_version.jar (run 'make java-pack' first)"

    mkdir -p "$dep_dir"
    cp "$jna_jar" "$gson_jar" "$dep_dir/"
}

write_consumer_cpp_makefile() {
    local dest="$1"
    cat >"$dest/Makefile" <<'EOF'
# ITSCAM SDK C++ Examples -- consumer bundle Makefile (Linux x64)
#
# Run from the unpacked SDK tarball root:
#   make -C examples/cpp
#
# Copyright (c) 2026 Pumatronix

SDK_ROOT  := ../..
INCLUDE   := $(SDK_ROOT)/linux-x64/cpp/include
LIB_DIR   := $(SDK_ROOT)/linux-x64/cpp/lib
BUILD_DIR := build

CXX       ?= g++
CXXFLAGS  := -std=c++17 -Wall -Wextra
INCLUDES  := -I$(INCLUDE)
LDFLAGS   := -L$(LIB_DIR) -litscam_sdk -lpthread -Wl,-rpath,'$$ORIGIN:$(LIB_DIR)'

EXAMPLES := \
	$(BUILD_DIR)/itscam_sdk_example \
	$(BUILD_DIR)/itscam_rest_example \
	$(BUILD_DIR)/itscam_cgi_example \
	$(BUILD_DIR)/itscam_trigger_recorder

.PHONY: all clean help

all: $(EXAMPLES)

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%: %.cpp | $(BUILD_DIR)
	@echo "Building $(notdir $@)..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Build C++ examples against the Linux x64 SDK in this bundle."
	@echo "Usage: make -C examples/cpp [target]"
EOF
}

write_csharp_example_csproj() {
    local dest="$1"
    local project="$2"
    cat >"$dest/$project.csproj" <<EOF
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net8.0</TargetFramework>
    <RootNamespace>Pumatronix.Itscam.Examples</RootNamespace>
    <Nullable>disable</Nullable>
    <LangVersion>9.0</LangVersion>
  </PropertyGroup>
  <ItemGroup>
    <PackageReference Include="Pumatronix.Itscam.Sdk" Version="${NUGET_VERSION}" />
  </ItemGroup>
</Project>
EOF
}

write_examples_readme() {
    cat >"$STAGING/examples/README.txt" <<EOF
ITSCAM SDK Examples (source)
============================

This directory contains example source code for every supported language.
Build and run them against the SDK libraries shipped in this bundle.

Pre-built binaries: only the Wails desktop viewer is included, under bin/.
  bin/linux-x64/itscam-viewer
  bin/win-x64/itscam-viewer.exe

C++ (examples/cpp/)
-------------------
  make -C examples/cpp
  ./examples/cpp/build/itscam_sdk_example 192.168.254.254
  Link against ../linux-x64/cpp/lib (Linux) or ../win-x64/cpp/bin (Windows).

Go CLI (examples/go/*.go)
-------------------------
  export LD_LIBRARY_PATH=\$(pwd)/linux-x64/go/itscam-sdk-go/native:\$LD_LIBRARY_PATH
  go build -C examples/go -o capture_example capture_example.go

Go Wails GUI (examples/go/gui/)
-------------------------------
  Pre-built: ./examples/bin/linux-x64/itscam-viewer
  Rebuild from source: see examples/go/gui/README.md

C# (examples/csharp/)
---------------------
  cat > nuget.config <<EOF
  <?xml version="1.0" encoding="utf-8"?>
  <configuration>
    <packageSources>
      <add key="itscam-sdk" value="\$(pwd)/csharp" />
      <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
    </packageSources>
  </configuration>
  EOF
  dotnet restore --project examples/csharp/CaptureExample
  dotnet run --project examples/csharp/CaptureExample -- 192.168.254.254 admin 1234

Java (examples/java/)
---------------------
    Compile directly with the SDK JAR and bundled runtime dependencies:
    JAVA_SDK_DIR=linux-x64/java   # or linux-arm64/java, win-x64/java, ...
    javac -cp "\$JAVA_SDK_DIR/itscam-sdk-${MAVEN_VERSION}.jar:\$JAVA_SDK_DIR/lib/*" \\
        examples/java/src/main/java/com/pumatronix/itscam/examples/CaptureExample.java
    java -cp "\$JAVA_SDK_DIR/itscam-sdk-${MAVEN_VERSION}.jar:\$JAVA_SDK_DIR/lib/*:examples/java/src/main/java" \\
                com.pumatronix.itscam.examples.CaptureExample 192.168.254.254 1234

Python (examples/python/)
-------------------------
  pip install linux-x64/python/itscam-*.whl
  python3 examples/python/capture_example.py 192.168.254.254

Node.js (examples/nodejs/)
--------------------------
  npm install ./linux-x64/nodejs/pumatronix-itscam-sdk-${NPM_VERSION}.tgz
  node examples/nodejs/capture-example.js 192.168.254.254 1234
EOF
}

stage_examples_source() {
    local dest="$STAGING/examples"
    echo "=== Staging example source ==="

    mkdir -p "$dest/cpp"
    cp "$EXAMPLES_CPP"/*.cpp "$dest/cpp/"
    write_consumer_cpp_makefile "$dest/cpp"
    if [ -f "$EXAMPLES_CPP/README.md" ]; then
        cp "$EXAMPLES_CPP/README.md" "$dest/cpp/"
    fi

    mkdir -p "$dest/go"
    cp "$GO_EXAMPLES"/capture_example.go \
       "$GO_EXAMPLES"/rest_example.go \
       "$GO_EXAMPLES"/cgi_snapshot_example.go \
       "$dest/go/"
    mkdir -p "$dest/go/gui"
    tar -C "$GO_EXAMPLES/gui" --exclude='./build' -cf - . | tar -C "$dest/go/gui" -xf -

    local csharp_projects=(
        CaptureExample
        MjpegGrabberExample
        SoftwareTriggerSnapshotExample
        BinaryCaptureExample
    )
    for project in "${csharp_projects[@]}"; do
        mkdir -p "$dest/csharp/$project"
        cp "$CSHARP_EXAMPLES/$project/Program.cs" "$dest/csharp/$project/"
        write_csharp_example_csproj "$dest/csharp/$project" "$project"
    done

    mkdir -p "$dest/java"
    cp -r "$JAVA_EXAMPLES_SRC/src" "$dest/java/"

    mkdir -p "$dest/python"
    cp "$PY_EXAMPLES"/*.py "$dest/python/"

    mkdir -p "$dest/nodejs"
    cp "$NODEJS_EXAMPLES"/*.js "$dest/nodejs/"

    write_examples_readme
}

stage_wails_gui_binaries() {
    local dest="$STAGING/examples/bin"
    echo "=== Staging pre-built Wails GUI ==="

    require_file "$GO_GUI_BIN/linux/itscam-viewer"
    require_file "$GO_GUI_BIN/windows/itscam-viewer.exe"

    mkdir -p "$dest/linux-x64" "$dest/win-x64"
    cp "$GO_GUI_BIN/linux/itscam-viewer" "$dest/linux-x64/"
    cp "$GO_GUI_BIN/windows/itscam-viewer.exe" "$dest/win-x64/"
}

stage_nodejs_tarball() {
    # The Node.js tarball is platform-aware (native binaries staged under
    # native/<platform>-<arch>/) but the published artefact ships every
    # supported platform together, so a single tarball lands in every
    # platform dir.
    local dest="$1/nodejs"
    mkdir -p "$dest"

    shopt -s nullglob
    local tgzs=("$NODEJS"/pumatronix-itscam-sdk-*.tgz)
    shopt -u nullglob

    if [ "${#tgzs[@]}" -eq 0 ]; then
        echo "make-sdk-dist: no Node.js tarball in $NODEJS -- run 'make nodejs-pack' (or 'make docker-nodejs-pack') first to include Node.js in the dist; skipping." >&2
        return 0
    fi

    cp "${tgzs[@]}" "$dest/"
}

stage_linux_platform() {
    local dest="$STAGING/linux-x64"
    mkdir -p "$dest"
    stage_cpp_headers "$dest/cpp/include"
    stage_linux_libs "$dest" "$dest"
    stage_c_headers "$dest"
    stage_python_wheel_linux "$dest"
    stage_go_module "$dest" "" ""
    stage_java_jar "$dest"
    stage_nodejs_tarball "$dest"
}

# linux-arm (ARMv7 hard-float, ITSCAM450 + most Yocto/Buildroot armhf).
stage_linux_arm_platform() {
    local dest="$STAGING/linux-arm"
    if [ ! -f "$LINUX_ARM_LIB_DIR/libitscam_sdk.so.${SDK_LIB_VERSION}" ]; then
        echo "make-sdk-dist: skipping linux-arm (no build under $LINUX_ARM_LIB_DIR -- run 'make lib-arm' first)" >&2
        return 0
    fi
    mkdir -p "$dest"
    stage_cpp_headers "$dest/cpp/include"
    stage_linux_libs_from "$dest" "$dest" "$LINUX_ARM_LIB_DIR"
    stage_c_headers "$dest"
    stage_python_wheel_linux_arch "$dest" "$LINUX_ARM_LIB_DIR" "manylinux2014_armv7l"
    stage_go_module "$dest" "" "" "$LINUX_ARM_LIB_DIR"
    stage_java_jar "$dest"
    stage_nodejs_tarball "$dest"
}

# linux-arm64 (ARMv8 / aarch64, ITSCAM600 + most arm64 servers/SBCs).
stage_linux_arm64_platform() {
    local dest="$STAGING/linux-arm64"
    if [ ! -f "$LINUX_ARM64_LIB_DIR/libitscam_sdk.so.${SDK_LIB_VERSION}" ]; then
        echo "make-sdk-dist: skipping linux-arm64 (no build under $LINUX_ARM64_LIB_DIR -- run 'make lib-arm64' first)" >&2
        return 0
    fi
    mkdir -p "$dest"
    stage_cpp_headers "$dest/cpp/include"
    stage_linux_libs_from "$dest" "$dest" "$LINUX_ARM64_LIB_DIR"
    stage_c_headers "$dest"
    stage_python_wheel_linux_arch "$dest" "$LINUX_ARM64_LIB_DIR" "manylinux2014_aarch64"
    stage_go_module "$dest" "" "" "$LINUX_ARM64_LIB_DIR"
    stage_java_jar "$dest"
    stage_nodejs_tarball "$dest"
}

stage_windows_x64_platform() {
    local dest="$STAGING/win-x64"
    mkdir -p "$dest"
    stage_cpp_headers "$dest/cpp/include"
    stage_windows_libs "$dest" "$dest" "$WIN_X64_DLL" "$WIN_X64_IMPLIB"
    stage_c_headers "$dest"
    stage_python_wheel_windows "$dest" "$WIN_X64_DLL" "win_amd64"
    stage_go_module "$dest" "$WIN_X64_DLL" "$WIN_X64_IMPLIB"
    stage_java_jar "$dest"
    stage_nodejs_tarball "$dest"
}

stage_windows_x86_platform() {
    local dest="$STAGING/win-x86"
    mkdir -p "$dest"
    stage_cpp_headers "$dest/cpp/include"
    stage_windows_libs "$dest" "$dest" "$WIN_X86_DLL" "$WIN_X86_IMPLIB"
    stage_c_headers "$dest"
    stage_python_wheel_windows "$dest" "$WIN_X86_DLL" "win32"
    stage_go_module "$dest" "$WIN_X86_DLL" "$WIN_X86_IMPLIB"
    stage_java_jar "$dest"
    stage_nodejs_tarball "$dest"
}

stage_documentation() {
    echo "=== Staging documentation ==="
    cp "$ROOT/README.md" "$STAGING/README-repo.md"
    cp "$ROOT/README.en-US.md" \
       "$ROOT/AGENTS.md" \
       "$STAGING/"
    cp -r "$ROOT/docs" "$STAGING/"
}

write_sdk_readme() {
    cat >"$STAGING/README-sdk.md" <<README_SDK_MD
# ITSCAM Client SDK ${SDK_VERSION_FULL}

[Português (Brasil)](README-sdk.md) | [English (US)](README-sdk.en-US.md)

Pacote consumer gerado por: \`make sdk-dist\`
Commit Git: ${SDK_GIT_SHA}
Data do build: ${SDK_BUILD_DATE}

Plataformas: linux-x64, linux-arm (ARMv7), linux-arm64 (ARMv8), win-x64, win-x86
Veja \`VERSION.json\` para metadados legíveis por máquina.

## Documentação

- **README-sdk.md** — este arquivo; layout do tarball e notas de instalação por linguagem
- **README-sdk.en-US.md** — same in English (US)
- **README-repo.md** — hub de navegação e quick-link matrix do repositório GitHub (PT-BR)
- **README.en-US.md** — mesmo hub, em inglês (US)
- **AGENTS.md** — briefing para AI coding agents que integram o SDK
- **docs/** — guias chapter-style (getting started, API clients, wrappers, ...)

## Layout

- \`csharp/\` — NuGet (multi-RID: linux-x64 + win-x64 + win-x86 native binaries)
- \`examples/\` — source dos examples para todas as linguagens; Wails GUI pré-compilado em \`examples/bin/\`
- \`linux-x64/\` — headers C/C++ + \`.so\`, wheel Python, módulo Go, JAR Java, tarball npm
- \`linux-arm/\` — mesmas linguagens, \`.so\` ARMv7 hard-float (presente apenas quando \`make lib-arm\` foi executado)
- \`linux-arm64/\` — mesmas linguagens, \`.so\` ARMv8 / aarch64 (presente apenas quando \`make lib-arm64\` foi executado)
- \`win-x64/\` — headers C/C++ + \`.dll\`/\`.a\` (64-bit), wheel Python, módulo Go, JAR Java, tarball npm
- \`win-x86/\` — headers C/C++ + \`.dll\`/\`.a\` (32-bit), wheel Python, módulo Go, JAR Java, tarball npm

## Examples

\`examples/\` inclui source para C++, Go, C#, Java, Python e Node.js.
Veja \`examples/README.txt\` para instruções de build/execução por linguagem.

O viewer desktop Wails é o único binary de example pré-compilado:

- \`examples/bin/linux-x64/itscam-viewer\`
- \`examples/bin/win-x64/itscam-viewer.exe\`

Início rápido (Wails GUI):

\`\`\`bash
./examples/bin/linux-x64/itscam-viewer          # Linux
examples\\bin\\win-x64\\itscam-viewer.exe       # Windows 64-bit
\`\`\`

## C# / .NET

\`\`\`bash
cat > nuget.config <<EOF
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="itscam-sdk" value="\$(pwd)/csharp" />
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
  </packageSources>
</configuration>
EOF
ITSCAM_VERSION=\$(sed -n 's/.*"nugetVersion"[[:space:]]*:[[:space:]]*"\\([^"]*\\)".*/\\1/p' "\$(pwd)/VERSION.json")
dotnet add package Pumatronix.Itscam.Sdk --version "\$ITSCAM_VERSION"
\`\`\`

## Linux C / C++

Headers: \`linux-x64/cpp/include/\`
Library: \`linux-x64/cpp/lib/\` (link \`-litscam_sdk -lpthread\`)

\`\`\`bash
g++ -std=c++17 -Ilinux-x64/cpp/include -c your_app.cpp
g++ your_app.o -Llinux-x64/cpp/lib -litscam_sdk -lpthread \\
    -Wl,-rpath,'\$ORIGIN/linux-x64/cpp/lib' -o your_app
\`\`\`

## Linux C API

Headers: \`linux-x64/c/include/c_api/\`
Library: \`linux-x64/c/lib/\`

## Linux Python

\`\`\`bash
pip install linux-x64/python/itscam-*.whl
\`\`\`

## Linux Go

\`\`\`bash
export LD_LIBRARY_PATH=\$(pwd)/linux-x64/go/itscam-sdk-go/native:\$LD_LIBRARY_PATH
go build -C linux-x64/go/itscam-sdk-go ./...
\`\`\`

## Windows C / C++

Headers: \`win-x64/cpp/include/\`
Binaries: \`win-x64/cpp/bin/itscam_sdk.dll\` + \`libitscam_sdk.a\`

Link com \`libitscam_sdk.a\` e distribua \`itscam_sdk.dll\` junto do \`.exe\`.

## Windows C API

Headers: \`win-x64/c/include/c_api/\`
Binaries: \`win-x64/c/bin/\`

## Windows Python

\`\`\`bash
pip install win-x64/python/itscam-*.whl    # 64-bit
pip install win-x86/python/itscam-*.whl    # 32-bit
\`\`\`

## Windows Go

Copie \`win-x64/go/itscam-sdk-go\` ou \`win-x86/go/itscam-sdk-go\` para o seu projeto.
Build no Windows com CGO; \`native/itscam_sdk.dll\` deve estar no PATH ou ao lado do \`.exe\`.

## Java (qualquer plataforma)

O JAR contém native libraries para linux-x64, win-x64, win-x86, etc.
embutidas em \`META-INF/native/<os>-<arch>/\`; as dependências Java ficam em
\`<plataforma>/java/lib/\`. Sem compilação JNI.

\`\`\`bash
JAVA_SDK_DIR=linux-x64/java   # ou linux-arm64/java, win-x64/java, ...
javac -cp "\$JAVA_SDK_DIR/itscam-sdk-${SDK_VERSION}.jar:\$JAVA_SDK_DIR/lib/*" SeuApp.java
java -cp ".:\$JAVA_SDK_DIR/itscam-sdk-${SDK_VERSION}.jar:\$JAVA_SDK_DIR/lib/*" SeuApp
\`\`\`

## Node.js (qualquer plataforma)

\`\`\`bash
npm install ./linux-x64/nodejs/pumatronix-itscam-sdk-${SDK_VERSION}.tgz
\`\`\`

Depois: \`const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');\`
README_SDK_MD

    cat >"$STAGING/README-sdk.en-US.md" <<README_SDK_EN
# ITSCAM Client SDK ${SDK_VERSION_FULL}

[Português (Brasil)](README-sdk.md) | [English (US)](README-sdk.en-US.md)

Consumer bundle produced by: \`make sdk-dist\`
Git commit: ${SDK_GIT_SHA}
Build date: ${SDK_BUILD_DATE}

Platforms: linux-x64, linux-arm (ARMv7), linux-arm64 (ARMv8), win-x64, win-x86
See \`VERSION.json\` for machine-readable metadata.

## Documentation

- **README-sdk.md** — tarball layout and per-language install notes (Portuguese)
- **README-sdk.en-US.md** — this file
- **README-repo.md** — navigation hub and quick-link matrix from the GitHub repo (Portuguese)
- **README.en-US.md** — same hub, in English (US)
- **AGENTS.md** — briefing for AI coding agents integrating the SDK
- **docs/** — chapter-style guides (getting started, API clients, wrappers, ...)

## Layout

- \`csharp/\` — NuGet (multi-RID: linux-x64 + win-x64 + win-x86 native binaries)
- \`examples/\` — example source for all languages; pre-built Wails GUI in \`examples/bin/\`
- \`linux-x64/\` — C/C++ headers + \`.so\`, Python wheel, Go module, Java JAR, npm tarball
- \`linux-arm/\` — same set, ARMv7 hard-float \`.so\` (present only when \`make lib-arm\` was run)
- \`linux-arm64/\` — same set, ARMv8 / aarch64 \`.so\` (present only when \`make lib-arm64\` was run)
- \`win-x64/\` — C/C++ headers + \`.dll\`/\`.a\` (64-bit), Python wheel, Go module, Java JAR, npm tarball
- \`win-x86/\` — C/C++ headers + \`.dll\`/\`.a\` (32-bit), Python wheel, Go module, Java JAR, npm tarball

## Examples

\`examples/\` ships source code for C++, Go, C#, Java, Python, and Node.js.
See \`examples/README.txt\` for build/run instructions per language.

The Wails desktop viewer is the only pre-built example binary:

- \`examples/bin/linux-x64/itscam-viewer\`
- \`examples/bin/win-x64/itscam-viewer.exe\`

Quick start (Wails GUI):

\`\`\`bash
./examples/bin/linux-x64/itscam-viewer          # Linux
examples\\bin\\win-x64\\itscam-viewer.exe       # Windows 64-bit
\`\`\`

## C# / .NET

\`\`\`bash
cat > nuget.config <<EOF
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources>
    <add key="itscam-sdk" value="\$(pwd)/csharp" />
    <add key="nuget.org" value="https://api.nuget.org/v3/index.json" protocolVersion="3" />
  </packageSources>
</configuration>
EOF
ITSCAM_VERSION=\$(sed -n 's/.*"nugetVersion"[[:space:]]*:[[:space:]]*"\\([^"]*\\)".*/\\1/p' "\$(pwd)/VERSION.json")
dotnet add package Pumatronix.Itscam.Sdk --version "\$ITSCAM_VERSION"
\`\`\`

## Linux C / C++

Headers: \`linux-x64/cpp/include/\`
Library: \`linux-x64/cpp/lib/\` (link \`-litscam_sdk -lpthread\`)

\`\`\`bash
g++ -std=c++17 -Ilinux-x64/cpp/include -c your_app.cpp
g++ your_app.o -Llinux-x64/cpp/lib -litscam_sdk -lpthread \\
    -Wl,-rpath,'\$ORIGIN/linux-x64/cpp/lib' -o your_app
\`\`\`

## Linux C API

Headers: \`linux-x64/c/include/c_api/\`
Library: \`linux-x64/c/lib/\`

## Linux Python

\`\`\`bash
pip install linux-x64/python/itscam-*.whl
\`\`\`

## Linux Go

\`\`\`bash
export LD_LIBRARY_PATH=\$(pwd)/linux-x64/go/itscam-sdk-go/native:\$LD_LIBRARY_PATH
go build -C linux-x64/go/itscam-sdk-go ./...
\`\`\`

## Windows C / C++

Headers: \`win-x64/cpp/include/\`
Binaries: \`win-x64/cpp/bin/itscam_sdk.dll\` + \`libitscam_sdk.a\`

Link with \`libitscam_sdk.a\` and ship \`itscam_sdk.dll\` next to your \`.exe\`.

## Windows C API

Headers: \`win-x64/c/include/c_api/\`
Binaries: \`win-x64/c/bin/\`

## Windows Python

\`\`\`bash
pip install win-x64/python/itscam-*.whl    # 64-bit
pip install win-x86/python/itscam-*.whl    # 32-bit
\`\`\`

## Windows Go

Copy \`win-x64/go/itscam-sdk-go\` or \`win-x86/go/itscam-sdk-go\` into your project.
Build on Windows with CGO; \`native/itscam_sdk.dll\` must be on PATH or beside the \`.exe\`.

## Java (any platform)

The JAR contains native libraries for linux-x64, win-x64, win-x86, etc.
embedded under \`META-INF/native/<os>-<arch>/\`; Java dependencies are bundled
under \`<platform>/java/lib/\`. No JNI compilation needed.

\`\`\`bash
JAVA_SDK_DIR=linux-x64/java   # or linux-arm64/java, win-x64/java, ...
javac -cp "\$JAVA_SDK_DIR/itscam-sdk-${SDK_VERSION}.jar:\$JAVA_SDK_DIR/lib/*" YourApp.java
java -cp ".:\$JAVA_SDK_DIR/itscam-sdk-${SDK_VERSION}.jar:\$JAVA_SDK_DIR/lib/*" YourApp
\`\`\`

## Node.js (any platform)

\`\`\`bash
npm install ./linux-x64/nodejs/pumatronix-itscam-sdk-${SDK_VERSION}.tgz
\`\`\`

Then: \`const { ItscamCgiClient } = require('@pumatronix/itscam-sdk');\`
README_SDK_EN
}

main() {
    load_version_metadata

    local lib_real="$LINUX_LIB_DIR/libitscam_sdk.so.${SDK_LIB_VERSION}"
    STAGING="$DIST_ROOT/sdk-staging/itscam-sdk-${SDK_VERSION}"
    ARCHIVE="$DIST_ROOT/itscam-sdk-${SDK_VERSION}.tar.gz"

    require_file "$lib_real"
    require_file "$WIN_X64_DLL"
    require_file "$WIN_X64_IMPLIB"
    require_file "$WIN_X86_DLL"
    require_file "$WIN_X86_IMPLIB"

    # ARM builds are optional -- the SDK still ships if a hosting CI
    # doesn't have the Arm cross-toolchains installed.  Each stage helper
    # logs and skips when its .so is missing.
    local platforms="linux-x64 win-x64 win-x86"
    [ -f "$LINUX_ARM_LIB_DIR/libitscam_sdk.so.${SDK_LIB_VERSION}" ]   && platforms="$platforms linux-arm"
    [ -f "$LINUX_ARM64_LIB_DIR/libitscam_sdk.so.${SDK_LIB_VERSION}" ] && platforms="$platforms linux-arm64"

    echo "=== Staging SDK distribution ${SDK_VERSION} ($platforms) ==="
    rm -rf "$STAGING"
    mkdir -p "$STAGING"

    stage_csharp_nupkg "$STAGING"
    stage_linux_platform
    stage_linux_arm_platform
    stage_linux_arm64_platform
    stage_windows_x64_platform
    stage_windows_x86_platform
    stage_examples_source
    stage_wails_gui_binaries
    stage_documentation
    cp "$ROOT/VERSION.json" "$STAGING/VERSION.json"
    write_sdk_readme

    mkdir -p "$DIST_ROOT"
    rm -f "$ARCHIVE"
    tar -C "$DIST_ROOT/sdk-staging" -czf "$ARCHIVE" "itscam-sdk-${SDK_VERSION}"

    echo "=== SDK archive ready ==="
    echo "$ARCHIVE"
    tar -tzf "$ARCHIVE" >"$DIST_ROOT/.sdk-dist-list"
    sed -n '1,24p' "$DIST_ROOT/.sdk-dist-list"
    echo "... ($(wc -l <"$DIST_ROOT/.sdk-dist-list") entries total)"
    rm -f "$DIST_ROOT/.sdk-dist-list"
}

main "$@"
