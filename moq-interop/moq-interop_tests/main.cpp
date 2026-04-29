#include "test_suite.h"
#include <algorithm>
#include <folly/String.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBaseThread.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
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

namespace it = interop_test;

/**
 * Parse category names from string to bit flags
 */
it::TestCategory parseCategoryFilter(const std::string& categoryStr) {
  if (categoryStr.empty()) {
    return it::TestCategory::ALL;
  }

  // Category name to flag mapping
  static const std::map<std::string, it::TestCategory> categoryMap = {
    {"publisher", it::TestCategory::PUBLISHER},
    {"subscriber", it::TestCategory::SUBSCRIBER},
    {"namespace", it::TestCategory::NAMESPACE},
    {"errorhandling", it::TestCategory::ERROR_HANDLING},
    {"error_handling", it::TestCategory::ERROR_HANDLING},
    {"update", it::TestCategory::UPDATE},
    {"connection", it::TestCategory::CONNECTION},
    {"all", it::TestCategory::ALL}
  };

  std::vector<std::string> categoryNames;
  folly::split(',', categoryStr, categoryNames, true);

  it::TestCategory filter = it::TestCategory::NONE;
  
  for (const auto& name : categoryNames) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    auto mapIt = categoryMap.find(lower);
    if (mapIt != categoryMap.end()) {
      if (mapIt->second == it::TestCategory::ALL) {
        return it::TestCategory::ALL;  // If "all" is specified, return immediately
      }
      filter |= mapIt->second;
    } else {
      LOG(WARNING) << "Unknown category '" << name << "', ignoring.";
    }
  }
  
  // If no valid categories were parsed, default to ALL
  if (filter == it::TestCategory::NONE) {
    return it::TestCategory::ALL;
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
  auto testSuite = std::make_unique<it::TestSuite>(eventBase);

  // Configure test suite
  it::TestSuiteConfig config;
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

  return success ? 0 : 1;
}