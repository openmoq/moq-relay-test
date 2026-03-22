#!/usr/bin/env bash
# configure.sh — CMake configure for moq-interop_tests.
#
# CMakeLists.txt queries moxygen's install dir directly via getdeps.py at
# configure time, so no prefix-path overrides are needed here.
#
# Usage:
#   ./scripts/configure.sh [build_dir] [sanitize_type]
#
# Examples:
#   ./scripts/configure.sh                                    # default build/
#   ./scripts/configure.sh build_address address             # ASAN
#   ./scripts/configure.sh build_thread   thread             # TSAN
#   ./scripts/configure.sh build_undefined undefined         # UBSAN

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INTEROP_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"
SOURCE_DIR="${INTEROP_ROOT}/moq-interop_tests"
BUILD_DIR="${1:-${SOURCE_DIR}/build}"
SANITIZE="${2:-}"

# ── Set up cmake / ninja from the getdeps environment ────────────────────────

eval "$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" env moxygen)"

# ── Sanity checks ─────────────────────────────────────────────────────────────

if [[ ! -f "${SOURCE_DIR}/CMakeLists.txt" ]]; then
  echo "Error: CMakeLists.txt not found at ${SOURCE_DIR}" >&2
  exit 1
fi

# ── Assemble cmake arguments ──────────────────────────────────────────────────

CMAKE_ARGS=(
  -S "$SOURCE_DIR"
  -B "$BUILD_DIR"
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
  -G Ninja
)

if [[ -n "$SANITIZE" ]]; then
  echo "==> Sanitizer: ${SANITIZE}"
  CMAKE_ARGS+=("-DSANITIZE=${SANITIZE}")
fi

# ── Configure ─────────────────────────────────────────────────────────────────

echo "==> Configuring moq-interop_tests"
echo "    source : ${SOURCE_DIR}"
echo "    build  : ${BUILD_DIR}"
cmake "${CMAKE_ARGS[@]}"
