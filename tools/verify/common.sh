# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# common.sh -- helpers shared by tools/verify/verify.sh and
# tools/compat/check-compat.sh.  Source it; it needs bash, docker and git.
#
# Images (built by `make docker-build docker-build-core-xenial
# docker-build-compat`, or by CI with buildx):
#   BUILDER_IMAGE  itscam-sdk-builder       wrappers, MinGW, Arm GNU-A 8.3, Go, .NET, Java, Node
#   CORE_IMAGE     itscam-sdk-core-xenial   Linux cores at the glibc 2.23 floor (GCC 5.4)
#   COMPAT_IMAGE   itscam-sdk-compat        abidiff, Python 3.10

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILDER_IMAGE="${BUILDER_IMAGE:-itscam-sdk-builder}"
CORE_IMAGE="${CORE_IMAGE:-itscam-sdk-core-xenial}"
COMPAT_IMAGE="${COMPAT_IMAGE:-itscam-sdk-compat}"

# Everything verify writes lives here (git-ignored)
VERIFY_DIR="$ROOT/build/verify"
# Package caches survive between runs (Maven, NuGet, npm, Go)
VERIFY_CACHE="${VERIFY_CACHE:-$VERIFY_DIR/cache}"

# A worktree's .git is a file pointing into the main checkout: mount that
# too, or git (version metadata, baseline sources) fails in the container
_git_mounts=()
_common="$(cd "$ROOT" && cd "$(git rev-parse --git-common-dir 2>/dev/null || echo .)" && pwd)"
if [ -n "$_common" ] && [[ "$_common" != "$ROOT"/* ]]; then
    _git_mounts=(-v "$_common:$_common")
fi

# docker_run <image> [docker options...] -- <command...>
# Runs as the host user with the checkout at /sdk.
docker_run() {
    local image="$1"; shift
    local opts=()
    while [ $# -gt 0 ] && [ "$1" != "--" ]; do opts+=("$1"); shift; done
    shift
    mkdir -p "$VERIFY_CACHE/home"
    docker run --rm \
        -v "$ROOT:/sdk" "${_git_mounts[@]}" \
        -v "$VERIFY_CACHE:/cache" \
        -w /sdk \
        -u "$(id -u):$(id -g)" \
        -e HOME=/cache/home \
        -e XDG_CACHE_HOME=/cache/home/.cache \
        -e NPM_CONFIG_CACHE=/cache/npm \
        -e DOTNET_CLI_HOME=/cache/home \
        -e NUGET_PACKAGES=/cache/nuget \
        -e DOTNET_CLI_TELEMETRY_OPTOUT=1 \
        -e DOTNET_NOLOGO=1 \
        -e GOPATH=/cache/go \
        -e GOMODCACHE=/cache/go/pkg/mod \
        -e GOCACHE=/cache/go/build-cache \
        -e GOFLAGS=-mod=mod \
        -e MAVEN_OPTS=-Dmaven.repo.local=/cache/m2 \
        "${opts[@]}" "$image" "$@"
}

# Official images are pulled by digest (tools/verify/images.lock), from the
# first registry that answers: AWS's public mirror of Docker Hub's library,
# then Docker Hub.  Both limit anonymous requests, and CI runners share
# addresses, so every request is retried.
VERIFY_REGISTRIES="${VERIFY_REGISTRIES-public.ecr.aws/docker/library docker.io/library}"
IMAGES_LOCK="$ROOT/tools/verify/images.lock"

# _manifest_digest <registry/image:tag> <platform>: that platform's digest
_manifest_digest() {
    docker manifest inspect "$1" 2>/dev/null | python3 -c '
import json, sys
os_, arch, *variant = sys.argv[1].split("/")
try:
    manifests = json.load(sys.stdin).get("manifests", [])
except ValueError:
    manifests = []
for m in manifests:
    p = m.get("platform", {})
    if p.get("os") == os_ and p.get("architecture") == arch and \
            (not variant or p.get("variant") == variant[0]):
        print(m["digest"]); break
' "$2"
}

# platform_image <image:tag> <platform>
# Prints a reference to that platform's variant, by digest, pulling it if
# needed.  Pulling by digest never retags a local <image:tag>.
platform_image() {
    local image="$1" platform="$2" digest registry ref attempt
    digest="$(awk -v i="$image" -v p="$platform" '$1 == i && $2 == p {print $3}' "$IMAGES_LOCK" | tail -1)"
    if [ -z "$digest" ]; then
        # Not pinned yet: look it up, and say what to add to the lock
        for attempt in 1 2 3 4 5; do
            for registry in $VERIFY_REGISTRIES; do
                digest="$(_manifest_digest "$registry/$image" "$platform")"
                [ -n "$digest" ] && break 2
            done
            sleep $((attempt * attempt))
        done
        if [ -z "$digest" ]; then
            echo "error: no $platform variant of $image found in: $VERIFY_REGISTRIES" >&2
            return 1
        fi
        echo "images.lock: add \"$image $platform $digest\"" >&2
    fi
    for registry in $VERIFY_REGISTRIES; do
        ref="$registry/${image%:*}@$digest"
        if docker image inspect "$ref" >/dev/null 2>&1; then echo "$ref"; return 0; fi
    done
    for attempt in 1 2 3 4; do
        for registry in $VERIFY_REGISTRIES; do
            ref="$registry/${image%:*}@$digest"
            if docker pull -q "$ref" >/dev/null 2>&1; then echo "$ref"; return 0; fi
        done
        sleep $((attempt * attempt))
    done
    echo "error: could not pull $image ($platform, $digest) from: $VERIFY_REGISTRIES" >&2
    return 1
}

# Results, printed by summary
_results=()
_failed=0

# step <name> <command...>: runs it, records PASS/FAIL, never stops the run
step() {
    local name="$1"; shift
    echo ""
    echo "=== $name"
    if "$@"; then
        _results+=("PASS  $name")
    else
        _results+=("FAIL  $name")
        _failed=1
    fi
}

summary() {
    echo ""
    echo "=== Summary"
    printf '%s\n' "${_results[@]}"
    if [ "$_failed" -ne 0 ]; then
        echo "FAILED"
        return 1
    fi
    echo "OK"
}
