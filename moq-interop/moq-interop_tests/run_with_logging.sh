#!/bin/bash

# Script to run the interop test framework with detailed logging

# Set Folly logging configuration
export FOLLY_LOG_LEVEL=DBG

# Set specific logger levels
export GLOG_v=1
export GLOG_logtostderr=1

# ---------- Heap diagnostics ------------------------------------------------
# --asan   : run the ASAN binary (build_address/ must exist, built with
#            ./build_with_moxygen_env.sh --sanitize=address)
# --malloc : enable macOS libMalloc guard pages (no recompile needed)
# (both flags can be combined)
BINARY="./build/bin/interop_tests"
for arg in "$@"; do
    if [[ "$arg" == "--asan" ]]; then
        BINARY="./build_address/bin/interop_tests"
        # ASAN on macOS: suppress false positives from system libraries
        export ASAN_OPTIONS="detect_leaks=0:abort_on_error=1:print_stats=1"
        echo "[run_with_logging] Using ASAN binary: $BINARY"
    fi
    if [[ "$arg" == "--malloc" ]]; then
        # macOS malloc: guard edges, scribble freed memory, check heap
        # Each alloc/free will verify neighbour chunk metadata.
        export MallocGuardEdges=1
        export MallocScribble=1
        export MallocPreScribble=1
        export MallocCheckHeapEach=100
        export MallocCheckHeapSleep=0
        echo "[run_with_logging] macOS malloc guards enabled"
    fi
done
# Strip our own flags before passing the rest to the binary
FORWARD_ARGS=()
for arg in "$@"; do
    [[ "$arg" == "--asan" || "$arg" == "--malloc" ]] && continue
    FORWARD_ARGS+=("$arg")
done
# ---------------------------------------------------------------------------

# Run the test
echo "Running interop test with detailed logging..."
echo "Log level: DBG1"
echo "----------------------------------------"

$BINARY "${FORWARD_ARGS[@]}"