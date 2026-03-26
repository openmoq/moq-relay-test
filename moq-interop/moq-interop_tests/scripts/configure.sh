#!/usr/bin/env bash
# configure.sh — CMake configure for moq-interop_tests.
#
# Translates a preset name into explicit cmake arguments so
# no dependency on cmake --preset support is required.
#
# Usage:
#   ./scripts/configure.sh [preset]
#
# Presets:
#   moxygen        — full build with moxygen_adapter  (default)  → build/
#   moxygen-san    — full build with ASAN                         → build_address/
#   moxygen-tsan   — full build with TSAN                         → build_thread/
#   core           — base + publisher_tests, no moxygen adapter   → build_core/
#   docker         — Linux-only CMakeLists in docker/             → build/
#   docker-san     — Linux-only + ASAN CMakeLists in docker/      → build_address/

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
WORKSPACE_ROOT="$(cd "$TESTS_ROOT/.." && pwd)"

MOXYGEN_DIR="${WORKSPACE_ROOT}/moqt_implementation/moxygen/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"

PRESET="${1:-moxygen}"

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: getdeps.py not found at ${GETDEPS}" >&2
  echo "  Did you init submodules?  git submodule update --init --recursive" >&2
  exit 1
fi

# ── Set up cmake / ninja from the getdeps environment ────────────────────────
# If SYSTEM_CMAKE was already exported by a parent script (e.g. build.sh),
# keep using it. Otherwise capture it before eval updates PATH.
if [[ -z "${SYSTEM_CMAKE:-}" ]]; then
  export SYSTEM_CMAKE="$(command -v cmake 2>/dev/null || true)"
fi

eval "$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" env moxygen)"

# Use the pre-eval cmake so its share/Modules directory is intact.
CMAKE_BIN="${SYSTEM_CMAKE:-cmake}"

# ── Translate preset → cmake args ────────────────────────────────────────────
CMAKE_SOURCE_DIR="${TESTS_ROOT}"

case "$PRESET" in
  moxygen)
    BUILD_DIR="${TESTS_ROOT}/build"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_MOXYGEN_ADAPTER=ON)
    ;;
  moxygen-san)
    BUILD_DIR="${TESTS_ROOT}/build_address"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=Debug -DWITH_MOXYGEN_ADAPTER=ON -DSANITIZE=address)
    ;;
  moxygen-tsan)
    BUILD_DIR="${TESTS_ROOT}/build_thread"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=Debug -DWITH_MOXYGEN_ADAPTER=ON -DSANITIZE=thread)
    ;;
  core)
    BUILD_DIR="${TESTS_ROOT}/build_core"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_MOXYGEN_ADAPTER=OFF)
    ;;
  docker)
    BUILD_DIR="${TESTS_ROOT}/build"
    CMAKE_SOURCE_DIR="${TESTS_ROOT}/docker"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=RelWithDebInfo)
    ;;
  docker-san)
    BUILD_DIR="${TESTS_ROOT}/build_address"
    CMAKE_SOURCE_DIR="${TESTS_ROOT}/docker"
    CMAKE_EXTRA=(-DCMAKE_BUILD_TYPE=Debug -DSANITIZE=address)
    ;;
  *)
    echo "Unknown preset '${PRESET}'. Valid: moxygen, moxygen-san, moxygen-tsan, core, docker, docker-san" >&2
    exit 1 ;;
esac

if [[ ! -f "${CMAKE_SOURCE_DIR}/CMakeLists.txt" ]]; then
  echo "Error: CMakeLists.txt not found at ${CMAKE_SOURCE_DIR}" >&2
  exit 1
fi

echo "==> Configuring moq-interop_tests (preset: ${PRESET})"
echo "    source : ${CMAKE_SOURCE_DIR}"
echo "    build  : ${BUILD_DIR}"
"$CMAKE_BIN" -S "$CMAKE_SOURCE_DIR" -B "$BUILD_DIR" -G Ninja "${CMAKE_EXTRA[@]}"
