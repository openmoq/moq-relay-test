#!/usr/bin/env bash
# build-moxygen.sh — Rebuild just the moxygen library.
#
# Assumes Meta OSS deps (folly, fizz, wangle, mvfst, proxygen) are already
# installed by a prior setup-deps.sh run. Only re-compiles moxygen itself
# using getdeps default paths, then refreshes the moxygen rev stamp.
#
# Usage:
#   ./scripts/build-moxygen.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INTEROP_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"
STAMP_DIR="${INTEROP_ROOT}/.scratch"

# ── Sanity checks ─────────────────────────────────────────────────────────────

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: getdeps.py not found. Did you init submodules?" >&2
  exit 1
fi

if [[ ! -f "${STAMP_DIR}/moxygen.rev" ]]; then
  echo "Error: no prior setup found. Run ./scripts/setup-deps.sh first." >&2
  exit 1
fi

# ── Rebuild moxygen ───────────────────────────────────────────────────────────

echo "==> Rebuilding moxygen..."
cd "$MOXYGEN_DIR"
python3 "$GETDEPS" build --no-tests moxygen

# ── Refresh stamp ─────────────────────────────────────────────────────────────

git -C "$MOXYGEN_DIR" rev-parse HEAD > "${STAMP_DIR}/moxygen.rev"

echo "==> moxygen rebuild complete."
