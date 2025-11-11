#include "PublishTest.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <folly/logging/Init.h>
#include <folly/logging/LoggerDB.h>
#include <folly/io/async/EventBaseThread.h>

using namespace interop_test;

int main (int argc, char* argv[]) {
    folly::Init init(&argc, &argv);

    // Create EventBaseThread like debug_connection.cpp does
    auto eventBaseThread = std::make_unique<folly::EventBaseThread>();

    // Create PublishTest
    auto test = std::make_shared<PublishTest>(eventBaseThread->getEventBase());

    PublishTestConfig config;
    config.trackNamespace = "test";
    config.trackName = "interop-track";
    // Try with proper WebTransport URL format
    config.serverUrl = "https://localhost:4433/moq";

    std::cout << "Running Interop Publish Test...\n";

    TestResult result = TestResult::ERROR;

    try {
        // Run the test using blockingWait like debug_connection.cpp
        result = test->runTest(config);
    } catch (const std::exception& ex) {
        std::cout << "Exception in test: " << ex.what() << std::endl;
        result = TestResult::ERROR;
    }

    switch (result) {
        case TestResult::PASS:
            std::cout << "Interop Publish Test PASSED\n";
            break;
        case TestResult::FAIL:
            std::cout << "Interop Publish Test FAILED: " << test->getLastError() << "\n";
            break;
        case TestResult::TIMEOUT:
            std::cout << "Interop Publish Test TIMED OUT\n";
            break;
        case TestResult::ERROR:
            std::cout << "Interop Publish Test ERROR: " << test->getLastError() << "\n";
            break;
    }

    return 0;
}