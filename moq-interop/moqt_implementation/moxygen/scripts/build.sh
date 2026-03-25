#!/usr/bin/env bash
# build.sh — Main build driver for moq-interop_tests.
#
# On first run, builds moxygen + all Meta OSS deps from source via
# setup-deps.sh, then configures and compiles moq-interop_tests.
# On subsequent runs, detects whether the moxygen submodule has changed
# and only rebuilds what is necessary.
#
# Usage:
#   ./scripts/build.sh [preset] [--clean]
#
# Presets (from CMakePresets.json):
#   moxygen        — full build with moxygen_adapter  (default) → build/
#   moxygen-san    — full build with ASAN                       → build_address/
#   moxygen-tsan   — full build with TSAN                       → build_thread/
#   core           — base + publisher_tests, no moxygen         → build_core/
#   docker         — Linux-only, simplified CMakeLists           → build/
#   docker-san     — Linux-only + ASAN                           → build_address/
#
# Options:
#   --clean        Clean and rebuild all dependencies from scratch

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INTEROP_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"

# Parse arguments: preset and --clean flag
PRESET="${1:-moxygen}"
CLEAN_FLAG=""
if [[ "${2:-}" == "--clean" ]]; then
  CLEAN_FLAG="--clean"
elif [[ "${PRESET}" == "--clean" ]]; then
  # Allow --clean as first argument
  CLEAN_FLAG="--clean"
  PRESET="moxygen"
fi

SOURCE_DIR="${INTEROP_ROOT}/moq-interop_tests"

# Map preset name to the binary output directory
case "$PRESET" in
  moxygen)     BUILD_DIR="${SOURCE_DIR}/build"         ;;
  moxygen-san) BUILD_DIR="${SOURCE_DIR}/build_address" ;;
  moxygen-tsan)BUILD_DIR="${SOURCE_DIR}/build_thread"  ;;
  core)        BUILD_DIR="${SOURCE_DIR}/build_core"    ;;
  docker)      BUILD_DIR="${SOURCE_DIR}/build"         ;;
  docker-san)  BUILD_DIR="${SOURCE_DIR}/build_address" ;;
  *)
    echo "Unknown preset '${PRESET}'. Valid: moxygen, moxygen-san, moxygen-tsan, core, docker, docker-san" >&2
    exit 1 ;;
esac

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
# Save system cmake before getdeps prepends its own (which may lack share/)
# Export so configure.sh inherits the pre-getdeps cmake path.
export SYSTEM_CMAKE="$(command -v cmake 2>/dev/null || true)"

eval "$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" env moxygen)"

CMAKE_BIN="${SYSTEM_CMAKE:-cmake}"

# ── Main ──────────────────────────────────────────────────────────────────────

NEED_CONFIGURE=false

if [[ -n "$CLEAN_FLAG" ]]; then
  echo "==> --clean requested: rebuilding all dependencies..."
  "${SCRIPT_DIR}/setup-deps.sh" "$CLEAN_FLAG"
  write_rev_stamp
  NEED_CONFIGURE=true
elif ! moxygen_is_installed; then
  echo "==> moxygen not found — running full setup..."
  "${SCRIPT_DIR}/setup-deps.sh"
  write_rev_stamp
  NEED_CONFIGURE=true
elif moxygen_changed; then
  echo "==> moxygen submodule changed — rebuilding moxygen..."
  "${SCRIPT_DIR}/build-moxygen.sh"
  write_rev_stamp
  NEED_CONFIGURE=true
fi

if [[ "$NEED_CONFIGURE" == true ]] || needs_configure; then
  "${SCRIPT_DIR}/configure.sh" "$PRESET"
fi

echo "==> Building moq-interop_tests (preset=${PRESET})..."
NPROC=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
"$CMAKE_BIN" --build "$BUILD_DIR" -- -j"$NPROC"

BINARY="${BUILD_DIR}/bin/interop_tests"
if [[ -f "$BINARY" ]]; then
  echo "==> Build complete: ${BINARY}"
else
  echo "==> Build complete: ${BUILD_DIR}"
fi
