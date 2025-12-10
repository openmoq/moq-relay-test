#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Build script that uses moxygen environment
cd "$PROJECT_ROOT/moqt_implementation/moxygen"

# Set up the moxygen environment
eval "$(./build/fbcode_builder/getdeps.py env moxygen)"

# Now build the interop test framework
mkdir -p "$PROJECT_ROOT/interop_test_framework/build"
cd "$PROJECT_ROOT/interop_test_framework/build"

# Build with the current CMakeLists.txt using moxygen environment
cmake ..
make