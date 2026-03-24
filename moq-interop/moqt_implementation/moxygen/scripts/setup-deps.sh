#!/usr/bin/env bash
# setup-deps.sh — Build moxygen and all its Meta OSS dependencies.
#
# Uses getdeps.py (bundled with the moxygen submodule) to fetch, build, and
# install folly, fizz, wangle, mvfst, proxygen, and moxygen into getdeps'
# default temp directory. Getdeps handles all incrementality natively.
#
# Usage:
#   ./scripts/setup-deps.sh [--clean]
#
# Options:
#   --clean    Clean and rebuild all dependencies from scratch

set -euo pipefail

CLEAN_FLAG=""
[[ "${1:-}" == "--clean" ]] && CLEAN_FLAG="--clean"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

MOXYGEN_DIR="${PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"

# ── Sanity checks ─────────────────────────────────────────────────────────────

if [[ ! -f "$GETDEPS" ]]; then
  echo "Error: getdeps.py not found at ${GETDEPS}" >&2
  echo "  Did you init submodules?  git submodule update --init" >&2
  exit 1
fi

# ── Build moxygen + all deps ──────────────────────────────────────────────────

echo "==> Building moxygen + Meta OSS deps (this may take a while on first run)..."
cd "$MOXYGEN_DIR"
python3 "$GETDEPS" build --no-tests $CLEAN_FLAG moxygen

# ── Patch glog: older getdeps installs omit log_severity.h which is required
#    by logging.h on glog >= 0.6. Inject a compatibility shim if missing.
GLOG_INST="$(python3 "$GETDEPS" show-inst-dir glog 2>/dev/null | tail -1)"
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
echo "==> Done. Build moq-interop_tests with:"
echo "    ./scripts/build.sh"
