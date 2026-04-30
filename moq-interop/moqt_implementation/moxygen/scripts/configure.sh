#!/usr/bin/env bash
# configure.sh — CMake configure for moq-interop_tests.
#
# Translates a CMakePresets.json preset name into explicit cmake arguments so
# no dependency on cmake --preset support (requires cmake 3.21+) is needed.
#
# Usage:
#   ./scripts/configure.sh [preset]
#
# Presets (matching CMakePresets.json):
#   moxygen        — full build with moxygen_adapter  (default)  → build/
#   moxygen-san    — full build with ASAN                         → build_address/
#   moxygen-tsan   — full build with TSAN                         → build_thread/
#   core           — base + publisher_tests, no moxygen adapter   → build_core/

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INTEROP_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"
SOURCE_DIR="${INTEROP_ROOT}/moq-interop_tests"

PRESET="${1:-moxygen}"

# ── Set up cmake / ninja from the getdeps environment ────────────────────────
# If SYSTEM_CMAKE was already exported by a parent script (e.g. build.sh),
# keep using it.  Otherwise capture it before the eval overwrites PATH.
if [[ -z "${SYSTEM_CMAKE:-}" ]]; then
  export SYSTEM_CMAKE="$(command -v cmake 2>/dev/null || true)"
fi

eval "$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" env moxygen)"

# Use the pre-eval cmake so its share/Modules directory is intact.
CMAKE_BIN="${SYSTEM_CMAKE:-cmake}"

# ── Translate preset → cmake args ────────────────────────────────────────────

# cmake source dir: usually equals SOURCE_DIR; docker presets use a subdirectory.
CMAKE_SOURCE_DIR="${SOURCE_DIR}"

case "$PRESET" in
  moxygen)
    BUILD_DIR="${SOURCE_DIR}/build"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_MOXYGEN_ADAPTER=ON)
    ;;
  moxygen-san)
    BUILD_DIR="${SOURCE_DIR}/build_address"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=Debug -DWITH_MOXYGEN_ADAPTER=ON -DSANITIZE=address)
    ;;
  moxygen-tsan)
    BUILD_DIR="${SOURCE_DIR}/build_thread"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=Debug -DWITH_MOXYGEN_ADAPTER=ON -DSANITIZE=thread)
    ;;
  core)
    BUILD_DIR="${SOURCE_DIR}/build_core"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_MOXYGEN_ADAPTER=OFF)
    ;;
  # ── Docker-specific presets — use moq-interop_tests/docker/CMakeLists.txt ──
  # Simpler, Linux-only CMake; no macOS conditionals, no platform guards.
  docker)
    BUILD_DIR="${SOURCE_DIR}/build"
    CMAKE_SOURCE_DIR="${SOURCE_DIR}/docker"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=RelWithDebInfo)
    ;;
  docker-san)
    BUILD_DIR="${SOURCE_DIR}/build_address"
    CMAKE_SOURCE_DIR="${SOURCE_DIR}/docker"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=Debug -DSANITIZE=address)
    ;;
  *)
    echo "Unknown preset '${PRESET}'. Valid: moxygen, moxygen-san, moxygen-tsan, core, docker, docker-san" >&2
    exit 1 ;;
esac

# ── Sanity check ──────────────────────────────────────────────────────────────

if [[ ! -f "${CMAKE_SOURCE_DIR}/CMakeLists.txt" ]]; then
  echo "Error: CMakeLists.txt not found at ${CMAKE_SOURCE_DIR}" >&2
  exit 1
fi

# ── Configure ─────────────────────────────────────────────────────────────────

echo "==> Configuring moq-interop_tests (preset: ${PRESET})"
echo "    source : ${CMAKE_SOURCE_DIR}"
echo "    build  : ${BUILD_DIR}"
"$CMAKE_BIN" -S "$CMAKE_SOURCE_DIR" -B "$BUILD_DIR" -G Ninja "${CMAKE_EXTRA[@]}"
