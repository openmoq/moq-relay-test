#include "test_suite.h"
#include "base/base_test.h"
#include <iostream>
#include <algorithm>

namespace interop_test {

TestSuite::TestSuite(folly::EventBase* eventBase)
    : eventBase_(eventBase) {
}

bool TestSuite::runAll(const TestSuiteConfig& config) {
    std::cout << "=== Running MoQ Interop Test Suite ===\n";
    std::cout << "Relay URL: " << config.relayUrl << "\n\n";

    // Create test context
    TestContext testContext(config.relayUrl, eventBase_);
    testContext.verbose = false;

    // Get tests from registry
    auto tests = TestRegistry::instance().getTestsByCategory(config.category);
    
    if (tests.empty()) {
        std::cout << "No tests found in registry!\n";
        return false;
    }

    // Filter tests by name if specific tests are requested
    if (!config.testNames.empty()) {
        std::vector<TestInfo> filteredTests;
        for (const auto& testInfo : tests) {
            if (std::find(config.testNames.begin(), config.testNames.end(), testInfo.name) != config.testNames.end()) {
                filteredTests.push_back(testInfo);
            }
        }
        tests = filteredTests;
        
        if (tests.empty()) {
            std::cout << "No matching tests found!\n";
            return false;
        }
    }

    std::cout << "Found " << tests.size() << " test(s) to run\n\n";

    // Run all registered tests
    bool allPassed = true;
    for (const auto& testInfo : tests) {
        bool passed = runTest(testInfo.name, testContext);
        allPassed = allPassed && passed;
    }

    // Print summary
    printSummary();

    return allPassed;
}

bool TestSuite::runTest(const std::string& testName, const TestContext& context) {
    std::cout << "\n=== Running Test: " << testName << " ===\n";

    try {
        auto test = TestRegistry::instance().createTest(testName, context);
        
        if (!test) {
            recordResult(testName, TestResult::ERROR, "Failed to create test instance");
            return false;
        }
        
        std::cout << "Description: " << test->getDescription() << "\n";
        
        TestResult result = test->run();
        
        std::string error;
        if (result != TestResult::PASS) {
            error = test->getLastError();
        }

        recordResult(test->getName(), result, error);
        return result == TestResult::PASS;
    } catch (const std::exception& e) {
        recordResult(testName, TestResult::ERROR, std::string("Exception: ") + e.what());
        return false;
    }
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
