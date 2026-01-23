#pragma once

#include "base/base_test.h"
#include "test_registry.h"
#include <folly/io/async/EventBase.h>
#include <memory>
#include <string>
#include <vector>

namespace interop_test {

struct TestSuiteConfig {
  std::string relayUrl{"https://localhost:4433/moq"};
  TestCategory category{TestCategory::ALL}; // Run tests from specific category
  std::vector<std::string>
      testNames; // Specific test names to run (empty = run all)
};

class TestSuite {
public:
  explicit TestSuite(folly::EventBase *eventBase);
  ~TestSuite() = default;

  // Run all tests in the suite (or by category)
  bool runAll(const TestSuiteConfig &config);

  // Run a specific test by name
  bool runTest(const std::string &testName, const TestContext &context);

  // Get summary of results
  void printSummary() const;

private:
  folly::EventBase *eventBase_;
  int totalTests_{0};
  int passedTests_{0};
  int failedTests_{0};
  std::vector<std::string> errors_;

  void recordResult(const std::string &testName, TestResult result,
                    const std::string &error = "");
};

} // namespace interop_test
