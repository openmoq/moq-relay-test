#!/usr/bin/env bash
# setup-deps.sh — Build moxygen and all its Meta OSS dependencies.
#
# Uses getdeps.py (bundled with the moxygen submodule) to fetch, build, and
# install folly, fizz, wangle, mvfst, proxygen, and moxygen into a
# deterministic scratch directory under the project root. Getdeps handles
# all incrementality natively.
#
# Source patches (see PATCH section below) are applied to clones inside the
# scratch dir which getdeps fetches fresh from upstream — they do NOT touch
# the moxygen git submodule checked out at moqt_implementation/moxygen/moxygen.
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
    --scratch-path)  SCRATCH_PATH="$2"; shift ;;
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

# First attempt: build moxygen.  If it fails because MoQServer.cpp uses the
# old proxygen::QuicWtSession::setHandler API (removed in newer proxygen builds),
# apply a targeted source patch and retry.  The interop_tests binary is
# client-only and does not link moxygen_moq_server, so removing the
# setHandler call is safe for this build configuration.
if ! python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" build --no-tests $CLEAN_FLAG moxygen; then
  MOXYGEN_SRC="$(python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" show-source-dir moxygen 2>/dev/null | tail -1 || true)"
  MOQSERVER="${MOXYGEN_SRC}/moxygen/MoQServer.cpp"
  # Detect any state of the setHandler API-drift bug:
  #   State A: original – setHandler call present (single- or two-line)
  #   State B: previous partial patch – setHandler line deleted but
  #             orphaned "moqSession.get());" remains on its own line
  if [[ -f "$MOQSERVER" ]] && \
     grep -qE 'setHandler|moqSession\.get\(\)\)' "$MOQSERVER"; then
    echo "==> Patching MoQServer.cpp: removing setHandler call (proxygen API drift)"
    python3 - "$MOQSERVER" << 'PATCH_EOF'
import sys, re
path = sys.argv[1]
with open(path) as f:
    content = f.read()

patched = content

# State A (two-line form): remove the whole call spanning two source lines
patched = re.sub(
    r'[ \t]*static_cast<proxygen::QuicWtSession\*>\(wtPtr\)->setHandler\(\n[ \t]*moqSession\.get\(\)\);[ \t]*\n',
    '',
    patched,
)

# State A' (single-line form)
patched = re.sub(
    r'[ \t]*static_cast<proxygen::QuicWtSession\*>\(wtPtr\)->setHandler\(moqSession\.get\(\)\);[ \t]*\n',
    '',
    patched,
)

# State B: setHandler line was already deleted by a previous tool run;
# "moqSession.get());" is now an orphan on its own line (note the extra ')').
patched = re.sub(
    r'^[ \t]*moqSession\.get\(\)\);[ \t]*\n',
    '',
    patched,
    flags=re.MULTILINE,
)

if patched == content:
    print("WARNING: setHandler patch did not match any known pattern — skipping")
else:
    with open(path, 'w') as f:
        f.write(patched)
    print(f"Patched: {path}")
PATCH_EOF
    echo "==> Retrying moxygen build..."
    python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" build --no-tests $CLEAN_FLAG moxygen
  else
    echo "==> moxygen build failed for unknown reason" >&2
    exit 1
  fi
fi

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

patched = src.replace(
    '#include <folly/system/os/linux.h>\n\n#include <linux/openat2.h>\n\n#include <folly/portability/SysSyscall.h>\n\nnamespace folly {\n\nlong linux_syscall_openat2(\n    int const dirfd,\n    char const* const pathname,\n    struct open_how const* const how) {\n  constexpr long no_openat2 =\n#if defined(__linux__) && defined(SYS_openat2)\n      SYS_openat2;\n#else\n      -1;\n#endif\n  return detail::linux_syscall(\n      no_openat2, dirfd, pathname, how, sizeof(struct open_how));\n}\n\n} // namespace folly',
    '#include <folly/system/os/linux.h>\n\n#if defined(__linux__)\n#include <linux/openat2.h>\n#include <folly/portability/SysSyscall.h>\n\nnamespace folly {\n\nlong linux_syscall_openat2(\n    int const dirfd,\n    char const* const pathname,\n    struct open_how const* const how) {\n  constexpr long no_openat2 =\n#if defined(SYS_openat2)\n      SYS_openat2;\n#else\n      -1;\n#endif\n  return detail::linux_syscall(\n      no_openat2, dirfd, pathname, how, sizeof(struct open_how));\n}\n\n} // namespace folly\n\n#else // !defined(__linux__)\n\nnamespace folly {\n\n// openat2 is a Linux-only syscall; this stub satisfies the linker on macOS.\nlong linux_syscall_openat2(\n    int const /*dirfd*/,\n    char const* const /*pathname*/,\n    struct open_how const* const /*how*/) {\n  return -1;\n}\n\n} // namespace folly\n\n#endif // defined(__linux__)',
)

if patched == src:
    print("  WARNING: folly linux.cpp patch did not apply (pattern not found)")
else:
    with open(path, 'w') as f:
        f.write(patched)
    print("  patched:", path)
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
