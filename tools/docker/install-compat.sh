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

# apt-get update alone only warns when it can't fetch: --error-on=any makes
# it fail, so the loop can retry, then try another mirror.  GitHub's runners
# have failed to resolve archive.ubuntu.com for a whole job, while their own
# images use the Azure mirror.
update() {
    for attempt in 1 2 3; do
        if apt-get -o Acquire::Retries=3 update --error-on=any; then return 0; fi
        sleep $((attempt * 5))
    done
    return 1
}
if ! update; then
    for mirror in http://azure.archive.ubuntu.com/ubuntu http://mirrors.kernel.org/ubuntu; do
        echo "install-compat: archive.ubuntu.com unreachable, trying $mirror"
        sed -i -E "s#http://(archive|security)\\.ubuntu\\.com/ubuntu#$mirror#g" /etc/apt/sources.list
        if update; then break; fi
        if [ "$mirror" = http://mirrors.kernel.org/ubuntu ]; then exit 1; fi
        sed -i -E "s#$mirror#http://archive.ubuntu.com/ubuntu#g" /etc/apt/sources.list
    done
fi
apt-get install -y --no-install-recommends -o Acquire::Retries=5 abigail-tools python3 binutils
rm -rf /var/lib/apt/lists/*
