# Interop Tests

This directory contains the MoQ interoperability test suite for testing MoQ relay implementations.

## Building

To build the tests, you need to use the provided build script that sets up the Moxygen environment:

```bash
./build_with_moxygen_env.sh
```

This script will:
1. Set up the necessary Moxygen environment variables
2. Run CMake configuration
3. Build the project

The compiled binary will be located at `build/bin/interop_tests`.

Alternatively, if the Moxygen environment is already configured, you can build manually:

```bash
cd build
cmake ..
make
```

## Usage

```bash
./build/bin/interop_tests [OPTIONS]
```

## CLI Parameters

### `--relay` (string)
Specifies the MoQ relay server URL to connect to for testing.

**Default:** `https://localhost:4433/moq`

**Example:**
```bash
./build/bin/interop_tests --relay=https://relay.example.com:4433/moq
```

### `--tests` (string)
Comma-separated list of specific test names to run. If empty, all tests will be executed.

**Default:** (empty - runs all tests)

**Example:**
```bash
# Run a single test
./build/bin/interop_tests --tests=publish_test

# Run multiple specific tests
./build/bin/interop_tests --tests=publish_test,subscribe_test,goaway_test
```

### `--categories` (string)
Comma-separated list of test categories to filter which tests should run. This allows you to run only tests from specific categories.

**Default:** (empty - runs all categories)

**Available Categories:**
- `Publisher` - Tests related to publishing data
- `Subscriber` - Tests related to subscribing to data
- `Namespace` - Tests related to namespace operations
- `ErrorHandling` - Tests related to error scenarios
- `Update` - Tests related to update operations
- `Connection` - Tests related to connection management
- `All` - Run all tests (default behavior)

**Example:**
```bash
# Run only publisher tests
./build/bin/interop_tests --categories=Publisher

# Run publisher and subscriber tests
./build/bin/interop_tests --categories=Publisher,Subscriber

# Run error handling tests
./build/bin/interop_tests --categories=ErrorHandling
```

**Note:** Category names are case-insensitive, and `error_handling` can be used as an alternative to `ErrorHandling`.

### `--list` (boolean flag)
Lists all available tests without running them. Useful for discovering test names before running specific tests.

**Default:** `false`

**Example:**
```bash
./build/bin/interop_tests --list
```

## Common Usage Examples

### Run all tests against local relay
```bash
./build/bin/interop_tests
```

### Run all tests against a remote relay
```bash
./build/bin/interop_tests --relay=https://relay.example.com:4433/moq
```

### List all available tests
```bash
./build/bin/interop_tests --list
```

### Run only publisher-related tests
```bash
./build/bin/interop_tests --categories=Publisher
```

### Run specific tests by name
```bash
./build/bin/interop_tests --tests=publish_test,subscribe_test
```

### Run subscriber and error handling tests on a specific relay
```bash
./build/bin/interop_tests --relay=https://relay.example.com:4433/moq --categories=Subscriber,ErrorHandling
```

### Combine filters: specific tests from a category
```bash
# First list tests to see what's available
./build/bin/interop_tests --list --categories=Publisher

# Then run specific tests from that category
./build/bin/interop_tests --tests=publish_test,publish_namespace_test
```

## Exit Codes

- `0` - All tests passed successfully
- `1` - One or more tests failed

## Test Structure

Tests are organized into the following directories:
- `publisher_tests/` - Publisher-related tests
- `subscriber_tests/` - Subscriber-related tests
- `base/` - Base test framework and utilities
- `moxygen_adapter/` - Adapter layer for the Moxygen implementation

## Additional Scripts

- `build_with_moxygen_env.sh` - Build script with Moxygen environment setup
