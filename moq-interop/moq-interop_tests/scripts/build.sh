#!/usr/bin/env bash
# =============================================================================
# moq-interop_tests build driver
#
# Builds the interop_tests binary. moxygen and its dependencies must already
# be installed via:
#     cd ../moqt_implementation/moxygen && bash scripts/setup-deps.sh
#
# Usage:
#   ./scripts/build.sh [--asan|--tsan|--ubsan] [--clean]
#                      [--build-dir DIR] [--scratch-path PATH]
#
# Environment:
#   MOXYGEN_SCRATCH    Same as --scratch-path; --scratch-path takes priority.
#                      Must match the value used by setup-deps.sh.
#                      Default: <workspace>/moqt_implementation/moxygen/.getdeps-scratch
#
# Examples:
#   ./scripts/build.sh                  # RelWithDebInfo → build/
#   ./scripts/build.sh --asan           # AddressSanitizer → build_asan/
#   ./scripts/build.sh --clean          # wipe build dir first
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
WORKSPACE_ROOT="$(cd "$TESTS_ROOT/.." && pwd)"

MOXYGEN_PROJECT_ROOT="${WORKSPACE_ROOT}/moqt_implementation/moxygen"
MOXYGEN_DIR="${MOXYGEN_PROJECT_ROOT}/moxygen"
GETDEPS="${MOXYGEN_DIR}/build/fbcode_builder/getdeps.py"

# ─── Arg parsing ─────────────────────────────────────────────────────────────
SANITIZE=""
BUILD_DIR=""
CLEAN=0
SCRATCH_PATH="${MOXYGEN_SCRATCH:-${MOXYGEN_PROJECT_ROOT}/.getdeps-scratch}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --asan)          SANITIZE=address ;;
        --tsan)          SANITIZE=thread ;;
        --ubsan)         SANITIZE=undefined ;;
        --clean)         CLEAN=1 ;;
        --build-dir)     BUILD_DIR="$2"; shift ;;
        --scratch-path)  SCRATCH_PATH="$2"; shift ;;
        -h|--help)
            sed -n '2,21p' "$0"
            exit 0 ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1 ;;
    esac
    shift
done

if [[ -z "$BUILD_DIR" ]]; then
    if [[ -n "$SANITIZE" ]]; then
        BUILD_DIR="${TESTS_ROOT}/build_${SANITIZE}"
    else
        BUILD_DIR="${TESTS_ROOT}/build"
    fi
fi

INSTALL_PREFIX="${SCRATCH_PATH}/installed"
GETDEPS_ARGS=(--scratch-path "$SCRATCH_PATH" --install-prefix "$INSTALL_PREFIX")

# ─── Sanity checks ───────────────────────────────────────────────────────────
if [[ ! -f "$GETDEPS" ]]; then
    echo "Error: getdeps.py not found at ${GETDEPS}" >&2
    echo "  Did you initialize submodules? git submodule update --init --recursive" >&2
    exit 1
fi

if ! python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" show-inst-dir moxygen >/dev/null 2>&1; then
    echo "Error: moxygen is not installed at ${INSTALL_PREFIX}." >&2
    echo "  Run: cd ${MOXYGEN_PROJECT_ROOT} && bash scripts/setup-deps.sh" >&2
    echo "  (or set MOXYGEN_SCRATCH to point at an existing install)" >&2
    exit 1
fi

# ─── Activate getdeps environment ────────────────────────────────────────────
# `getdeps env` sets CMAKE_PREFIX_PATH for moxygen's *dependencies* (folly,
# proxygen, mvfst, …) but NOT for moxygen itself, and does not enumerate
# the full transitive closure (e.g. libsodium, which is fizz→sodium).
# Solution: capture the moxygen install prefix, derive the getdeps installed/
# root, and add ALL installed package dirs to CMAKE_PREFIX_PATH so that every
# transitive find_dependency() call in the installed cmake configs can succeed.
# Preserve the host's cmake before getdeps prepends its own copy (which may
# have an incomplete share/Modules).
SYSTEM_CMAKE="$(command -v cmake 2>/dev/null || true)"
MOXYGEN_INST="$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" show-inst-dir moxygen 2>/dev/null | tail -1)"
eval "$(cd "$MOXYGEN_DIR" && python3 "$GETDEPS" "${GETDEPS_ARGS[@]}" env moxygen)"

echo "==> Scratch path:   ${SCRATCH_PATH}"
echo "==> moxygen install: ${MOXYGEN_INST}"

# MOXYGEN_INST is …/installed/moxygen; strip the last component to get the
# parent installed/ directory, then glob every sub-directory into PREFIX_PATH.
GETDEPS_INSTALLED="${MOXYGEN_INST%/*}"
if [[ -d "$GETDEPS_INSTALLED" ]]; then
    EXTRA=""
    LIBPATH=""
    for d in "$GETDEPS_INSTALLED"/*/; do
        d="${d%/}"
        [[ -d "$d" ]] && EXTRA="${d}${EXTRA:+:${EXTRA}}"
        # Collect lib dirs for the linker (handles packages like libsodium that
        # only install a static archive with no cmake config)
        for ld in "$d/lib" "$d/lib64"; do
            [[ -d "$ld" ]] && LIBPATH="${ld}${LIBPATH:+:${LIBPATH}}"
        done
    done
    export CMAKE_PREFIX_PATH="${EXTRA}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
    # LIBRARY_PATH is searched by gcc/g++ for -lsodium style link flags coming
    # from installed cmake targets that didn't use an absolute path.
    export LIBRARY_PATH="${LIBPATH}${LIBRARY_PATH:+:${LIBRARY_PATH}}"
fi

CMAKE_BIN="${SYSTEM_CMAKE:-cmake}"

# ─── Configure & build ───────────────────────────────────────────────────────
if [[ "$CLEAN" -eq 1 ]]; then
    echo "==> --clean: removing ${BUILD_DIR}"
    rm -rf "$BUILD_DIR"
fi

CMAKE_ARGS=(-S "$TESTS_ROOT" -B "$BUILD_DIR" -G Ninja)
if [[ -n "$SANITIZE" ]]; then
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Debug -DSANITIZE="$SANITIZE")
else
    CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=RelWithDebInfo)
fi
# Append -lz at the very end of the link command so that any static archive
# (e.g. getdeps libunwind.a) that references uncompress() can resolve it even
# after libz appears earlier in the link order.
CMAKE_ARGS+=(-DCMAKE_EXE_LINKER_FLAGS="-lz")

echo "==> Configuring (build dir: ${BUILD_DIR})"
"$CMAKE_BIN" "${CMAKE_ARGS[@]}"

NPROC="$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"
echo "==> Building with ${NPROC} jobs"
"$CMAKE_BIN" --build "$BUILD_DIR" -- -j"$NPROC"

BINARY="${BUILD_DIR}/bin/interop_tests"
if [[ -x "$BINARY" ]]; then
    echo "==> Build complete: ${BINARY}"
else
    echo "==> Build finished but binary not found at ${BINARY}" >&2
    exit 1
fi
