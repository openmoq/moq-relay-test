#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Build script that uses moxygen environment
cd "$PROJECT_ROOT/moqt_implementation/moxygen/moxygen"

# Build moxygen first if not already built
echo "Building moxygen..."
./build/fbcode_builder/getdeps.py build moxygen

# Set up the moxygen environment
eval "$(./build/fbcode_builder/getdeps.py env moxygen)"

# Parse optional --sanitize=<type> flag (e.g. --sanitize=address)
SANITIZE_FLAG=""
BUILD_DIR="build"
for arg in "$@"; do
    if [[ "$arg" == --sanitize=* ]]; then
        SANITIZE_TYPE="${arg#--sanitize=}"
        SANITIZE_FLAG="-DSANITIZE=${SANITIZE_TYPE}"
        BUILD_DIR="build_${SANITIZE_TYPE}"
        echo "AddressSanitizer enabled: ${SANITIZE_TYPE}, output dir: $BUILD_DIR"
    fi
done

# Now build the interop test framework
mkdir -p "$PROJECT_ROOT/interop_tests/${BUILD_DIR}"
cd "$PROJECT_ROOT/interop_tests/${BUILD_DIR}"

# Build with the current CMakeLists.txt using moxygen environment
cmake .. ${SANITIZE_FLAG}
make -j$(nproc 2>/dev/null || sysctl -n hw.logicalcpu)