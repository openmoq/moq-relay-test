#!/usr/bin/env bash
# build.sh — Main build driver for moq-interop_tests.
#
# On first run, builds moxygen + all Meta OSS deps from source via
# setup-deps.sh, then configures and compiles moq-interop_tests.
# On subsequent runs, detects whether the moxygen submodule has changed
# and only rebuilds what is necessary.
#
# getdeps uses .scratch/ (inside this folder) as its scratch path.
#
# Usage:
#   ./scripts/build.sh [sanitize_type]
#
# Examples:
#   ./scripts/build.sh                 # default build  → build/
#   ./scripts/build.sh address         # ASAN           → build_address/
#   ./scripts/build.sh thread          # TSAN           → build_thread/
#   ./scripts/build.sh undefined       # UBSAN          → build_undefined/

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INTEROP_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"
STAMP_DIR="${INTEROP_ROOT}/.scratch"

SANITIZE="${1:-}"
SOURCE_DIR="${INTEROP_ROOT}/moq-interop_tests"
BUILD_DIR="${SOURCE_DIR}/build$([ -n "$SANITIZE" ] && echo "_${SANITIZE}" || echo "")"

MOXYGEN_REV_FILE="${STAMP_DIR}/moxygen.rev"

# ── Helper functions ──────────────────────────────────────────────────────────

needs_setup() {
  [[ ! -f "$MOXYGEN_REV_FILE" ]]
}

moxygen_rev() {
  git -C "$MOXYGEN_DIR" rev-parse HEAD 2>/dev/null || echo "unknown"
}

moxygen_changed() {
  local current
  current=$(moxygen_rev)
  if [[ ! -f "$MOXYGEN_REV_FILE" ]] || [[ "$current" != "$(cat "$MOXYGEN_REV_FILE")" ]]; then
    return 0
  fi
  return 1
}

needs_configure() {
  [[ ! -f "${BUILD_DIR}/build.ninja" ]]
}

# ── Sanity checks ─────────────────────────────────────────────────────────────

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: moxygen submodule not initialized." >&2
  echo "  Run: git submodule update --init" >&2
  exit 1
fi

# ── Set up cmake / ninja / compilers from getdeps ────────────────────────────

eval "$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" env moxygen)"

# ── Main ──────────────────────────────────────────────────────────────────────

NEED_CONFIGURE=false

if needs_setup; then
  echo "==> No prior build found — running full setup..."
  "${SCRIPT_DIR}/setup-deps.sh"
  NEED_CONFIGURE=true
elif moxygen_changed; then
  echo "==> Moxygen submodule changed — rebuilding moxygen..."
  "${SCRIPT_DIR}/build-moxygen.sh"
  NEED_CONFIGURE=true
fi

if [[ "$NEED_CONFIGURE" == true ]] || needs_configure; then
  "${SCRIPT_DIR}/configure.sh" "$BUILD_DIR" "$SANITIZE"
fi

echo "==> Building moq-interop_tests ($([ -n "$SANITIZE" ] && echo "sanitizer=${SANITIZE}" || echo "release"))..."
NPROC=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
cmake --build "$BUILD_DIR" -- -j"$NPROC"

BINARY="${BUILD_DIR}/bin/interop_tests"
if [[ -f "$BINARY" ]]; then
  echo "==> Build complete: ${BINARY}"
else
  echo "==> Build complete: ${BUILD_DIR}"
fi
