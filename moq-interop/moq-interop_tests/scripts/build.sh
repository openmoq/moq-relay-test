#!/usr/bin/env bash
# build.sh — Build driver for moq-interop_tests.
#
# Builds the interop test binary only. moxygen and its dependencies must
# already be installed via moqt_implementation/moxygen/scripts/build.sh
# or setup-deps.sh.
#
# Usage:
#   ./scripts/build.sh [preset] [--clean]
#
# Presets:
#   moxygen        — full build with moxygen_adapter  (default) → build/
#   moxygen-san    — full build with ASAN                       → build_address/
#   moxygen-tsan   — full build with TSAN                       → build_thread/
#   core           — base + publisher_tests, no moxygen         → build_core/
#   docker         — Linux-only, simplified CMakeLists           → build/
#   docker-san     — Linux-only + ASAN                           → build_address/

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
WORKSPACE_ROOT="$(cd "$TESTS_ROOT/.." && pwd)"

MOXYGEN_DIR="${WORKSPACE_ROOT}/moqt_implementation/moxygen/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"

PRESET="moxygen"
CLEAN_FLAG=""

for arg in "$@"; do
  case "$arg" in
    --clean)
      CLEAN_FLAG="--clean"
      ;;
    moxygen|moxygen-san|moxygen-tsan|core|docker|docker-san)
      PRESET="$arg"
      ;;
    *)
      echo "Unknown argument: ${arg}" >&2
      echo "Usage: ./scripts/build.sh [preset] [--clean]" >&2
      exit 1
      ;;
  esac
done

case "$PRESET" in
  moxygen|docker)      BUILD_DIR="${TESTS_ROOT}/build" ;;
  moxygen-san|docker-san) BUILD_DIR="${TESTS_ROOT}/build_address" ;;
  moxygen-tsan)        BUILD_DIR="${TESTS_ROOT}/build_thread" ;;
  core)                BUILD_DIR="${TESTS_ROOT}/build_core" ;;
  *)
    echo "Unknown preset '${PRESET}'." >&2
    exit 1
    ;;
esac

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: getdeps.py not found at ${GETDEPS}" >&2
  echo "  Did you init submodules?  git submodule update --init --recursive" >&2
  exit 1
fi

required_dep="moxygen"
if [[ "$PRESET" == "core" ]]; then
  required_dep="folly"
fi

if ! python3 "$GETDEPS" show-inst-dir "$required_dep" >/dev/null 2>&1; then
  echo "Error: ${required_dep} is not installed in getdeps yet." >&2
  echo "  Run: cd ${WORKSPACE_ROOT}/moqt_implementation/moxygen && bash scripts/build.sh" >&2
  exit 1
fi

# Preserve system cmake before getdeps env prepends paths.
export SYSTEM_CMAKE="$(command -v cmake 2>/dev/null || true)"
eval "$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" env moxygen)"
CMAKE_BIN="${SYSTEM_CMAKE:-cmake}"

if [[ -n "$CLEAN_FLAG" ]]; then
  echo "==> --clean requested: removing ${BUILD_DIR}"
  rm -rf "$BUILD_DIR"
fi

if [[ ! -f "${BUILD_DIR}/build.ninja" ]]; then
  bash "${SCRIPT_DIR}/configure.sh" "$PRESET"
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
