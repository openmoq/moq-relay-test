#!/bin/bash

# Build script for MOQ Server Test Framework

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== MOQ Server Test Framework Build Script ==="
echo "Script directory: $SCRIPT_DIR"
echo "Build directory: $BUILD_DIR"
echo

# Check if moxygen is built
MOXYGEN_DIR="$SCRIPT_DIR/../moqt_implementation/moxygen"
MOXYGEN_LIB="$MOXYGEN_DIR/build/moxygen/libmoxygen.a"

if [ ! -f "$MOXYGEN_LIB" ]; then
    echo "Moxygen library not found at: $MOXYGEN_LIB"
    echo "Building moxygen first..."

    if [ ! -d "$MOXYGEN_DIR" ]; then
        echo "Moxygen directory not found. Please run copy_moxygen.sh first."
        exit 1
    fi

    cd "$MOXYGEN_DIR"
    if [ -f "build.sh" ]; then
        echo "Running moxygen build script..."
        ./build.sh
    else
        echo "Creating moxygen build directory and building..."
        mkdir -p build
        cd build
        cmake ..
        make -j$(nproc)
    fi

    if [ ! -f "$MOXYGEN_LIB" ]; then
        echo "Failed to build moxygen library"
        exit 1
    fi

    echo "Moxygen built successfully"
    cd "$SCRIPT_DIR"
fi

echo "Moxygen library found at: $MOXYGEN_LIB"

# Create build directory
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

echo "Building test framework..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo
    echo "Build successful!"
    echo
    echo "Built artifacts:"
    echo "  - libmoq_test_framework.a"
    echo "  - moq_test_runner"
    echo
    echo "To test the framework:"
    echo "  cd $BUILD_DIR"
    echo "  ./moq_test_runner --help"
    echo
    echo "Example usage:"
    echo "  ./moq_test_runner --server_url=https://localhost:4433/moq --verbose=true"
else
    echo "Build failed"
    exit 1
fi