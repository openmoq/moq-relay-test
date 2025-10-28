/*
 * MOQ Server Test Framework
 *
 * A comprehensive test framework for validating MOQ servers by sending
 * various MOQ protocol requests and validating responses.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <folly/coro/Task.h>
#include <folly/Expected.h>
#include <proxygen/lib/utils/URL.h>

// Include moxygen headers
#include "moxygen/MoQClient.h"
#include "moxygen/MoQFramer.h"
#include "moxygen/MoQSession.h"
#include "moxygen/events/MoQFollyExecutorImpl.h"
#include "moxygen/Subscriber.h"

namespace moq_test_framework {

// Test result types
enum class TestResult {
  PASS,
  FAIL,
  TIMEOUT,
  ERROR
};

// Test case configuration
struct TestConfig {
  std::string testName;
  std::chrono::milliseconds timeout{5000};
  bool expectSuccess{true};
  std::string description;
};

// PUBLISH test specific configuration
struct PublishTestConfig : public TestConfig {
  moxygen::TrackNamespace trackNamespace{"test"};
  std::string trackName{"test-track"};
  moxygen::TrackAlias trackAlias{1};
  moxygen::GroupOrder groupOrder{moxygen::GroupOrder::Default};
  bool forward{true};
  std::vector<moxygen::TrackRequestParameter> params;

  PublishTestConfig() {
    testName = "PublishTest";
    description = "Test PUBLISH request with OK response validation";
  }
};

// Test execution result
struct TestExecutionResult {
  TestResult result;
  std::string testName;
  std::string message;
  std::chrono::milliseconds duration{0};

  // PUBLISH specific results
  folly::Optional<moxygen::PublishOk> publishOk;
  folly::Optional<moxygen::PublishError> publishError;
};

// Callback interface for test results
class TestResultCallback {
public:
  virtual ~TestResultCallback() = default;
  virtual void onTestComplete(const TestExecutionResult& result) = 0;
  virtual void onTestProgress(const std::string& testName, const std::string& message) = 0;
};

// Main test framework class
class MoQServerTestFramework : public moxygen::Subscriber,
                              public std::enable_shared_from_this<MoQServerTestFramework> {
public:
  explicit MoQServerTestFramework(
      folly::EventBase* evb,
      const proxygen::URL& serverUrl,
      std::shared_ptr<TestResultCallback> callback = nullptr);

  ~MoQServerTestFramework() override = default;

  // Core test execution methods
  folly::coro::Task<void> initialize();
  folly::coro::Task<TestExecutionResult> runPublishTest(const PublishTestConfig& config);
  folly::coro::Task<std::vector<TestExecutionResult>> runAllTests();

  // Test configuration
  void addPublishTest(const PublishTestConfig& config);
  void setTestResultCallback(std::shared_ptr<TestResultCallback> callback);

  // Connection management
  folly::coro::Task<bool> connectToServer();
  void disconnect();
  bool isConnected() const;

  // Subscriber interface overrides (required for moxygen::Subscriber)
  void goaway(moxygen::Goaway goaway) override;
  folly::coro::Task<moxygen::Subscriber::AnnounceResult> announce(
      moxygen::Announce ann,
      std::shared_ptr<moxygen::AnnounceCallback> callback = nullptr) override;

private:
  // Internal test execution helpers
  folly::coro::Task<TestExecutionResult> executePublishTest(
      const PublishTestConfig& config);

  // Message validation helpers
  bool validatePublishOk(
      const moxygen::PublishOk& publishOk,
      const moxygen::PublishRequest& originalRequest);

  bool validatePublishError(
      const moxygen::PublishError& publishError,
      const moxygen::PublishRequest& originalRequest);

  // Utility methods
  moxygen::PublishRequest createPublishRequest(
      const PublishTestConfig& config,
      uint64_t requestId);

  std::string formatTestResult(const TestExecutionResult& result);
  void logTestProgress(const std::string& testName, const std::string& message);

  // Member variables
  folly::EventBase* eventBase_;
  proxygen::URL serverUrl_;
  std::shared_ptr<TestResultCallback> resultCallback_;

  // MOQ client components
  std::shared_ptr<moxygen::MoQFollyExecutorImpl> executor_;
  std::unique_ptr<moxygen::MoQClient> client_;
  std::shared_ptr<moxygen::MoQSession> session_;

  // Test configuration
  std::vector<PublishTestConfig> publishTests_;

  // Connection state
  bool connected_;
  uint64_t nextRequestId_;
};

// Utility classes for common test scenarios

// Simple test result callback that logs to console
class ConsoleTestResultCallback : public TestResultCallback {
public:
  void onTestComplete(const TestExecutionResult& result) override;
  void onTestProgress(const std::string& testName, const std::string& message) override;
};

// Builder class for easy test configuration
class PublishTestBuilder {
public:
  PublishTestBuilder& withTestName(const std::string& name);
  PublishTestBuilder& withTrackNamespace(const std::string& ns);
  PublishTestBuilder& withTrackName(const std::string& name);
  PublishTestBuilder& withTrackAlias(moxygen::TrackAlias alias);
  PublishTestBuilder& withGroupOrder(moxygen::GroupOrder order);
  PublishTestBuilder& withForward(bool forward);
  PublishTestBuilder& withTimeout(std::chrono::milliseconds timeout);
  PublishTestBuilder& withExpectSuccess(bool expectSuccess);
  PublishTestBuilder& withDescription(const std::string& description);
  PublishTestBuilder& withParameter(const moxygen::TrackRequestParameter& param);

  PublishTestConfig build();

private:
  PublishTestConfig config_;
};

// Predefined test suites for common scenarios
class StandardTestSuites {
public:
  // Basic PUBLISH test with minimal valid parameters
  static PublishTestConfig basicPublishTest();

  // PUBLISH test with various track namespaces
  static std::vector<PublishTestConfig> trackNamespaceTests();

  // PUBLISH test with different group ordering
  static std::vector<PublishTestConfig> groupOrderTests();

  // PUBLISH test with forward/backward settings
  static std::vector<PublishTestConfig> forwardTests();

  // Invalid PUBLISH tests (expecting errors)
  static std::vector<PublishTestConfig> invalidPublishTests();

  // Complete test suite combining all scenarios
  static std::vector<PublishTestConfig> comprehensiveTestSuite();
};

} // namespace moq_test_framework