#!/usr/bin/env bash
# setup-deps.sh — Build moxygen and all its Meta OSS dependencies.
#
# Uses getdeps.py (bundled with the moxygen submodule) to fetch, build, and
# install folly, fizz, wangle, mvfst, proxygen, and moxygen into a
# deterministic scratch directory under the project root. Getdeps handles
# all incrementality natively.
#
# moxygen is built from the pinned submodule (synced into getdeps' clone); the
# small folly/glog patches below touch only the getdeps scratch dir.
#
# Usage:
#   ./scripts/setup-deps.sh [--clean] [--scratch-path PATH]
#
# Options:
#   --clean              Clean and rebuild all dependencies from scratch
#   --scratch-path PATH  Override the scratch directory (default: see below)
#
# Environment:
#   MOXYGEN_SCRATCH      Same as --scratch-path; --scratch-path takes priority
#
# Scratch path resolution order:
#   1. --scratch-path PATH
#   2. $MOXYGEN_SCRATCH
#   3. <PROJECT_ROOT>/.getdeps-scratch     (default — deterministic)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"

CLEAN_FLAG=""
SCRATCH_PATH="${MOXYGEN_SCRATCH:-${PROJECT_ROOT}/.getdeps-scratch}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)         CLEAN_FLAG="--clean" ;;
    --scratch-path)
      if [[ $# -lt 2 ]]; then
        echo "Error: --scratch-path requires an argument" >&2
        sed -n '2,27p' "$0"
        exit 1
      fi
      SCRATCH_PATH="$2"
      shift
      ;;
    -h|--help)       sed -n '2,27p' "$0"; exit 0 ;;
    *)               echo "Unknown argument: $1" >&2; exit 1 ;;
  esac
  shift
done

mkdir -p "$SCRATCH_PATH"
INSTALL_PREFIX="${SCRATCH_PATH}/installed"

# Common getdeps args used everywhere — pin scratch + install prefix so output
# locations are deterministic and not derived from a hash of the source path.
GETDEPS_ARGS=(--scratch-path "$SCRATCH_PATH" --install-prefix "$INSTALL_PREFIX")

# ── Sanity checks ─────────────────────────────────────────────────────────────

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: getdeps.py not found at ${GETDEPS}" >&2
  echo "  Did you init submodules?  git submodule update --init" >&2
  exit 1
fi

echo "==> Scratch path: ${SCRATCH_PATH}"
echo "==> Install prefix: ${INSTALL_PREFIX}"

# ── Build moxygen + all deps ──────────────────────────────────────────────────

echo "==> Building moxygen + Meta OSS deps (this may take a while on first run)..."
cd "$MOXYGEN_DIR"

# getdeps builds moxygen from its own upstream clone, not from the submodule.
# Sync the pinned submodule into that clone; rsync -c copies only changed files
# (fresh mtimes), so unchanged sources are not rebuilt.
python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" fetch --no-tests moxygen
MOXYGEN_CLONE="$(python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" show-source-dir moxygen | tail -1)"
rsync -rlc --exclude=/.git --exclude=/build "$MOXYGEN_DIR/" "$MOXYGEN_CLONE/"

python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" build --no-tests $CLEAN_FLAG moxygen

# Sanity check: the install tree should now exist at the deterministic prefix.
MOXYGEN_INST="$(python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" show-inst-dir moxygen 2>/dev/null | tail -1 || true)"
if [[ -z "$MOXYGEN_INST" || ! -d "$MOXYGEN_INST" ]]; then
  echo "Error: moxygen install dir missing after build (${MOXYGEN_INST})" >&2
  exit 1
fi

# ── Patch glog: older getdeps installs omit log_severity.h which is required
#    by logging.h on glog >= 0.6. Inject a compatibility shim if missing.
# ── Patch folly: linux.cpp includes <linux/openat2.h> unconditionally but
#    that header is Linux-only.  Add a platform guard so macOS builds succeed.
#    This patches in the source repo (which --clean does not wipe) so it is
#    applied once and survives incremental rebuilds.
FOLLY_LINUX_CPP="$(python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" show-source-dir folly 2>/dev/null | tail -1)/folly/system/os/linux.cpp"
if [[ -f "$FOLLY_LINUX_CPP" ]] && grep -q '#include <linux/openat2.h>' "$FOLLY_LINUX_CPP"; then
  echo "==> Patching folly: guarding linux/openat2.h include in folly/system/os/linux.cpp"
  # Use Python for a portable in-place sed equivalent.
  python3 - "$FOLLY_LINUX_CPP" << 'PATCH_EOF'
import sys, re

path = sys.argv[1]
with open(path) as f:
    src = f.read()

# Try specific pattern first
patched = re.sub(
    r'(#include <folly/system/os/linux\.h>)\n\n(#include <linux/openat2\.h>)',
    r'\1\n\n#if defined(__linux__)\n\2',
    src,
    count=1
)

# If specific pattern didn't work, try more flexible approach
if patched == src:
    print("  INFO: specific pattern didn't match, trying flexible replacement...")
    # Just wrap the single include line more flexibly
    patched = re.sub(
        r'(#include <linux/openat2\.h>)',
        r'#if defined(__linux__)\n\1',
        src,
        count=1
    )
    # Add the closing endif at the end of the file or before the next major section
    patched = re.sub(
        r'(#include <linux/openat2\.h>\n)',
        r'\1#endif // defined(__linux__)\n',
        patched,
        count=1
    )

if patched != src:
    with open(path, 'w') as f:
        f.write(patched)
    print("  patched:", path)
else:
    print("  WARNING: folly linux.cpp patch pattern not found (may already be patched or version mismatch)")
PATCH_EOF
fi

GLOG_INST="$(python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" show-inst-dir glog 2>/dev/null | tail -1)"
SEVERITY_H="${GLOG_INST}/include/glog/log_severity.h"
if [[ -n "$GLOG_INST" && ! -f "$SEVERITY_H" ]]; then
  echo "==> Patching glog: generating missing log_severity.h"
  mkdir -p "$(dirname "$SEVERITY_H")"
  cat > "$SEVERITY_H" << 'GLOG_EOF'
// Compatibility shim: log_severity.h was split out in glog 0.6.
// This stub satisfies the #include in glog/logging.h on older installs.
#pragma once
namespace google {
typedef int LogSeverity;
const int GLOG_INFO = 0, GLOG_WARNING = 1, GLOG_ERROR = 2, GLOG_FATAL = 3;
const int INFO = GLOG_INFO, WARNING = GLOG_WARNING,
          ERROR = GLOG_ERROR, FATAL = GLOG_FATAL;
const int NUM_SEVERITIES = 4;
} // namespace google
GLOG_EOF
  echo "==> log_severity.h created at ${SEVERITY_H}"
fi

echo ""
echo "==> Done."
echo "    Scratch dir:     ${SCRATCH_PATH}"
echo "    Install prefix:  ${INSTALL_PREFIX}"
echo "    moxygen install: ${MOXYGEN_INST}"
echo ""
echo "    Build moq-interop_tests with:"
echo "        cd ../../moq-interop_tests && \\"
echo "        MOXYGEN_SCRATCH=\"${SCRATCH_PATH}\" bash scripts/build.sh"
