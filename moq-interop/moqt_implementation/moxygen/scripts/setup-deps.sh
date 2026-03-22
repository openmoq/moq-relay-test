#!/usr/bin/env bash
# setup-deps.sh — Build moxygen and all its Meta OSS dependencies.
#
# Uses getdeps.py (bundled with the moxygen submodule) to fetch, build, and
# install folly, fizz, wangle, mvfst, proxygen, and moxygen into .scratch/.
# CMakeLists.txt queries this same location at configure time via
# getdeps.py show-inst-dir, so no extra path files are needed.
#
# Usage:
#   ./scripts/setup-deps.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INTEROP_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"
STAMP_DIR="${INTEROP_ROOT}/.scratch"

# ── Sanity checks ─────────────────────────────────────────────────────────────

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: getdeps.py not found at ${GETDEPS}" >&2
  echo "  Did you init submodules?  git submodule update --init" >&2
  exit 1
fi

# ── Build moxygen + all deps ──────────────────────────────────────────────────

echo "==> Building moxygen + Meta OSS deps (this may take a while on first run)..."
mkdir -p "$STAMP_DIR"
cd "$MOXYGEN_DIR"
python3 "$GETDEPS" build --no-tests moxygen

# ── Stamp current moxygen revision ───────────────────────────────────────────

git -C "$MOXYGEN_DIR" rev-parse HEAD > "${STAMP_DIR}/moxygen.rev"

echo ""
echo "==> Done. Build moq-interop_tests with:"
echo "    ./scripts/build.sh"
