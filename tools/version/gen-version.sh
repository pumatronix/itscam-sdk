#!/usr/bin/env bash
#
# Generate SDK version metadata from git tags, commit SHA, and build date.
# Writes:
#   tools/version/sdk-version.mk
#   src/core/itscam_sdk_version.h
#   src/wrappers/python/itscam/_version.py
#   src/wrappers/csharp/Version.props
#   src/wrappers/go/itscam/version.go
#   src/wrappers/java/itscam-sdk/src/main/java/com/pumatronix/itscam/internal/SdkVersion.java
#   src/wrappers/nodejs/src/version.js
#
# Copyright (c) 2026 Pumatronix

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT_MK="$ROOT/tools/version/sdk-version.mk"

python3 - "$ROOT" "$OUT_MK" <<'PY'
from __future__ import annotations

import datetime as dt
import os
import re
import subprocess
import sys
from pathlib import Path

root = Path(sys.argv[1])
out_mk = Path(sys.argv[2])

SEMVER_RE = re.compile(
    r"^(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)(?:-(?P<prerelease>[^+]+))?(?:\+(?P<buildmeta>.+))?$"
)


def run(*args: str, cwd: Path = root) -> str:
    return subprocess.check_output(args, cwd=cwd, stderr=subprocess.DEVNULL, text=True).strip()


def try_run(*args: str, cwd: Path = root) -> str | None:
    try:
        return run(*args, cwd=cwd)
    except subprocess.CalledProcessError:
        return None


def git_available() -> bool:
    try:
        run("git", "rev-parse", "HEAD")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def fallback_from_header() -> str:
    version_header = root / "src/core/itscam_sdk_version.h"
    if version_header.exists():
        for line in version_header.read_text(encoding="utf-8").splitlines():
            if line.startswith('#define ITSCAM_SDK_VERSION_STRING "'):
                return line.split('"')[1]
    return "0.0.0"


def parse_semver(version: str) -> dict[str, str | int]:
    match = SEMVER_RE.match(version)
    if not match:
        raise SystemExit(f"invalid semver: {version!r}")
    return {
        "major": int(match.group("major")),
        "minor": int(match.group("minor")),
        "patch": int(match.group("patch")),
        "prerelease": match.group("prerelease") or "",
        "buildmeta": match.group("buildmeta") or "",
    }


def compute() -> dict[str, str | int]:
    build_date = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    build_date_short = build_date[:10]

    if git_available():
        git_sha = run("git", "rev-parse", "HEAD")
        git_sha_short = run("git", "rev-parse", "--short=12", "HEAD")
        # `git status` can fail with exit 128 inside Docker bind mounts when
        # the repo is owned by a different uid (Git "dubious ownership").
        porcelain = try_run("git", "status", "--porcelain")
        if porcelain:
            # Exclude LFS-tracked binaries that may appear modified inside
            # Docker containers without git-lfs.  Only genuinely unexpected
            # changes should flag the build as dirty.
            import fnmatch
            lfs_patterns: list[str] = []
            ga = root / ".gitattributes"
            if ga.exists():
                for _line in ga.read_text(encoding="utf-8").splitlines():
                    _parts = _line.split()
                    if len(_parts) >= 2 and any("filter=lfs" in p for p in _parts[1:]):
                        lfs_patterns.append(_parts[0])
            filtered = []
            for _line in porcelain.splitlines():
                _path = _line[2:].lstrip(" ").split(" -> ")[-1].strip()
                if lfs_patterns and any(fnmatch.fnmatch(_path, p) for p in lfs_patterns):
                    continue
                filtered.append(_line)
            porcelain = "\n".join(filtered)
        dirty = porcelain is not None and porcelain != ""
        tag = try_run("git", "describe", "--tags", "--match", "v*", "--abbrev=0")
        if tag is not None:
            base_version = tag[1:] if tag.startswith("v") else tag
            count = try_run("git", "rev-list", "--count", f"{tag}..HEAD")
            commits_since = int(count) if count is not None else 0
        else:
            base_version = fallback_from_header()
            commits_since = 0
    else:
        git_sha = "unknown"
        git_sha_short = "unknown"
        dirty = False
        base_version = fallback_from_header()
        commits_since = 0

    base = parse_semver(base_version.split("+", 1)[0].split("-", 1)[0])
    lib_version = f"{base['major']}.{base['minor']}.{base['patch']}"

    if commits_since > 0:
        nuget_version = f"{lib_version}-dev.{commits_since}"
        python_version = f"{lib_version}.dev{commits_since}"
        # Maven uses -SNAPSHOT for non-release builds; npm follows SemVer
        # pre-release rules (lowercase, hyphen-prefixed segments).
        maven_version = f"{lib_version}-SNAPSHOT"
        npm_version = f"{lib_version}-dev.{commits_since}"
    elif dirty:
        nuget_version = f"{lib_version}-dev.0"
        python_version = f"{lib_version}.dev0"
        maven_version = f"{lib_version}-SNAPSHOT"
        npm_version = f"{lib_version}-dev.0"
    else:
        nuget_version = lib_version
        python_version = lib_version
        maven_version = lib_version
        npm_version = lib_version

    if dirty:
        build_meta = f"dirty.{git_sha_short}"
    elif git_sha_short != "unknown":
        build_meta = git_sha_short
    else:
        build_meta = ""

    if build_meta:
        package_version = f"{nuget_version}+{build_meta}"
        python_version = f"{python_version}+{build_meta}"
    else:
        package_version = nuget_version

    version_full = f"{package_version} ({git_sha_short}, {build_date_short})"

    return {
        "major": base["major"],
        "minor": base["minor"],
        "patch": base["patch"],
        "lib_version": lib_version,
        "base_version": lib_version,
        "nuget_version": nuget_version,
        "python_version": python_version,
        "maven_version": maven_version,
        "npm_version": npm_version,
        "package_version": package_version,
        "version_full": version_full,
        "git_sha": git_sha,
        "git_sha_short": git_sha_short,
        "build_date": build_date,
        "build_date_short": build_date_short,
        "commits_since_tag": commits_since,
        "dirty": "1" if dirty else "0",
    }


info = compute()


def safe_write(path: Path, text: str) -> None:
    """Remove then write -- avoids PermissionError when a previous Docker
    build left the file owned by root."""
    try:
        path.unlink(missing_ok=True)
    except OSError:
        pass
    path.write_text(text, encoding="utf-8")


core_header = root / "src/core/itscam_sdk_version.h"
safe_write(core_header,
    f"""/* Auto-generated by tools/version/gen-version.sh. Do not edit. */
#pragma once

#define ITSCAM_SDK_VERSION_MAJOR {info['major']}
#define ITSCAM_SDK_VERSION_MINOR {info['minor']}
#define ITSCAM_SDK_VERSION_PATCH {info['patch']}
#define ITSCAM_SDK_VERSION_STRING "{info['lib_version']}"
#define ITSCAM_SDK_VERSION_FULL "{info['version_full']}"
#define ITSCAM_SDK_GIT_SHA "{info['git_sha']}"
#define ITSCAM_SDK_GIT_SHA_SHORT "{info['git_sha_short']}"
#define ITSCAM_SDK_BUILD_DATE "{info['build_date']}"
""")

py_version = root / "src/wrappers/python/itscam/_version.py"
safe_write(py_version,
    f'''"""Auto-generated by tools/version/gen-version.sh. Do not edit."""
__version__ = "{info['python_version']}"
__version_full__ = "{info['version_full']}"
__git_sha__ = "{info['git_sha']}"
__git_sha_short__ = "{info['git_sha_short']}"
__build_date__ = "{info['build_date']}"
''')

csharp_props = root / "src/wrappers/csharp/Version.props"
safe_write(csharp_props,
    f"""<?xml version="1.0" encoding="utf-8"?>
<!-- Auto-generated by tools/version/gen-version.sh. Do not edit. -->
<Project>
  <PropertyGroup>
    <Version>{info['nuget_version']}</Version>
    <InformationalVersion>{info['version_full']}</InformationalVersion>
    <ItscamLibVersion>{info['lib_version']}</ItscamLibVersion>
    <ItscamGitSha>{info['git_sha_short']}</ItscamGitSha>
    <ItscamBuildDate>{info['build_date']}</ItscamBuildDate>
  </PropertyGroup>
</Project>
""")

go_version = root / "src/wrappers/go/itscam/version.go"
safe_write(go_version,
    f"""// Code generated by tools/version/gen-version.sh; DO NOT EDIT.

package itscam

const (
\tSDKVersion      = "{info['nuget_version']}"
\tSDKVersionFull  = "{info['version_full']}"
\tSDKGitSHA       = "{info['git_sha']}"
\tSDKGitSHAShort  = "{info['git_sha_short']}"
\tSDKBuildDate    = "{info['build_date']}"
)

// GetSDKVersion returns the SDK package version string.
func GetSDKVersion() string {{ return SDKVersion }}

// GetSDKVersionFull returns version, git SHA, and build date.
func GetSDKVersionFull() string {{ return SDKVersionFull }}
""")

java_version = root / "src/wrappers/java/itscam-sdk/src/main/java/com/pumatronix/itscam/internal/SdkVersion.java"
java_version.parent.mkdir(parents=True, exist_ok=True)
safe_write(java_version,
    f"""// Code generated by tools/version/gen-version.sh; DO NOT EDIT.

package com.pumatronix.itscam.internal;

/** Auto-generated package metadata for the ITSCAM Java wrapper. */
public final class SdkVersion {{

    public static final String VERSION = "{info['maven_version']}";
    public static final String VERSION_FULL = "{info['version_full']}";
    public static final String LIB_VERSION = "{info['lib_version']}";
    public static final String GIT_SHA = "{info['git_sha']}";
    public static final String GIT_SHA_SHORT = "{info['git_sha_short']}";
    public static final String BUILD_DATE = "{info['build_date']}";

    private SdkVersion() {{ /* no instances */ }}
}}
""")

nodejs_version = root / "src/wrappers/nodejs/src/version.js"
safe_write(nodejs_version,
    f"""// Auto-generated by tools/version/gen-version.sh; DO NOT EDIT.
'use strict';

module.exports = {{
    VERSION:        '{info['npm_version']}',
    VERSION_FULL:   '{info['version_full']}',
    LIB_VERSION:    '{info['lib_version']}',
    GIT_SHA:        '{info['git_sha']}',
    GIT_SHA_SHORT:  '{info['git_sha_short']}',
    BUILD_DATE:     '{info['build_date']}',
}};
""")


def mk_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace("$", "$$")


safe_write(out_mk,
    "\n".join(
        [
            "# Auto-generated by tools/version/gen-version.sh -- do not edit.",
            f"SDK_VERSION_MAJOR := {info['major']}",
            f"SDK_VERSION_MINOR := {info['minor']}",
            f"SDK_VERSION_PATCH := {info['patch']}",
            f"SDK_LIB_VERSION := {info['lib_version']}",
            f"SDK_VERSION := {mk_escape(str(info['package_version']))}",
            f"SDK_VERSION_FULL := {mk_escape(str(info['version_full']))}",
            f"SDK_MAVEN_VERSION := {mk_escape(str(info['maven_version']))}",
            f"SDK_GIT_SHA := {mk_escape(str(info['git_sha']))}",
            f"SDK_GIT_SHA_SHORT := {mk_escape(str(info['git_sha_short']))}",
            f"SDK_BUILD_DATE := {mk_escape(str(info['build_date']))}",
            "",
        ]
    ))

version_json = root / "VERSION.json"
safe_write(version_json,
    __import__("json").dumps(
        {
            "version": info["package_version"],
            "nugetVersion": info["nuget_version"],
            "pythonVersion": info["python_version"],
            "mavenVersion": info["maven_version"],
            "npmVersion": info["npm_version"],
            "libVersion": info["lib_version"],
            "versionFull": info["version_full"],
            "gitSha": info["git_sha"],
            "gitShaShort": info["git_sha_short"],
            "buildDate": info["build_date"],
            "commitsSinceTag": info["commits_since_tag"],
            "dirty": bool(int(info["dirty"])),
            "platforms": ["linux-x64", "win-x64", "win-x86"],
        },
        indent=2,
    )
    + "\n")

print(
    f"SDK version {info['package_version']} "
    f"(lib {info['lib_version']}, {info['git_sha_short']}, {info['build_date_short']})"
)
PY
