#!/usr/bin/env bash
# entrypoint.sh — Runtime dispatcher for the moq-interop container.
#
# Modes
#   all    (default) Run the full interop test suite. Relay must be running
#                    separately at the configured endpoint.
#   tests            Run only the interop_tests binary.  Point it at an
#                    external relay with --relay=<url>.
#   /bin/bash        (or any other path) — bypasses the dispatcher and is
#                    executed directly; useful for debugging.
#
# Environment variables (all have sensible defaults)
#   RELAY_PORT       UDP/TCP port of the relay to connect to    (default: 4433)
#   RELAY_ENDPOINT   HTTP path for the MoQ endpoint            (default: /moq)
#
# Extra arguments
#   Any arguments after the mode are forwarded verbatim to the binary being run.
#
# Examples
#   docker run --rm moq-interop:latest            # run all tests (relay running separately)
#   docker run --rm moq-interop:latest tests --relay=https://relay.host:4433/moq
#   docker run --rm moq-interop:latest all --categories=Publisher
#   docker run --rm -it moq-interop:latest /bin/bash

set -euo pipefail

# ── Binary paths ─────────────────────────────────────────────────────────────
readonly TESTS_BIN=/opt/moq/bin/interop_tests

# ── Configuration from environment ───────────────────────────────────────────
PORT="${RELAY_PORT:-4433}"
ENDPOINT="${RELAY_ENDPOINT:-/moq}"

# ── Argument parsing ──────────────────────────────────────────────────────────
MODE="${1:-all}"
shift || true   # remaining args are forwarded to the binary

# If the first argument looks like an executable path, exec it directly.
if [[ "$MODE" == /* || "$MODE" == ./* ]]; then
    exec "$MODE" "$@"
fi

# ── Helpers ───────────────────────────────────────────────────────────────────
log() { echo "[entrypoint] $*" >&2; }

run_tests() {
    local relay_url="${1:-https://localhost:${PORT}${ENDPOINT}}"
    shift || true
    log "Running interop tests against ${relay_url}"
    exec "$TESTS_BIN" --relay="$relay_url" "$@"
}

# ── Mode dispatch ─────────────────────────────────────────────────────────────
case "$MODE" in


  # ── tests ──────────────────────────────────────────────────────────────────
  tests)
    # Allow the caller to override the relay URL via --relay=<url>; otherwise
    # default to localhost so the container can reach a sidecar relay.
    "$TESTS_BIN" "$@"
    ;;

  # ── all (default) ──────────────────────────────────────────────────────────
  all)
    # Run the test suite against the relay running separately.
    RELAY_URL="https://localhost:${PORT}${ENDPOINT}"
    log "Running interop tests against ${RELAY_URL}"
    exec "$TESTS_BIN" --relay="$RELAY_URL" "$@"
    ;;

  *)
    echo "Usage: $0 {all|tests} [args...]" >&2
    echo ""                                        >&2
    echo "  all   — run all interop tests (default)"    >&2
    echo "  tests — run only the interop tests"                        >&2
    exit 1
    ;;
esac
