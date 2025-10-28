# MOQ Interop Test Framework

A simple, minimal conformance test framework for MOQ (Media over QUIC) publish functionality using real moxygen client.

## Overview

This framework performs real MOQ testing by executing the moxygen `moqtextclient` as a subprocess.

## Files

- `main.cpp` - Test runner entry point
- `PublishTest.h/cpp` - Test case implementation
- `MoQClientWrapper.h/cpp` - C interface wrapper for moxygen subprocess
- `CMakeLists.txt` - Build configuration

## Build & Run

```bash
cd build
cmake ..
make
./bin/interop_test_runner
```

## Test Output

The framework outputs:
- Connection status
- Publish operation results
- Real moxygen client logs
- Final test result (PASSED/FAILED)

## Dependencies

- C++20 compiler
- CMake 3.16+
- Built moxygen installation in `../moqt_implementation/moxygen`

## Architecture

The framework uses a C interface wrapper to avoid C++ dependency conflicts with moxygen/folly libraries. It executes the `moqtextclient` binary with appropriate parameters to perform real MOQ operations, providing true interoperability testing without requiring direct library integration.

## Architecture

The framework uses a C interface wrapper to avoid C++ dependency conflicts with moxygen/folly libraries. The subprocess mode executes the `moqtextclient` binary with appropriate parameters to perform real MOQ operations.