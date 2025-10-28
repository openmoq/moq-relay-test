#!/bin/bash

# Build script that uses moxygen environment
cd /Users/akashn/Projects/openmoq/moq-relay-test/moqt_implementation/moxygen

# Set up the moxygen environment
eval "$(./build/fbcode_builder/getdeps.py env moxygen)"

# Now build the interop test framework
cd /Users/akashn/Projects/openmoq/moq-relay-test/test_framework/build

# Build with the current CMakeLists.txt using moxygen environment
cmake ..
make