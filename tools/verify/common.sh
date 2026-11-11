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

# Official images come from AWS's public mirror of Docker Hub's library,
# which has no anonymous pull-rate limit (Docker Hub's is easily reached by
# a run, and CI runners share addresses).  VERIFY_REGISTRY= uses Docker Hub.
VERIFY_REGISTRY="${VERIFY_REGISTRY-public.ecr.aws/docker/library}"

# platform_image <image:tag> <platform>
# Prints a reference to that platform's variant, by digest, pulling it if
# needed.  A digest is looked up once per machine (cached in VERIFY_CACHE),
# and pulling by digest never retags a local <image:tag>.
platform_image() {
    local image="$1" platform="$2" src digest
    src="${VERIFY_REGISTRY:+$VERIFY_REGISTRY/}$image"
    local cache="$VERIFY_CACHE/image-digests.txt"
    mkdir -p "$VERIFY_CACHE"
    digest="$(awk -v s="$src" -v p="$platform" '$1 == s && $2 == p {print $3}' "$cache" 2>/dev/null | tail -1)"
    local attempt
    for attempt in 1 2 3 4 5 6; do
        [ -n "$digest" ] && break
        # Registries limit bursts of manifest requests: back off and retry
        [ "$attempt" -gt 1 ] && sleep $((attempt * attempt))
        digest="$(docker manifest inspect "$src" 2>/dev/null | python3 -c '
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
' "$platform")"
        [ -n "$digest" ] && echo "$src $platform $digest" >> "$cache"
    done
    if [ -z "$digest" ]; then
        echo "error: no $platform variant of $src (docker manifest inspect: $(docker manifest inspect "$src" 2>&1 | head -c 200))" >&2
        return 1
    fi
    local ref="${src%:*}@$digest"
    docker image inspect "$ref" >/dev/null 2>&1 || docker pull -q "$ref" >/dev/null || return 1
    echo "$ref"
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
