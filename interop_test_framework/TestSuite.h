#pragma once

#include "PublishTest.h"
#include <memory>
#include <vector>
#include <string>
#include <folly/io/async/EventBase.h>

namespace interop_test {

struct TestSuiteConfig {
    std::string relayUrl{"https://localhost:4433/moq"};
};

class TestSuite {
public:
    explicit TestSuite(folly::EventBase* eventBase);
    ~TestSuite() = default;

    // Run all tests in the suite
    bool runAll(const TestSuiteConfig& config);

    // Run individual tests
    bool runPublishTest(const TestSuiteConfig& config);

    // Get summary of results
    void printSummary() const;

private:
    folly::EventBase* eventBase_;
    int totalTests_{0};
    int passedTests_{0};
    int failedTests_{0};
    std::vector<std::string> errors_;

    void recordResult(const std::string& testName, TestResult result, const std::string& error = "");
};

} // namespace interop_test
