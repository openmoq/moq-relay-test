#include "test_suite.h"
#include <algorithm>
#include <folly/String.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBaseThread.h>
#include <gflags/gflags.h>
#include <iostream>
#include <map>

DEFINE_string(relay, "https://localhost:4433/moq", "MoQ relay server URL");
DEFINE_string(
    tests, "",
    "Comma-separated list of test names to run (runs all tests if empty)");
DEFINE_string(
    categories, "",
    "Comma-separated list of categories to filter tests (e.g., 'Publisher,Subscriber'). "
    "Available: Publisher, Subscriber, Namespace, ErrorHandling, Update, Connection, All");
DEFINE_bool(list, false, "List all available tests without running them");

using namespace interop_test;

/**
 * Parse category names from string to bit flags
 */
TestCategory parseCategoryFilter(const std::string& categoryStr) {
  if (categoryStr.empty()) {
    return TestCategory::ALL;
  }

  // Category name to flag mapping
  static const std::map<std::string, TestCategory> categoryMap = {
    {"publisher", TestCategory::PUBLISHER},
    {"subscriber", TestCategory::SUBSCRIBER},
    {"namespace", TestCategory::NAMESPACE},
    {"errorhandling", TestCategory::ERROR_HANDLING},
    {"error_handling", TestCategory::ERROR_HANDLING},
    {"update", TestCategory::UPDATE},
    {"connection", TestCategory::CONNECTION},
    {"all", TestCategory::ALL}
  };

  std::vector<std::string> categoryNames;
  folly::split(',', categoryStr, categoryNames, true);

  TestCategory filter = TestCategory::NONE;
  
  for (const auto& name : categoryNames) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    auto it = categoryMap.find(lower);
    if (it != categoryMap.end()) {
      if (it->second == TestCategory::ALL) {
        return TestCategory::ALL;  // If "all" is specified, return immediately
      }
      filter |= it->second;
    } else {
      std::cerr << "Warning: Unknown category '" << name << "', ignoring.\n";
    }
  }
  
  // If no valid categories were parsed, default to ALL
  if (filter == TestCategory::NONE) {
    return TestCategory::ALL;
  }
  
  return filter;
}

int main(int argc, char *argv[]) {
  folly::Init init(&argc, &argv);

  // Create EventBaseThread to continuously drive the event loop in background
  // This allows MoQ connections to process incoming messages (e.g., SUBSCRIBE 
  // requests from relay) even when test code is sleeping or doing other work
  folly::EventBaseThread eventBaseThread;
  auto eventBase = eventBaseThread.getEventBase();

  // Create test suite
  auto testSuite = std::make_unique<TestSuite>(eventBase);

  // Configure test suite
  TestSuiteConfig config;
  config.relayUrl = FLAGS_relay;
  config.categoryFilter = parseCategoryFilter(FLAGS_categories);

  // Parse test names if provided
  if (!FLAGS_tests.empty()) {
    folly::split(',', FLAGS_tests, config.testNames, true);
  }

  // List tests if requested
  if (FLAGS_list) {
    testSuite->listTests(config);
    return 0;
  }

  // Run all tests
  bool success = testSuite->runAll(config);

  // // Destroy test suite BEFORE EventBaseThread so all sessions are cleaned up
  // // before the EventBase stops.
  // testSuite.reset();

  // eventBaseThread.stop();

  return success ? 0 : 1;
}