#!/usr/bin/env bash
# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# Build and run every ITSCAM SDK example against a live camera.
#
# Assumptions (by design):
#   * HTTP on port 80 (no TLS)
#   * CGI endpoints are anonymous (configCgi.blockAPI=false)
#   * REST credentials are supplied for REST-backed examples
#   * Go Wails GUI example is skipped
#
# All file-producing examples run inside a gitignored run directory:
#   .regression/<YYYYMMDD-HHMMSS>_<camera_ip>/
#
# Usage:
#   tools/regression/run-examples.sh <camera_ip> <rest_user> <rest_password>
#
# Environment (optional):
#   BINARY_PASSWORD      TCP/binary auth password (default: same as REST password)
#   SKIP_BUILD=1         Skip the build phase
#   REGRESSION_BASE      Base directory for runs (default: <repo>/.regression)
#   REGRESSION_RUN_DIR   Override the per-run output directory (advanced)

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="${ROOT}/src"
LIB_DIR="${SRC}/core/build/linux"
CPP_BIN="${SRC}/examples/build"
GO_BIN="${SRC}/wrappers/go/examples"
PY_BIN="${SRC}/wrappers/python/examples"
PY_PKG="${SRC}/wrappers/python"
CSHARP_DIR="${SRC}/wrappers/csharp/examples"
REGRESSION_BASE="${REGRESSION_BASE:-${ROOT}/.regression}"

CAMERA_IP="${1:-}"
REST_USER="${2:-}"
REST_PASS="${3:-}"
BINARY_PASS="${BINARY_PASSWORD:-$REST_PASS}"

LONG_RUN_SEC="${LONG_RUN_SEC:-12}"
DEFAULT_TIMEOUT="${DEFAULT_TIMEOUT:-180}"

declare -a PASSED=()
declare -a FAILED=()
declare -a SKIPPED=()

usage() {
    cat <<EOF
Usage: $(basename "$0") <camera_ip> <rest_user> <rest_password>

Build and run all non-GUI ITSCAM SDK examples against a live camera over
plain HTTP.  CGI examples run without credentials; REST examples use the
supplied username and password.

Each run writes artifacts under a timestamped, camera-tagged folder inside
${REGRESSION_BASE}/ (gitignored).  Example:

  .regression/20260522-153045_10_8_19_8/cpp/itscam_cgi_example/snapshot-0.jpg

Environment:
  BINARY_PASSWORD      Password for TCP/binary client examples (default: REST password)
  SKIP_BUILD=1         Skip building; use existing binaries
  REGRESSION_BASE      Base directory for regression runs (default: .regression/)
  REGRESSION_RUN_DIR   Use this exact run directory instead of creating a new one
  LONG_RUN_SEC         Seconds for open-ended binary capture examples (default: 12)
  DEFAULT_TIMEOUT      Per-example wall-clock limit in seconds (default: 180)

Skipped: src/wrappers/go/examples/gui (Wails desktop GUI)
EOF
}

log()  { printf '==> %s\n' "$*"; }
die()  { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

need_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "Required command not found: $1"
}

have_cmd() {
    command -v "$1" >/dev/null 2>&1
}

sanitize_ip() {
    printf '%s' "$1" | tr '.:' '__'
}

run_example() {
    local name="$1"
    local timeout_sec="$2"
    shift 2

    log "RUN  ${name}"
    set +e
    timeout --foreground "${timeout_sec}" "$@"
    local rc=$?
    set -u

    if [[ $rc -eq 0 ]]; then
        PASSED+=("$name")
        log "PASS ${name}"
        return 0
    fi

    if [[ $rc -eq 124 ]]; then
        FAILED+=("${name} (timed out after ${timeout_sec}s)")
        log "FAIL ${name} (timeout)"
        return 1
    fi

    FAILED+=("${name} (exit ${rc})")
    log "FAIL ${name} (exit ${rc})"
    return 1
}

# Open-ended examples: timeout is expected once the soak window elapses.
run_long_example() {
    local name="$1"
    local soak_sec="$2"
    shift 2

    log "RUN  ${name} (soak ${soak_sec}s)"
    set +e
    timeout --foreground "$((soak_sec + 10))" "$@"
    local rc=$?
    set -u

    if [[ $rc -eq 0 || $rc -eq 124 ]]; then
        PASSED+=("$name")
        log "PASS ${name}"
        return 0
    fi

    FAILED+=("${name} (exit ${rc})")
    log "FAIL ${name} (exit ${rc})"
    return 1
}

with_dir() {
    local dir="$1"
    shift
    local prev="$PWD"
    mkdir -p "${dir}"
    cd "${dir}" || die "Cannot cd to ${dir}"
    "$@"
    local rc=$?
    cd "${prev}" || exit 1
    return $rc
}

example_dir() {
    local layer="$1"
    local name="$2"
    printf '%s/%s/%s' "${RUN_DIR}" "${layer}" "${name}"
}

build_all() {
    log "Building core library"
    make -C "${ROOT}" lib

    log "Building C++ examples"
    make -C "${ROOT}" examples

    if have_cmd go; then
        log "Building Go examples"
        for target in go-example go-rest-example go-cgi-example; do
            if ! make -C "${ROOT}" "${target}"; then
                SKIPPED+=("go/${target#go-} (build failed)")
            fi
        done
    else
        SKIPPED+=("go/* (go not installed)")
        log "Skipping Go examples (go not in PATH)"
    fi

    if have_cmd dotnet; then
        log "Building C# examples"
        make -C "${ROOT}" csharp-examples
    else
        SKIPPED+=("csharp/* (dotnet not installed)")
        log "Skipping C# examples (dotnet not in PATH)"
    fi
}

write_summary() {
    local summary="${RUN_DIR}/summary.txt"
    {
        echo "ITSCAM SDK example regression"
        echo "Run directory: ${RUN_DIR}"
        echo "Camera: http://${CAMERA_IP}:80"
        echo "REST user: ${REST_USER}"
        echo "Finished: $(date -Iseconds)"
        echo
        printf 'Passed (%d):\n' "${#PASSED[@]}"
        for name in "${PASSED[@]}"; do
            printf '  OK  %s\n' "$name"
        done
        if ((${#SKIPPED[@]} > 0)); then
            printf '\nSkipped (%d):\n' "${#SKIPPED[@]}"
            for name in "${SKIPPED[@]}"; do
                printf '  --  %s\n' "$name"
            done
        fi
        if ((${#FAILED[@]} > 0)); then
            printf '\nFailed (%d):\n' "${#FAILED[@]}"
            for name in "${FAILED[@]}"; do
                printf '  !!  %s\n' "$name"
            done
        fi
    } > "${summary}"
}

main() {
    if [[ $# -ne 3 || "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
        usage
        [[ $# -eq 0 || "${1:-}" == "-h" || "${1:-}" == "--help" ]] && exit 0
        exit 1
    fi

    need_cmd make
    need_cmd timeout

    if [[ ! -d "${LIB_DIR}" ]]; then
        mkdir -p "${LIB_DIR}"
    fi

    export LD_LIBRARY_PATH="${LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
    export PYTHONPATH="${PY_PKG}${PYTHONPATH:+:${PYTHONPATH}}"

    local safe_ip
    safe_ip="$(sanitize_ip "${CAMERA_IP}")"
    if [[ -n "${REGRESSION_RUN_DIR:-}" ]]; then
        RUN_DIR="${REGRESSION_RUN_DIR}"
    else
        RUN_DIR="${REGRESSION_BASE}/$(date +%Y%m%d-%H%M%S)_${safe_ip}"
    fi
    mkdir -p "${RUN_DIR}"

    log "Camera: http://${CAMERA_IP}:80"
    log "REST user: ${REST_USER}"
    log "Run directory: ${RUN_DIR}"

    if [[ "${SKIP_BUILD:-0}" != "1" ]]; then
        build_all
    else
        log "SKIP_BUILD=1 — using existing build outputs"
    fi

    # ------------------------------------------------------------------
    # C++ examples
    # ------------------------------------------------------------------
    if [[ -x "${CPP_BIN}/itscam_rest_example" ]]; then
        with_dir "$(example_dir cpp itscam_rest_example)" \
            run_example "cpp/itscam_rest_example" "${DEFAULT_TIMEOUT}" \
            "${CPP_BIN}/itscam_rest_example" "${CAMERA_IP}" 80 "${REST_USER}" "${REST_PASS}"
    else
        SKIPPED+=("cpp/itscam_rest_example (binary missing)")
    fi

    if [[ -x "${CPP_BIN}/itscam_cgi_example" ]]; then
        with_dir "$(example_dir cpp itscam_cgi_example)" \
            run_example "cpp/itscam_cgi_example" "${DEFAULT_TIMEOUT}" \
            "${CPP_BIN}/itscam_cgi_example" "${CAMERA_IP}"
    else
        SKIPPED+=("cpp/itscam_cgi_example (binary missing)")
    fi

    if [[ -x "${CPP_BIN}/itscam_sdk_example" ]]; then
        with_dir "$(example_dir cpp itscam_sdk_example)" \
            run_example "cpp/itscam_sdk_example" "${DEFAULT_TIMEOUT}" \
            "${CPP_BIN}/itscam_sdk_example" "${CAMERA_IP}" "${BINARY_PASS}"
    else
        SKIPPED+=("cpp/itscam_sdk_example (binary missing)")
    fi

    if [[ -x "${CPP_BIN}/itscam_trigger_recorder" ]]; then
        with_dir "$(example_dir cpp itscam_trigger_recorder)" \
            run_long_example "cpp/itscam_trigger_recorder" "${LONG_RUN_SEC}" \
            "${CPP_BIN}/itscam_trigger_recorder" "${CAMERA_IP}" . \
            -p "${BINARY_PASS}"
    else
        SKIPPED+=("cpp/itscam_trigger_recorder (binary missing)")
    fi

    # ------------------------------------------------------------------
    # Python examples
    # ------------------------------------------------------------------
    if have_cmd python3; then
        with_dir "$(example_dir python rest_example)" \
            run_example "python/rest_example" "${DEFAULT_TIMEOUT}" \
            python3 "${PY_BIN}/rest_example.py" "${CAMERA_IP}" "${REST_USER}" "${REST_PASS}"

        with_dir "$(example_dir python cgi_snapshot_example)" \
            run_example "python/cgi_snapshot_example" "${DEFAULT_TIMEOUT}" \
            python3 "${PY_BIN}/cgi_snapshot_example.py" "${CAMERA_IP}"

        with_dir "$(example_dir python capture_example)" \
            run_long_example "python/capture_example" "${LONG_RUN_SEC}" \
            python3 "${PY_BIN}/capture_example.py" "${CAMERA_IP}" "${BINARY_PASS}"
    else
        SKIPPED+=("python/* (python3 not installed)")
    fi

    # ------------------------------------------------------------------
    # Go examples (GUI skipped)
    # ------------------------------------------------------------------
    if [[ -x "${GO_BIN}/rest_example" ]]; then
        with_dir "$(example_dir go rest_example)" \
            run_example "go/rest_example" "${DEFAULT_TIMEOUT}" \
            "${GO_BIN}/rest_example" "${CAMERA_IP}" "${REST_USER}" "${REST_PASS}"
    elif have_cmd go; then
        SKIPPED+=("go/rest_example (binary missing)")
    fi

    if [[ -x "${GO_BIN}/cgi_snapshot_example" ]]; then
        with_dir "$(example_dir go cgi_snapshot_example)" \
            run_example "go/cgi_snapshot_example" "${DEFAULT_TIMEOUT}" \
            "${GO_BIN}/cgi_snapshot_example" "${CAMERA_IP}"
    elif have_cmd go; then
        SKIPPED+=("go/cgi_snapshot_example (binary missing)")
    fi

    if [[ -x "${GO_BIN}/capture_example" ]]; then
        with_dir "$(example_dir go capture_example)" \
            run_example "go/capture_example" "${DEFAULT_TIMEOUT}" \
            "${GO_BIN}/capture_example" "${CAMERA_IP}" "${BINARY_PASS}"
    elif have_cmd go; then
        SKIPPED+=("go/capture_example (binary missing)")
    fi

    # ------------------------------------------------------------------
    # C# examples
    # ------------------------------------------------------------------
    if have_cmd dotnet; then
        local cap_proj="${CSHARP_DIR}/CaptureExample"
        if [[ -f "${cap_proj}/bin/Release/net8.0/CaptureExample.dll" ]]; then
            with_dir "$(example_dir csharp CaptureExample)" \
                run_example "csharp/CaptureExample" "${DEFAULT_TIMEOUT}" \
                env LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" \
                dotnet exec "${cap_proj}/bin/Release/net8.0/CaptureExample.dll" \
                "${CAMERA_IP}" --user "${REST_USER}" --password "${REST_PASS}"
        else
            SKIPPED+=("csharp/CaptureExample (not built)")
        fi

        local mjpeg_proj="${CSHARP_DIR}/MjpegGrabberExample"
        if [[ -f "${mjpeg_proj}/bin/Release/net8.0/MjpegGrabberExample.dll" ]]; then
            with_dir "$(example_dir csharp MjpegGrabberExample)" \
                run_example "csharp/MjpegGrabberExample" "${DEFAULT_TIMEOUT}" \
                env LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" \
                dotnet exec "${mjpeg_proj}/bin/Release/net8.0/MjpegGrabberExample.dll" \
                "${CAMERA_IP}" --user "${REST_USER}" --password "${REST_PASS}" \
                --duration 3 --framerate 30
        else
            SKIPPED+=("csharp/MjpegGrabberExample (not built)")
        fi

        local snap_proj="${CSHARP_DIR}/SoftwareTriggerSnapshotExample"
        if [[ -f "${snap_proj}/bin/Release/net8.0/SoftwareTriggerSnapshotExample.dll" ]]; then
            with_dir "$(example_dir csharp SoftwareTriggerSnapshotExample)" \
                run_example "csharp/SoftwareTriggerSnapshotExample" "${DEFAULT_TIMEOUT}" \
                env LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" \
                dotnet exec "${snap_proj}/bin/Release/net8.0/SoftwareTriggerSnapshotExample.dll" \
                "${CAMERA_IP}" --user "${REST_USER}" --password "${REST_PASS}" \
                --count 1
        else
            SKIPPED+=("csharp/SoftwareTriggerSnapshotExample (not built)")
        fi

        local binary_proj="${CSHARP_DIR}/BinaryCaptureExample"
        if [[ -f "${binary_proj}/bin/Release/net8.0/BinaryCaptureExample.dll" ]]; then
            with_dir "$(example_dir csharp BinaryCaptureExample)" \
                run_long_example "csharp/BinaryCaptureExample" "${LONG_RUN_SEC}" \
                env LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" \
                dotnet exec "${binary_proj}/bin/Release/net8.0/BinaryCaptureExample.dll" \
                "${CAMERA_IP}" "${BINARY_PASS}"
        else
            SKIPPED+=("csharp/BinaryCaptureExample (not built)")
        fi
    fi

    # ------------------------------------------------------------------
    # Summary
    # ------------------------------------------------------------------
    echo
    echo "==================== regression summary ===================="
    printf 'Passed (%d):\n' "${#PASSED[@]}"
    for name in "${PASSED[@]}"; do
        printf '  OK  %s\n' "$name"
    done

    if ((${#SKIPPED[@]} > 0)); then
        printf '\nSkipped (%d):\n' "${#SKIPPED[@]}"
        for name in "${SKIPPED[@]}"; do
            printf '  --  %s\n' "$name"
        done
    fi

    if ((${#FAILED[@]} > 0)); then
        printf '\nFailed (%d):\n' "${#FAILED[@]}"
        for name in "${FAILED[@]}"; do
            printf '  !!  %s\n' "$name"
        done
        write_summary
        echo "Artifacts: ${RUN_DIR}"
        echo "=========================================================="
        exit 1
    fi

    write_summary
    echo "=========================================================="
    echo "All runnable examples passed."
    echo "Artifacts: ${RUN_DIR}"
    exit 0
}

main "$@"
