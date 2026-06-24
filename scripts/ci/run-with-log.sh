#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat >&2 <<'USAGE'
usage: scripts/ci/run-with-log.sh [--append] <log-file> <label> -- <command> [args...]

Runs a noisy CI command with full stdout/stderr written to <log-file> while
printing only periodic tail summaries to the GitHub Actions console.
USAGE
    exit 2
}

append=false
if [[ "${1:-}" == "--append" ]]; then
    append=true
    shift
fi

if [[ $# -lt 4 || "${3:-}" != "--" ]]; then
    usage
fi

log_file="$1"
label="$2"
shift 3

interval_s="${CAPOS_CI_LOG_INTERVAL_S:-120}"
tail_lines="${CAPOS_CI_LOG_TAIL_LINES:-25}"
success_tail_lines="${CAPOS_CI_LOG_SUCCESS_TAIL_LINES:-80}"
failure_tail_lines="${CAPOS_CI_LOG_FAILURE_TAIL_LINES:-240}"

case "$interval_s:$tail_lines:$success_tail_lines:$failure_tail_lines" in
    *[!0-9:]*|:*|*::*)
        echo "invalid CAPOS_CI_LOG_* numeric setting" >&2
        exit 2
        ;;
esac

mkdir -p "$(dirname "$log_file")"
if [[ "$append" == true ]]; then
    {
        echo
        echo "===== $label: $(date -u +'%Y-%m-%dT%H:%M:%SZ') ====="
    } >> "$log_file"
else
    : > "$log_file"
fi

cmd_pid=""
monitor_pid=""
cleanup() {
    if [[ -n "$monitor_pid" ]]; then
        kill "$monitor_pid" 2>/dev/null || true
    fi
    if [[ -n "$cmd_pid" ]]; then
        kill "$cmd_pid" 2>/dev/null || true
    fi
}
trap cleanup INT TERM

{
    echo "::group::$label"
    echo "Full log: $log_file"
    echo "Command: $*"
    echo "Console log policy: show the last $tail_lines lines every ${interval_s}s; keep complete output in the artifact."
} >&2

"$@" >> "$log_file" 2>&1 &
cmd_pid=$!

(
    sleep_pid=""
    monitor_cleanup() {
        if [[ -n "$sleep_pid" ]]; then
            kill "$sleep_pid" 2>/dev/null || true
        fi
        exit 0
    }
    trap monitor_cleanup INT TERM

    while true; do
        sleep "$interval_s" &
        sleep_pid=$!
        wait "$sleep_pid" || exit 0
        sleep_pid=""
        if ! kill -0 "$cmd_pid" 2>/dev/null; then
            exit 0
        fi
        {
            echo
            echo "[$(date -u +'%H:%M:%SZ')] $label is still running; recent log tail:"
            tail -n "$tail_lines" "$log_file" || true
        } >&2
    done
) &
monitor_pid=$!

set +e
wait "$cmd_pid"
status=$?
set -e

kill "$monitor_pid" 2>/dev/null || true
wait "$monitor_pid" 2>/dev/null || true
monitor_pid=""
cmd_pid=""

if [[ "$status" -eq 0 ]]; then
    {
        echo "$label completed successfully. Final log tail:"
        tail -n "$success_tail_lines" "$log_file" || true
        echo "::endgroup::"
    } >&2
else
    {
        echo "::error::$label failed with exit code $status. Showing the last $failure_tail_lines lines; complete log is in $log_file."
        tail -n "$failure_tail_lines" "$log_file" || true
        echo "::endgroup::"
    } >&2
fi

exit "$status"
