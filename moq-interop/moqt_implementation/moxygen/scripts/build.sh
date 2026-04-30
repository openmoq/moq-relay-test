#!/usr/bin/env bash
# build.sh — Main build driver for moxygen + Meta OSS dependencies.
#
# On first run, builds moxygen + all dependencies via setup-deps.sh.
# On subsequent runs, detects whether the moxygen submodule has changed
# and rebuilds only moxygen when needed.
#
# This script does NOT build moq-interop_tests. Use:
#   ../../../moq-interop_tests/scripts/build.sh
#
# Usage:
#   ./scripts/build.sh [--clean]
#
# Options:
#   --clean        Clean and rebuild all dependencies from scratch

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"

# Parse arguments: --clean flag
CLEAN_FLAG=""
if [[ "${1:-}" == "--clean" ]]; then
  CLEAN_FLAG="--clean"
fi

# ── Helper functions ──────────────────────────────────────────────────────────

moxygen_inst_dir() {
  python3 "$GETDEPS" show-inst-dir moxygen 2>/dev/null | tail -1
}

moxygen_is_installed() {
  local inst
  inst="$(moxygen_inst_dir)"
  [[ -n "$inst" ]] && ls "$inst"/lib/libmoxygen*.a &>/dev/null
}

moxygen_rev() {
  git -C "$MOXYGEN_DIR" rev-parse HEAD 2>/dev/null || echo "unknown"
}

moxygen_rev_stamp() {
  local inst
  inst="$(moxygen_inst_dir)"
  echo "${inst}/.moq-relay-moxygen.rev"
}

moxygen_changed() {
  local stamp
  stamp="$(moxygen_rev_stamp)"
  [[ ! -f "$stamp" ]] || [[ "$(cat "$stamp")" != "$(moxygen_rev)" ]]
}

write_rev_stamp() {
  local stamp
  stamp="$(moxygen_rev_stamp)"
  moxygen_rev > "$stamp"
}

# ── Sanity checks ─────────────────────────────────────────────────────────────

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: moxygen submodule not initialized." >&2
  echo "  Run: git submodule update --init" >&2
  exit 1
fi

# ── Main ──────────────────────────────────────────────────────────────────────

if [[ -n "$CLEAN_FLAG" ]]; then
  echo "==> --clean requested: rebuilding all dependencies..."
  "${SCRIPT_DIR}/setup-deps.sh" "$CLEAN_FLAG"
  write_rev_stamp
elif ! moxygen_is_installed; then
  echo "==> moxygen not found — running full setup..."
  "${SCRIPT_DIR}/setup-deps.sh"
  write_rev_stamp
elif moxygen_changed; then
  echo "==> moxygen submodule changed — rebuilding moxygen..."
  "${SCRIPT_DIR}/build-moxygen.sh"
  write_rev_stamp
else
  echo "==> moxygen is up to date."
fi

echo ""
echo "==> To build moq-interop_tests, run:"
echo "    cd ../../../moq-interop_tests && bash scripts/build.sh"
