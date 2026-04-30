#!/usr/bin/env bash
# entrypoint.sh — Run the moq-interop test suite against an external relay.
#
# All arguments are forwarded verbatim to interop_tests.
#
# Examples
#   docker run --rm moq-interop:latest --relay=https://relay.example.com:4433/moq
#   docker run --rm moq-interop:latest --relay=https://host.docker.internal:9668/moq
#   docker run --rm moq-interop:latest --relay=https://relay.example.com:4433/moq --categories=Publisher
#   docker run --rm moq-interop:latest --list
#   docker run --rm -it moq-interop:latest /bin/bash

set -euo pipefail

readonly TESTS_BIN=/opt/moq/bin/interop_tests

# Allow direct execution of arbitrary commands (e.g. /bin/bash for debugging).
if [[ "$#" -gt 0 && ( "$1" == /* || "$1" == ./* ) ]]; then
    exec "$@"
fi

exec "$TESTS_BIN" "$@"
