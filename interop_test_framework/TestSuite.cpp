#include "TestSuite.h"
#include <iostream>

namespace interop_test {

TestSuite::TestSuite(folly::EventBase* eventBase)
    : eventBase_(eventBase) {
}

bool TestSuite::runAll(const TestSuiteConfig& config) {
    std::cout << "=== Running MoQ Interop Test Suite ===\n";
    std::cout << "Relay URL: " << config.relayUrl << "\n\n";

    // Run all tests
    runPublishTest(config);

    // Print summary
    printSummary();

    return failedTests_ == 0;
}

bool TestSuite::runPublishTest(const TestSuiteConfig& config) {
    std::cout << "Running Publish Test...\n";

    auto test = std::make_shared<PublishTest>(eventBase_);

    PublishTestConfig publishConfig;
    publishConfig.trackNamespace = "test";
    publishConfig.trackName = "interop-track";
    publishConfig.serverUrl = config.relayUrl;

    TestResult result = TestResult::ERROR;

    try {
        result = test->runTest(publishConfig);
    } catch (const std::exception& ex) {
        std::cout << "Exception in Publish Test: " << ex.what() << std::endl;
        result = TestResult::ERROR;
    }

    std::string error;
    if (result != TestResult::PASS) {
        error = test->getLastError();
    }

    recordResult("PublishTest", result, error);
    return result == TestResult::PASS;
}

void TestSuite::recordResult(const std::string& testName, TestResult result, const std::string& error) {
    totalTests_++;

    switch (result) {
        case TestResult::PASS:
            passedTests_++;
            std::cout << "✓ " << testName << " PASSED\n\n";
            break;
        case TestResult::FAIL:
            failedTests_++;
            std::cout << "✗ " << testName << " FAILED: " << error << "\n\n";
            errors_.push_back(testName + ": " + error);
            break;
        case TestResult::TIMEOUT:
            failedTests_++;
            std::cout << "✗ " << testName << " TIMED OUT\n\n";
            errors_.push_back(testName + ": Timeout");
            break;
        case TestResult::ERROR:
            failedTests_++;
            std::cout << "✗ " << testName << " ERROR: " << error << "\n\n";
            errors_.push_back(testName + ": " + error);
            break;
    }
}

void TestSuite::printSummary() const {
    std::cout << "=== Test Suite Summary ===\n";
    std::cout << "Total Tests: " << totalTests_ << "\n";
    std::cout << "Passed: " << passedTests_ << "\n";
    std::cout << "Failed: " << failedTests_ << "\n";

    if (!errors_.empty()) {
        std::cout << "\nFailures:\n";
        for (const auto& error : errors_) {
            std::cout << "  - " << error << "\n";
        }
    }

    std::cout << "\n";
    if (failedTests_ == 0) {
        std::cout << "All tests passed! ✓\n";
    } else {
        std::cout << "Some tests failed.\n";
    }
}

} // namespace interop_test
