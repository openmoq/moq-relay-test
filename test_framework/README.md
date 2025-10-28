# MOQ Server Test Framework

A comprehensive test framework for validating MOQ (Media Over QUIC) servers by sending protocol requests and validating responses. This framework uses the moxygen library to send authentic MOQ PUBLISH requests and verify that servers respond correctly with OK or ERROR messages.

## Overview

This test framework provides:

- **PUBLISH Request Testing**: Send PUBLISH requests with various parameters and validate responses
- **Comprehensive Test Suites**: Pre-built test configurations for common scenarios
- **Flexible Configuration**: Easy-to-use builder pattern for custom test cases
- **Detailed Reporting**: Console output and file logging with detailed results
- **Real MOQ Protocol**: Uses moxygen library for authentic MOQ protocol communication

## Features

### Test Types

1. **Basic PUBLISH Tests**: Simple PUBLISH requests with minimal valid parameters
2. **Track Namespace Tests**: Tests with various namespace patterns (live, vod, chat, etc.)
3. **Group Order Tests**: Tests with different group ordering (Default, OldestFirst, NewestFirst)
4. **Forward Direction Tests**: Tests with forward=true/false
5. **Invalid Parameter Tests**: Tests expecting errors for invalid inputs
6. **Custom Scenario Tests**: Realistic streaming scenarios (live video/audio, VOD, chat)

### Validation Features

- **Request ID Matching**: Ensures response request IDs match the original request
- **Response Type Validation**: Verifies OK vs ERROR responses match expectations
- **Parameter Validation**: Checks response parameters are reasonable
- **Timeout Handling**: Configurable timeouts for each test
- **Success Rate Calculation**: Overall server validation scoring

## Prerequisites

1. **Moxygen Library**: The framework requires moxygen to be built and available
2. **Dependencies**: folly, proxygen, gflags, and other moxygen dependencies
3. **C++20 Compiler**: Modern C++ compiler with coroutine support
4. **CMake 3.16+**: For building the framework

## Building

### 1. Build Moxygen First

```bash
cd moqt_implementation/moxygen
./build.sh
```

### 2. Build the Test Framework

```bash
cd test_framework
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. Verify Build

```bash
./moq_test_runner --help
```

## Usage

### Basic Usage

Test a MOQ server with default comprehensive test suite:

```bash
./moq_test_runner --server_url=https://localhost:4433/moq
```

### Command Line Options

```bash
--server_url=<url>       # MOQ server URL to test (default: https://localhost:4433/moq)
--run_basic_only=<bool>  # Run only basic tests (default: false)
--verbose=<bool>         # Enable verbose logging (default: false)
--timeout_ms=<int>       # Default timeout in milliseconds (default: 5000)
```

### Examples

```bash
# Test with verbose output
./moq_test_runner --server_url=https://moq.example.com/moq --verbose=true

# Run only basic tests with custom timeout
./moq_test_runner --server_url=https://localhost:4433/moq --run_basic_only=true --timeout_ms=10000

# Test production server
./moq_test_runner --server_url=https://relay.moq.example.com/moq
```

## Programmatic Usage

### Simple Example

```cpp
#include "MoQServerTestFramework.h"

int main() {
    folly::EventBase evb;
    proxygen::URL serverUrl("https://localhost:4433/moq");

    auto testFramework = std::make_shared<MoQServerTestFramework>(
        &evb, serverUrl);

    // Initialize and connect
    folly::coro::blockingWait(testFramework->initialize());
    bool connected = folly::coro::blockingWait(testFramework->connectToServer());

    if (connected) {
        // Add basic test
        testFramework->addPublishTest(StandardTestSuites::basicPublishTest());

        // Run tests
        auto results = folly::coro::blockingWait(testFramework->runAllTests());

        // Process results...
    }

    return 0;
}
```

### Custom Test Configuration

```cpp
// Create a custom PUBLISH test
auto customTest = PublishTestBuilder()
    .withTestName("MyCustomTest")
    .withTrackNamespace("live/stream1")
    .withTrackName("video")
    .withTrackAlias(moxygen::TrackAlias{100})
    .withGroupOrder(moxygen::GroupOrder::NewestFirst)
    .withForward(true)
    .withTimeout(std::chrono::milliseconds(3000))
    .withExpectSuccess(true)
    .withDescription("Custom live video test")
    .build();

testFramework->addPublishTest(customTest);
```

### Custom Result Handling

```cpp
class MyResultCallback : public TestResultCallback {
public:
    void onTestComplete(const TestExecutionResult& result) override {
        if (result.result == TestResult::PASS) {
            std::cout << "✅ " << result.testName << " passed!" << std::endl;
        } else {
            std::cout << "❌ " << result.testName << " failed: "
                      << result.message << std::endl;
        }
    }

    void onTestProgress(const std::string& testName, const std::string& message) override {
        std::cout << "[" << testName << "] " << message << std::endl;
    }
};

auto callback = std::make_shared<MyResultCallback>();
testFramework->setTestResultCallback(callback);
```

## Test Configuration

### Standard Test Suites

The framework provides several pre-built test suites:

1. **BasicPublishTest**: Simple PUBLISH with minimal parameters
2. **TrackNamespaceTests**: Tests various namespace patterns
3. **GroupOrderTests**: Tests different group ordering options
4. **ForwardTests**: Tests forward/backward direction
5. **InvalidPublishTests**: Tests expecting error responses
6. **ComprehensiveTestSuite**: Combination of all standard tests

### Test Parameters

Each test can be configured with:

- **Test Name**: Unique identifier for the test
- **Track Namespace**: MOQ track namespace (e.g., "live", "vod", "chat")
- **Track Name**: MOQ track name within the namespace
- **Track Alias**: Numeric alias for the track
- **Group Order**: Ordering preference (Default, OldestFirst, NewestFirst)
- **Forward Direction**: Whether to forward or relay content
- **Timeout**: Maximum time to wait for response
- **Expected Result**: Whether test should succeed or fail
- **Parameters**: Additional MOQ track parameters

## Output and Reporting

### Console Output

The framework provides colored console output:
- 🟢 Green: PASS
- 🔴 Red: FAIL
- 🟡 Yellow: TIMEOUT
- 🟣 Purple: ERROR

### Log File

Detailed results are saved to `moq_test_results.log` including:
- Test configuration
- Request/response details
- Timing information
- Error details
- PUBLISH_OK/PUBLISH_ERROR message contents

### Success Criteria

- **Individual Test**: PASS if server responds as expected (OK for valid requests, ERROR for invalid)
- **Overall Validation**: PASS if ≥80% of tests succeed
- **Exit Code**: 0 for success, 1 for failure

## Architecture

### Key Components

1. **MoQServerTestFramework**: Main framework class that manages tests
2. **PublishTestConfig**: Configuration for PUBLISH request tests
3. **TestExecutionResult**: Results from individual test execution
4. **TestResultCallback**: Interface for handling test results
5. **PublishTestBuilder**: Builder pattern for easy test configuration
6. **StandardTestSuites**: Pre-built common test scenarios

### Protocol Integration

The framework uses moxygen components:
- **MoQClient**: For establishing MOQ connections
- **MoQSession**: For MOQ protocol communication
- **PublishRequest/Response**: Authentic MOQ message structures
- **MoQFramer**: For protocol message encoding/decoding

## Troubleshooting

### Common Issues

1. **Connection Failed**
   - Verify server URL is correct and accessible
   - Check if server supports the MOQ protocol version
   - Ensure server certificates are valid (for HTTPS)

2. **Build Errors**
   - Ensure moxygen is built first: `cd moxygen && ./build.sh`
   - Check all dependencies are installed
   - Verify CMake can find moxygen library

3. **Test Timeouts**
   - Increase timeout with `--timeout_ms=10000`
   - Check network connectivity to server
   - Verify server is responding to MOQ requests

4. **All Tests Failing**
   - Check if server implements PUBLISH request handling
   - Verify server supports the expected MOQ protocol version
   - Review server logs for error details

### Debug Mode

Enable verbose logging for detailed debugging:

```bash
./moq_test_runner --server_url=https://localhost:4433/moq --verbose=true
```

## Extending the Framework

### Adding New Test Types

1. Create new test configuration structure (similar to PublishTestConfig)
2. Add execution method to MoQServerTestFramework
3. Implement validation logic for the new message type
4. Add builder and standard test suites

### Custom Validation

Override validation methods for custom logic:

```cpp
class MyTestFramework : public MoQServerTestFramework {
protected:
    bool validatePublishOk(const moxygen::PublishOk& publishOk,
                          const moxygen::PublishRequest& originalRequest) override {
        // Custom validation logic
        return MoQServerTestFramework::validatePublishOk(publishOk, originalRequest) &&
               myCustomValidation(publishOk);
    }
};
```

## License

This test framework is provided under the same license as the moxygen project (MIT License).

## Contributing

Contributions welcome! Please ensure:
1. Tests pass with your changes
2. Code follows existing style
3. New features include appropriate tests
4. Documentation is updated