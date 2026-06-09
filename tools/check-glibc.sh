#!/usr/bin/env bash
# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# check-glibc.sh -- assert that a shared object only references glibc
# symbols up to a given maximum version.
#
# Usage:
#   tools/check-glibc.sh <file.so> <max-glibc-version>
#
# Example:
#   tools/check-glibc.sh src/core/build/linux/libitscam_sdk.so 2.27
#
# The check uses the cross-platform `objdump -T` (binutils) so it works
# for native and cross-compiled binaries.  It scans for GLIBC_<x>.<y>
# version tags and fails if any version exceeds the cap.
#
# Exits 0 if every referenced GLIBC_ symbol is <= the cap, non-zero on
# any breach (with a list of offending symbols).

set -euo pipefail

if [ $# -ne 2 ]; then
    echo "usage: $(basename "$0") <file> <max-glibc-version>" >&2
    exit 64
fi

file="$1"
cap="$2"

if [ ! -f "$file" ]; then
    echo "check-glibc: file not found: $file" >&2
    exit 1
fi

# Pick the right objdump.  Cross-arch binaries can be inspected with the
# host objdump just fine (binutils is multi-arch by default), but if a
# cross binutils is available we prefer it for clearer arch reporting.
OBJDUMP="${OBJDUMP:-objdump}"
if ! command -v "$OBJDUMP" >/dev/null 2>&1; then
    echo "check-glibc: $OBJDUMP not found in PATH" >&2
    exit 1
fi

cap_major="${cap%%.*}"
cap_minor="${cap##*.}"

if ! [[ "$cap_major" =~ ^[0-9]+$ && "$cap_minor" =~ ^[0-9]+$ ]]; then
    echo "check-glibc: invalid cap '$cap' (expected MAJOR.MINOR, e.g. 2.27)" >&2
    exit 64
fi

# Extract every distinct GLIBC_<major>.<minor> tag referenced by the file.
mapfile -t versions < <(
    "$OBJDUMP" -T "$file" 2>/dev/null |
    grep -oE 'GLIBC_[0-9]+\.[0-9]+' |
    sort -u
)

bad=()
for v in "${versions[@]}"; do
    ver="${v#GLIBC_}"
    major="${ver%%.*}"
    minor="${ver##*.}"
    if (( major > cap_major )) || (( major == cap_major && minor > cap_minor )); then
        bad+=("$v")
    fi
done

if [ "${#bad[@]}" -ne 0 ]; then
    echo "check-glibc: FAIL ${file}" >&2
    echo "  cap:    GLIBC_${cap}" >&2
    echo "  bad:    ${bad[*]}" >&2
    echo "" >&2
    echo "  Offending symbols:" >&2
    for v in "${bad[@]}"; do
        "$OBJDUMP" -T "$file" 2>/dev/null |
            awk -v v="$v" '$0 ~ v {printf("    %-12s %s\n", v, $NF)}' >&2
    done
    exit 1
fi

echo "check-glibc: OK   ${file}  (<= GLIBC_${cap}; tags seen: ${versions[*]:-none})"
