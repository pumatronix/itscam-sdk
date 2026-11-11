#!/bin/sh
# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# install-compat.sh -- what the compat image adds to ubuntu:22.04: libabigail
# (abidiff) and Python, for tools/compat/check-compat.sh.
#
# Run by the Dockerfile's compat stage, and by CI in a `docker run` container
# it then commits as the image: on GitHub's runners, `docker build` steps
# can't resolve host names, while containers started by `docker run` can.

set -eux
export DEBIAN_FRONTEND=noninteractive

# apt fails on any fetch error and retries: apt-get update alone only warns
for attempt in 1 2 3 4 5; do
    if apt-get -o Acquire::Retries=5 update --error-on=any; then break; fi
    if [ "$attempt" = 5 ]; then exit 1; fi
    sleep $((attempt * 10))
done
apt-get install -y --no-install-recommends -o Acquire::Retries=5 abigail-tools python3 binutils
rm -rf /var/lib/apt/lists/*
