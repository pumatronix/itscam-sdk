#!/usr/bin/env bash
# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# verify.sh -- build everything the SDK ships and check it on every platform
# it supports, the way an application would use it.  `make verify`, and CI
# on every pull request (.github/workflows/verify.yml).  Run it before
# opening a pull request; docs/compatibility.md says what each step proves.
#
#   cores      Linux libraries at the glibc 2.23 floor, x64/ARMv7/ARM64, with
#              no C++ runtime dependency and only the SDK's symbols exported
#   windows    Windows DLL and static library, x64 and x86
#   consumers  the C and C++ smoke tests built with every supported compiler
#              (GCC 5.4, GCC 7.5, Arm GNU-A 8.3, MinGW) as C++14 and C++17
#   wrappers   Go, Node.js, Java and C# smoke tests built
#   run        everything above run against tests/standin on each
#              architecture: C/C++/Go on Ubuntu 16.04 (the floor) and on the
#              compiler's own runtime, Python 3.7, Node.js 16 (20 on ARMv7),
#              C#, Java (also
#              with a non-UTF-8 default charset); ARM under emulation
#   cmake      tools/verify/cmake-matrix.sh: standalone, subproject options,
#              installed package, ARM and Windows toolchains
#   compat     tools/compat/check-compat.sh: API and ABI against compat/baseline
#   codegen    the generated REST types match tools/codegen/spec
#   java7      the Java wrapper compiles with a real JDK 7
#
# Environment:
#   VERIFY_STEPS   steps to run (default: all, in the order above)
#   VERIFY_ARCHES  Linux architectures (default: x64 arm arm64)
#   BUILDER_IMAGE, CORE_IMAGE, COMPAT_IMAGE: see tools/verify/common.sh
#
# ARM steps need Docker to run linux/arm/v7 and linux/arm64 containers
# (qemu-user through binfmt_misc: Docker Desktop has it; on Linux,
# `docker run --privileged --rm tonistiigi/binfmt --install arm,arm64`).

set -uo pipefail
source "$(dirname "$0")/common.sh"

STEPS="${VERIFY_STEPS:-cores windows consumers wrappers run cmake compat codegen java7}"
ARCHES="${VERIFY_ARCHES:-x64 arm arm64}"
BIN="build/verify/bin"
STANDIN="itscam-verify-standin-$$"
PORT=18080

core_dir()  { case "$1" in x64) echo linux ;; arm) echo linux-arm ;; arm64) echo linux-arm64 ;; esac; }
platform()  { case "$1" in x64) echo linux/amd64 ;; arm) echo linux/arm/v7 ;; arm64) echo linux/arm64 ;; esac; }
# Ubuntu 16.04's cross compilers (GCC 5.4) in the core image
gcc5()      { case "$1" in x64) echo "" ;; arm) echo arm-linux-gnueabihf- ;; arm64) echo aarch64-linux-gnu- ;; esac; }
# The builder's compilers: GCC 7.5 (x64), Arm GNU-A 8.3 (ARM)
newer()     { case "$1" in x64) echo "" ;; arm) echo /opt/cross/armhf/bin/arm-linux-gnueabihf- ;;
                                  arm64) echo /opt/cross/arm64/bin/aarch64-linux-gnu- ;; esac; }
newer_name() { case "$1" in x64) echo gcc7 ;; *) echo armgnu8 ;; esac; }
# Where the newer compilers' binaries run: their own libstdc++ and glibc
newer_runtime() { case "$1" in x64) echo ubuntu:18.04 ;; *) echo debian:buster ;; esac; }
# Node.js: the oldest supported (16), except where koffi's prebuilt binary
# needs a newer glibc than Node 16's images have (ARMv7: glibc 2.34)
node_image() { case "$1" in arm) echo node:20-bookworm-slim ;; *) echo node:16-slim ;; esac; }
goarch()    { case "$1" in x64) echo "GOARCH=amd64" ;; arm) echo "GOARCH=arm GOARM=7" ;; arm64) echo "GOARCH=arm64" ;; esac; }
has_step()  { [[ " $STEPS " == *" $1 "* ]]; }

INCLUDES="-Isrc/core -Isrc/core/3rdparty"

# --- Steps ---------------------------------------------------------------------

step_version() {
    docker_run "$BUILDER_IMAGE" -- make -s version
}

step_cores() {
    local libs="" checks=""
    for arch in $ARCHES; do
        local d; d="$(core_dir "$arch")"
        libs="$libs build/$d/libitscam_sdk.so build/$d/libitscam_sdk.a"
        checks="$checks check-glibc-$d"
    done
    docker_run "$CORE_IMAGE" -- bash -c "set -o pipefail
        make -s -C src/core -j\$(nproc) $libs 2>&1 | grep -v -E '^(Compiling|Linking|Creating)' || true
        make -s -C src/core $checks"
}

step_windows() {
    docker_run "$BUILDER_IMAGE" -- bash -c '
        set -e
        make -s -C src/core windows 2>&1 | grep -v -E "^(Compiling|Linking|Creating)" || true
        for t in x86_64:x64 i686:x86; do
            dll=$(ls src/core/build/win-${t##*:}/*.dll)
            deps=$(${t%%:*}-w64-mingw32-objdump -p "$dll" | awk "/DLL Name:/ {print \$3}" | tr "\n" " ")
            echo "$dll needs: $deps"
            if echo "$deps" | grep -q -i -E "libstdc|libgcc|libwinpthread"; then
                echo "FAIL: the DLL must carry its own C++ runtime"; exit 1
            fi
        done'
}

step_consumers() {
    local cmds="set -e"
    for arch in $ARCHES; do
        local lib="-Lsrc/core/build/$(core_dir "$arch") -litscam_sdk -lpthread"
        local p5 pn; p5="$(gcc5 "$arch")"; pn="$(newer "$arch")"
        local out5="$BIN/$arch/gcc5" outn="$BIN/$arch/$(newer_name "$arch")"
        docker_run "$CORE_IMAGE" -- bash -c "set -e; mkdir -p $out5
            for std in c++14 c++17; do
                ${p5}g++ -std=\$std -Wall -Wextra $INCLUDES tests/smoke/smoke.cpp -o $out5/smoke-cpp-\$std $lib
            done
            ${p5}gcc -std=c99 -Wall -Wextra $INCLUDES tests/smoke/smoke.c -o $out5/smoke-c $lib
            echo '$arch: GCC 5.4 consumers built (C++14, C++17, C99)'" || return 1
        cmds="$cmds; mkdir -p $outn
            for std in c++14 c++17; do
                ${pn}g++ -std=\$std -Wall -Wextra -Wno-psabi $INCLUDES tests/smoke/smoke.cpp -o $outn/smoke-cpp-\$std $lib
            done
            ${pn}gcc -std=c99 -Wall -Wextra $INCLUDES tests/smoke/smoke.c -o $outn/smoke-c $lib
            echo '$arch: $(newer_name "$arch") consumers built (C++14, C++17, C99)'"
    done
    docker_run "$BUILDER_IMAGE" -- bash -c "$cmds
        # Windows: C++ against the static library, C against the DLL
        mkdir -p $BIN/win
        for t in x86_64:x64 i686:x86; do
            cc=\${t%%:*}-w64-mingw32; d=src/core/build/win-\${t##*:}
            \$cc-g++ -std=c++17 -DITSCAM_SDK_STATIC -DWIN32 -D_WIN32 $INCLUDES tests/smoke/smoke.cpp \
                -o $BIN/win/smoke-cpp-\${t##*:}.exe \$d/libitscam_sdk_static.a -static -lws2_32 -lbcrypt -lcrypt32
            \$cc-gcc -std=c99 $INCLUDES tests/smoke/smoke.c -o $BIN/win/smoke-c-\${t##*:}.exe -L\$d -litscam_sdk
        done
        echo 'Windows consumers linked (C++17 static, C99 against the DLL; not run: no Windows here)'
        # Including the standard optional before the SDK must not compile
        if g++ -std=c++17 $INCLUDES -fsyntax-only tests/consumer/optional_std_first.cpp 2>/tmp/o.log; then
            echo 'FAIL: tests/consumer/optional_std_first.cpp compiled'; exit 1
        fi
        grep -q 'Include itscam_sdk.h before' /tmp/o.log && echo 'standard optional before the SDK: rejected, as it must be'"
}

step_wrappers() {
    local go_cmds=""
    for arch in $ARCHES; do
        local cc; cc="$(newer "$arch")gcc"
        go_cmds="$go_cmds; mkdir -p /sdk/$BIN/$arch/go
            (cd tests/smoke/go && CGO_ENABLED=1 $(goarch "$arch") CC=$cc CGO_CFLAGS=-I/sdk/src/core \
                CGO_LDFLAGS=-L/sdk/src/core/build/$(core_dir "$arch") go build -o /sdk/$BIN/$arch/go/smoke .)
            echo 'Go smoke built for $arch'"
    done
    docker_run "$BUILDER_IMAGE" -- bash -c "set -e $go_cmds
        (cd src/wrappers/nodejs && npm ci --no-audit --no-fund --silent) && echo 'Node.js wrapper installed'
        (cd src/wrappers/java/itscam-sdk && mvn -q -B \$MAVEN_OPTS -DskipTests -Drevision=0.0.0 \
            -Dmaven.javadoc.skip=true -Dmaven.source.skip=true package \
            dependency:build-classpath -Dmdep.includeScope=runtime -Dmdep.outputFile=/sdk/$BIN/java-classpath.txt)
        mkdir -p $BIN/java && javac -encoding UTF-8 -source 7 -target 7 -nowarn \
            -cp src/wrappers/java/itscam-sdk/target/itscam-sdk-0.0.0.jar:\$(cat $BIN/java-classpath.txt) \
            -d $BIN/java tests/smoke/java/Smoke.java 2>&1 | grep -v -E '^(warning|Note|1 warning)' || true
        test -f $BIN/java/Smoke.class && echo 'Java smoke built'
        dotnet build -nologo -v q -c Release -o $BIN/csharp tests/smoke/csharp/Smoke.csproj > $BIN/csharp-build.log 2>&1 ||
            { tail -20 $BIN/csharp-build.log; exit 1; }
        echo 'C# smoke built'"
}

start_standin() {
    docker run -d --rm --name "$STANDIN" -u "$(id -u):$(id -g)" -e PORT=$PORT \
        -v "$ROOT/tests/standin:/standin:ro" --entrypoint node "$BUILDER_IMAGE" /standin/server.js >/dev/null || return 1
    for _ in $(seq 1 50); do
        docker logs "$STANDIN" 2>&1 | grep -q "stand-in listening" && return 0
        sleep 0.2
    done
    docker logs "$STANDIN"
    return 1
}

stop_standin() { docker rm -f "$STANDIN" >/dev/null 2>&1 || true; }

# run_on <arch> <image> <command...>: in that architecture's container, with
# the stand-in's network and the checkout read-only at /sdk
run_on() {
    local arch="$1" image="$2"; shift 2
    local ref; ref="$(platform_image "$image" "$(platform "$arch")")" || return 1
    docker run --rm --platform "$(platform "$arch")" --network "container:$STANDIN" \
        -v "$ROOT:/sdk:ro" -w /sdk -e PYTHONDONTWRITEBYTECODE=1 \
        -e ITSCAM_TEST_HOST=127.0.0.1 -e ITSCAM_TEST_PORT=$PORT \
        -e LD_LIBRARY_PATH="/sdk/src/core/build/$(core_dir "$arch")" \
        "$ref" "$@"
}

check_emulation() {
    local arch="$1"
    [ "$arch" = x64 ] && return 0
    local ref; ref="$(platform_image ubuntu:16.04 "$(platform "$arch")")" || return 1
    if ! docker run --rm --platform "$(platform "$arch")" "$ref" true 2>/dev/null; then
        echo "Docker can't run $(platform "$arch") containers: install qemu-user through binfmt_misc"
        echo "  docker run --privileged --rm tonistiigi/binfmt --install arm,arm64"
        return 1
    fi
}

run_arch() {
    local arch="$1" n; n="$(newer_name "$arch")"
    step "run $arch: emulation available" check_emulation "$arch" || return
    for std in c++14 c++17; do
        step "run $arch: C++ ($std, GCC 5.4) on Ubuntu 16.04" run_on "$arch" ubuntu:16.04 "$BIN/$arch/gcc5/smoke-cpp-$std"
        step "run $arch: C++ ($std, $n) on $(newer_runtime "$arch")" run_on "$arch" "$(newer_runtime "$arch")" "$BIN/$arch/$n/smoke-cpp-$std"
    done
    step "run $arch: C (GCC 5.4) on Ubuntu 16.04" run_on "$arch" ubuntu:16.04 "$BIN/$arch/gcc5/smoke-c"
    step "run $arch: C ($n) on $(newer_runtime "$arch")" run_on "$arch" "$(newer_runtime "$arch")" "$BIN/$arch/$n/smoke-c"
    step "run $arch: Go on $(newer_runtime "$arch")" run_on "$arch" "$(newer_runtime "$arch")" "$BIN/$arch/go/smoke"
    step "run $arch: Python 3.7" run_on "$arch" python:3.7-slim \
        env PYTHONPATH=/sdk/src/wrappers/python python3 tests/smoke/smoke.py
    step "run $arch: Node.js ($(node_image "$arch"))" run_on "$arch" "$(node_image "$arch")" node tests/smoke/nodejs/smoke.js
}

run_x64_only() {
    local lib="/sdk/src/core/build/linux/libitscam_sdk.so"
    local cp="src/wrappers/java/itscam-sdk/target/itscam-sdk-0.0.0.jar:\$(cat $BIN/java-classpath.txt):$BIN/java"
    for enc in UTF-8 US-ASCII; do
        step "run x64: Java (default charset $enc)" docker_run "$BUILDER_IMAGE" --network "container:$STANDIN" \
            -e ITSCAM_TEST_PORT=$PORT -- bash -c "java -Dfile.encoding=$enc -Ditscam.sdk.library=$lib -cp $cp Smoke"
    done
    step "run x64: C# (.NET 8)" docker_run "$BUILDER_IMAGE" --network "container:$STANDIN" \
        -e ITSCAM_TEST_PORT=$PORT -e LD_LIBRARY_PATH=/sdk/src/core/build/linux -- dotnet "$BIN/csharp/Smoke.dll"
}

step_cmake() {
    docker_run "$BUILDER_IMAGE" --network "container:$STANDIN" -e ITSCAM_TEST_PORT=$PORT -- tools/verify/cmake-matrix.sh
}

step_codegen() {
    docker_run "$BUILDER_IMAGE" -- bash -c "cd tools/codegen && npm ci --no-audit --no-fund --silent && node codegen.mjs --check"
}

step_java7() {
    docker_run "$BUILDER_IMAGE" -- make -s java-jdk7-check
}

# --- Run -------------------------------------------------------------------------

trap stop_standin EXIT
echo "verify: steps [$STEPS], architectures [$ARCHES]"
echo "images: $BUILDER_IMAGE, $CORE_IMAGE, $COMPAT_IMAGE"

step "version metadata" step_version
has_step cores && step "cores: Linux libraries ($ARCHES)" step_cores
has_step windows && step "windows: DLL and static library" step_windows
has_step consumers && step "consumers: C and C++ smoke tests, every compiler and standard" step_consumers
has_step wrappers && step "wrappers: Go, Node.js, Java and C# smoke tests" step_wrappers
if has_step run || has_step cmake; then
    step "stand-in camera started" start_standin
fi
if has_step run; then
    for arch in $ARCHES; do run_arch "$arch"; done
    [[ " $ARCHES " == *" x64 "* ]] && run_x64_only
fi
has_step cmake && step "cmake: build matrix" step_cmake
stop_standin
has_step compat && step "compat: API and ABI against $(grep -v '^#' "$ROOT/compat/baseline" | head -1)" \
    "$ROOT/tools/compat/check-compat.sh"
has_step codegen && step "codegen: generated REST types match the spec" step_codegen
has_step java7 && step "java7: the Java wrapper compiles with JDK 7" step_java7

summary
