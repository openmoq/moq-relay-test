#!/usr/bin/env bash
# run_conformance.sh — Start moqtest_server connected to an external relay,
# run conformance_test.sh against that same relay, then stop the server.
#
# Usage: run_conformance.sh <relay_url> [versions] [Q|X]
#   relay_url  Relay URL to test against, e.g. https://relay.example.com:9668/moq-relay
#   versions   Comma-separated MoQT draft versions, e.g. "16"   (optional)
#   Q|X        Raw QUIC (Q) or QMUX-on-TCP (X) instead of WebTransport (optional)
#
# The moqtest_server connects to the relay as a publisher, announces the
# moq-test-00 namespace, and handles subscriptions forwarded by the relay.
# The conformance_test.sh then runs moqtest_client against the same relay URL.

set -euo pipefail

readonly SERVER_BIN=/opt/moq/moxygen/moqtest/moqtest_server
readonly CONFORMANCE_SCRIPT=/opt/moq/conformance/conformance_test.sh

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <relay_url> [versions] [Q]"
    exit 1
fi

RELAY_URL="$1"

# Same transport selector as conformance_test.sh: Q = raw QUIC, X = QMUX-on-TCP,
# anything else = WebTransport.
TRANSPORT=h3wt
VERSIONS=
for arg in "${@:2}"; do
    case "$arg" in
        Q) TRANSPORT=quic ;;
        X) TRANSPORT=qmux ;;
        *) VERSIONS="$arg" ;;
    esac
done

# Pin the server's relay leg to the same draft(s) the client tests.
SERVER_ARGS=(--relay_url="$RELAY_URL" --transport="$TRANSPORT" --versions="$VERSIONS")

# ── Ensure server binary exists ──────────────────────────────────────────────
if [[ ! -x "$SERVER_BIN" ]]; then
    echo "Error: moqtest_server not found at ${SERVER_BIN}." >&2
    echo "  Rebuild the image with WITH_CONFORMANCE=1 (the default)." >&2
    exit 1
fi

# ── Start moqtest_server connected to relay ──────────────────────────────────
echo "==> Starting moqtest_server → ${RELAY_URL}"
"${SERVER_BIN}" "${SERVER_ARGS[@]}" &
SERVER_PID=$!

cleanup() {
    echo "==> Stopping moqtest_server (pid=${SERVER_PID})"
    kill "${SERVER_PID}" 2>/dev/null || true
    wait "${SERVER_PID}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# Give the server a moment to connect to the relay and announce its namespace.
# moqtest_server logs "startRelayClient" once connected; 3 s is ample on LAN.
sleep 3

# Verify the server process is still alive (quick sanity check)
if ! kill -0 "${SERVER_PID}" 2>/dev/null; then
    echo "Error: moqtest_server exited immediately — check relay URL or TLS config." >&2
    exit 1
fi

echo "==> moqtest_server is running (pid=${SERVER_PID})"
echo

# ── Run conformance tests ────────────────────────────────────────────────────
MOXYGEN_DIR=/opt/moq "${CONFORMANCE_SCRIPT}" "$@"
