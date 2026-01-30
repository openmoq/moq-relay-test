#pragma once

#include "base/fixture_interface.h"
#include <chrono>
#include <folly/io/async/EventBase.h>
#include <memory>
#include <string>
#include <vector>

namespace interop_test {

enum class TestResult { PASS, FAIL, TIMEOUT, ERROR };

/**
 * Test categories for organizing and filtering tests
 * Uses bit flags to allow tests to belong to multiple categories
 */
enum class TestCategory : uint32_t {
  NONE = 0,
  PUBLISHER = 1 << 0,        // 0x01
  SUBSCRIBER = 1 << 1,       // 0x02
  NAMESPACE = 1 << 2,        // 0x04
  ERROR_HANDLING = 1 << 3,   // 0x08
  UPDATE = 1 << 4,           // 0x10
  CONNECTION = 1 << 5,       // 0x20
  ALL = 0xFFFFFFFF
};

// Bit flag operators for TestCategory
inline TestCategory operator|(TestCategory a, TestCategory b) {
  return static_cast<TestCategory>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline TestCategory operator&(TestCategory a, TestCategory b) {
  return static_cast<TestCategory>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline TestCategory operator|=(TestCategory& a, TestCategory b) {
  a = a | b;
  return a;
}

inline bool hasCategory(TestCategory flags, TestCategory category) {
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(category)) != 0;
}

/**
 * Test context provides shared configuration and resources for tests
 */
struct TestContext {
  std::string relayUrl{"https://localhost:4433/moq"};
  folly::EventBase *eventBase{nullptr};
  std::chrono::milliseconds defaultTimeout{5000};
  bool verbose{false};

  TestContext() = default;
  TestContext(const std::string &url, folly::EventBase *eb)
      : relayUrl(url), eventBase(eb) {}
};

/**
 * Abstract base class for all MoQ interop tests
 *
 * Provides:
 * - Common test lifecycle (setup, execute, teardown)
 * - Test metadata (name, description, category)
 * - Shared utilities and fixture access
 * - Assertion helpers
 */
class BaseTest {
public:
  explicit BaseTest(const TestContext &context);
  virtual ~BaseTest() = default;

  // Disable copy and move
  BaseTest(const BaseTest &) = delete;
  BaseTest &operator=(const BaseTest &) = delete;
  BaseTest(BaseTest &&) = delete;
  BaseTest &operator=(BaseTest &&) = delete;

  /**
   * Main test execution method - calls setup, execute, teardown
   * @return TestResult indicating pass/fail/error/timeout
   */
  TestResult run();

  /**
   * Get the test name (used for display and filtering)
   */
  virtual std::string getName() const = 0;

  /**
   * Get human-readable test description
   */
  virtual std::string getDescription() const = 0;

  /**
   * Get test categories for organization and filtering
   * Returns a bitmask of TestCategory flags
   */
  virtual TestCategory getCategories() const = 0;

  /**
   * Get list of test names this test depends on (must run before this test)
   */
  virtual std::vector<std::string> getDependencies() const { return {}; }

  /**
   * Set the test fixture (called by derived classes or test framework)
   */
  void setFixture(std::shared_ptr<ITestFixture> fixture) { fixture_ = fixture; }

  /**
   * Get the last error message if test failed
   */
  std::string getLastError() const { return lastError_; }

  /**
   * Check if test has verbose output enabled
   */
  bool isVerbose() const { return context_.verbose; }

protected:
  /**
   * Test-specific setup logic (override if needed)
   * Called before execute()
   */
  virtual void setUp() {}

  /**
   * Core test logic - must be implemented by derived classes
   * @return TestResult indicating the outcome
   */
  virtual TestResult execute() = 0;

  /**
   * Test-specific cleanup logic (override if needed)
   * Called after execute(), even if test fails
   */
  virtual void tearDown() {}

  /**
   * Set error message (automatically marks test as failed)
   */
  void setError(const std::string &error);

  /**
   * Log a message (respects verbose flag)
   */
  void log(const std::string &message) const;

  /**
   * Log a message unconditionally
   */
  void logAlways(const std::string &message) const;

  // Assertion helpers
  void assertTrue(bool condition, const std::string &message);
  void assertFalse(bool condition, const std::string &message);

  template <typename T>
  void assertEqual(const T &expected, const T &actual,
                   const std::string &message);

  template <typename T>
  void assertNotEqual(const T &expected, const T &actual,
                      const std::string &message);

  void assertNotNull(const void *ptr, const std::string &message);

  // Access to shared context and fixture
  const TestContext &context_;
  std::shared_ptr<ITestFixture> fixture_;

private:
  std::string lastError_;
};

// Template implementations
template <typename T>
void BaseTest::assertEqual(const T &expected, const T &actual,
                           const std::string &message) {
  if (expected != actual) {
    setError(message + " (expected: " + std::to_string(expected) +
             ", actual: " + std::to_string(actual) + ")");
    throw std::runtime_error(lastError_);
  }
}

template <typename T>
void BaseTest::assertNotEqual(const T &expected, const T &actual,
                              const std::string &message) {
  if (expected == actual) {
    setError(message + " (values are equal: " + std::to_string(expected) + ")");
    throw std::runtime_error(lastError_);
  }
}

// String specializations
template <>
inline void BaseTest::assertEqual<std::string>(const std::string &expected,
                                               const std::string &actual,
                                               const std::string &message) {
  if (expected != actual) {
    setError(message + " (expected: \"" + expected + "\", actual: \"" + actual +
             "\")");
    throw std::runtime_error(lastError_);
  }
}

/**
 * Helper to convert TestCategory to string
 */
inline std::string testCategoryToString(TestCategory categories) {
  if (categories == TestCategory::ALL) {
    return "All";
  }
  if (categories == TestCategory::NONE) {
    return "None";
  }
  
  std::vector<std::string> names;
  if (hasCategory(categories, TestCategory::PUBLISHER)) names.push_back("Publisher");
  if (hasCategory(categories, TestCategory::SUBSCRIBER)) names.push_back("Subscriber");
  if (hasCategory(categories, TestCategory::NAMESPACE)) names.push_back("Namespace");
  if (hasCategory(categories, TestCategory::ERROR_HANDLING)) names.push_back("ErrorHandling");
  if (hasCategory(categories, TestCategory::UPDATE)) names.push_back("Update");
  if (hasCategory(categories, TestCategory::CONNECTION)) names.push_back("Connection");
  
  if (names.empty()) {
    return "Unknown";
  }
  
  std::string result;
  for (size_t i = 0; i < names.size(); ++i) {
    if (i > 0) result += ", ";
    result += names[i];
  }
  return result;
}

} // namespace interop_test
