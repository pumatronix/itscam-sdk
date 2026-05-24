#!/usr/bin/env bash
# Refuse to run bind-mounted builds as root -- files would be root-owned on
# the host and break subsequent native `make` runs.
set -euo pipefail

if [ "$(id -u)" -eq 0 ]; then
    cat >&2 <<'EOF'
error: refusing to run as root with /sdk bind-mounted.

Docker would write root-owned files into your checkout. Re-run with your
host uid/gid, for example:

  docker run --rm -v "$(pwd)":/sdk -w /sdk \
    -u "$(id -u):$(id -g)" itscam-sdk-builder make all

Or use the Makefile wrappers (they pass -u automatically):

  make docker-all
  make docker-linux
EOF
    exit 1
fi

# Go module/build cache: image GOPATH (/go) is owned by the image's builder
# user (uid 1000), but bind-mount builds run as the host uid (Makefile
# DOCKER_RUN, GitHub Actions, etc.). Keep wails/gomarkdoc on PATH via
# /go/bin; only redirect writable cache dirs.
export GOPATH="${GOPATH:-/tmp/go}"
export GOMODCACHE="${GOMODCACHE:-/tmp/go/pkg/mod}"
export GOCACHE="${GOCACHE:-/tmp/go/build-cache}"
mkdir -p "$GOMODCACHE" "$GOCACHE"

# Ensure git-lfs filters are configured so `git status` correctly handles
# LFS-tracked files (avoids false dirty detection on bind mounts where the
# host smudged LFS pointers into actual binary content).
git lfs install --skip-repo >/dev/null 2>&1 || true

exec "$@"
