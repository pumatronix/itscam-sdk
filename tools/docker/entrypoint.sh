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

exec "$@"
