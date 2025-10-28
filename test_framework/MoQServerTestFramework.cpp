/*
 * MOQ Server Test Framework Implementation
 */

#include "MoQServerTestFramework.h"
#include <folly/logging/xlog.h>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace moq_test_framework {

// ============================================================================
// MoQServerTestFramework Implementation
// ============================================================================

MoQServerTestFramework::MoQServerTestFramework(
    folly::EventBase* evb,
    const proxygen::URL& serverUrl,
    std::shared_ptr<TestResultCallback> callback)
    : eventBase_(evb),
      serverUrl_(serverUrl),
      resultCallback_(std::move(callback)),
      executor_(std::make_shared<moxygen::MoQFollyExecutorImpl>(evb)),
      connected_(false),
      nextRequestId_(1) {

  if (!resultCallback_) {
    resultCallback_ = std::make_shared<ConsoleTestResultCallback>();
  }
}

folly::coro::Task<void> MoQServerTestFramework::initialize() {
  try {
    // Create the MOQ client
    client_ = std::make_unique<moxygen::MoQClient>(executor_, serverUrl_);

    logTestProgress("Framework", "Initialized MOQ test framework");
    co_return;

  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to initialize test framework: " << e.what();
    throw;
  }
}

folly::coro::Task<bool> MoQServerTestFramework::connectToServer() {
  try {
    logTestProgress("Connection", "Connecting to server: " + serverUrl_.getUrl());

    // Connect to the MOQ server
    auto connectResult = co_await client_->connect();
    if (connectResult.hasError()) {
      XLOG(ERR) << "Failed to connect to server: " << connectResult.error();
      co_return false;
    }

    session_ = connectResult.value();
    if (!session_) {
      XLOG(ERR) << "Failed to establish session";
      co_return false;
    }

    // Set ourselves as the subscriber handler
    session_->setSubscribeHandler(shared_from_this());

    connected_ = true;
    logTestProgress("Connection", "Successfully connected to server");
    co_return true;

  } catch (const std::exception& e) {
    XLOG(ERR) << "Exception during connection: " << e.what();
    connected_ = false;
    co_return false;
  }
}

void MoQServerTestFramework::disconnect() {
  if (session_) {
    session_.reset();
  }
  if (client_) {
    client_.reset();
  }
  connected_ = false;
  logTestProgress("Connection", "Disconnected from server");
}

bool MoQServerTestFramework::isConnected() const {
  return connected_ && session_ != nullptr;
}

folly::coro::Task<TestExecutionResult> MoQServerTestFramework::runPublishTest(
    const PublishTestConfig& config) {

  if (!isConnected()) {
    TestExecutionResult result;
    result.result = TestResult::ERROR;
    result.testName = config.testName;
    result.message = "Not connected to server";
    co_return result;
  }

  co_return co_await executePublishTest(config);
}

folly::coro::Task<TestExecutionResult> MoQServerTestFramework::executePublishTest(
    const PublishTestConfig& config) {

  TestExecutionResult result;
  result.testName = config.testName;
  auto startTime = std::chrono::steady_clock::now();

  try {
    logTestProgress(config.testName, "Starting PUBLISH test");

    // Create the PUBLISH request
    auto publishRequest = createPublishRequest(config, nextRequestId_++);

    logTestProgress(config.testName,
        "Sending PUBLISH request for track: " + config.trackNamespace + "/" + config.trackName);

    // Send the PUBLISH request
    auto publishResult = session_->publish(publishRequest);

    if (publishResult.hasError()) {
      // PUBLISH request failed immediately
      result.result = config.expectSuccess ? TestResult::FAIL : TestResult::PASS;
      result.message = "PUBLISH request failed: " + publishResult.error().reasonPhrase;
      result.publishError = publishResult.error();

      logTestProgress(config.testName, result.message);
    } else {
      // PUBLISH request succeeded, wait for async response
      auto publishConsumerAndReply = publishResult.value();

      logTestProgress(config.testName, "PUBLISH request sent, waiting for response");

      // Wait for the async reply with timeout
      try {
        auto replyTask = std::move(publishConsumerAndReply.reply);
        auto timeoutTask = folly::coro::sleep(config.timeout);

        auto replyResult = co_await folly::coro::co_withCancellation(
          folly::CancellationToken{},
          std::move(replyTask)
        );

        if (replyResult.hasValue()) {
          // Received PUBLISH_OK
          result.result = config.expectSuccess ? TestResult::PASS : TestResult::FAIL;
          result.message = "Received PUBLISH_OK response";
          result.publishOk = replyResult.value();

          // Validate the PUBLISH_OK response
          if (config.expectSuccess && !validatePublishOk(replyResult.value(), publishRequest)) {
            result.result = TestResult::FAIL;
            result.message += " (validation failed)";
          }

        } else {
          // Received PUBLISH_ERROR
          result.result = config.expectSuccess ? TestResult::FAIL : TestResult::PASS;
          result.message = "Received PUBLISH_ERROR: " + replyResult.error().reasonPhrase;
          result.publishError = replyResult.error();

          // Validate the PUBLISH_ERROR response
          if (!config.expectSuccess && !validatePublishError(replyResult.error(), publishRequest)) {
            result.result = TestResult::FAIL;
            result.message += " (validation failed)";
          }
        }

        logTestProgress(config.testName, result.message);

      } catch (const folly::FutureTimeout&) {
        result.result = TestResult::TIMEOUT;
        result.message = "PUBLISH request timed out after " +
                        std::to_string(config.timeout.count()) + "ms";
        logTestProgress(config.testName, result.message);
      }
    }

  } catch (const std::exception& e) {
    result.result = TestResult::ERROR;
    result.message = "Exception during PUBLISH test: " + std::string(e.what());
    logTestProgress(config.testName, result.message);
  }

  // Calculate test duration
  auto endTime = std::chrono::steady_clock::now();
  result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);

  // Notify callback
  if (resultCallback_) {
    resultCallback_->onTestComplete(result);
  }

  co_return result;
}

folly::coro::Task<std::vector<TestExecutionResult>> MoQServerTestFramework::runAllTests() {
  std::vector<TestExecutionResult> results;

  logTestProgress("TestSuite", "Starting all configured tests");

  for (const auto& config : publishTests_) {
    auto result = co_await runPublishTest(config);
    results.push_back(result);
  }

  // Print summary
  size_t passed = 0, failed = 0, errors = 0, timeouts = 0;
  for (const auto& result : results) {
    switch (result.result) {
      case TestResult::PASS: passed++; break;
      case TestResult::FAIL: failed++; break;
      case TestResult::ERROR: errors++; break;
      case TestResult::TIMEOUT: timeouts++; break;
    }
  }

  std::stringstream summary;
  summary << "Test Suite Complete - Total: " << results.size()
          << ", Passed: " << passed
          << ", Failed: " << failed
          << ", Errors: " << errors
          << ", Timeouts: " << timeouts;

  logTestProgress("TestSuite", summary.str());

  co_return results;
}

void MoQServerTestFramework::addPublishTest(const PublishTestConfig& config) {
  publishTests_.push_back(config);
  logTestProgress("Config", "Added PUBLISH test: " + config.testName);
}

void MoQServerTestFramework::setTestResultCallback(
    std::shared_ptr<TestResultCallback> callback) {
  resultCallback_ = callback;
}

moxygen::PublishRequest MoQServerTestFramework::createPublishRequest(
    const PublishTestConfig& config,
    uint64_t requestId) {

  moxygen::PublishRequest request;
  request.requestID = moxygen::RequestID{requestId};
  request.fullTrackName = moxygen::FullTrackName{config.trackNamespace, config.trackName};
  request.trackAlias = config.trackAlias;
  request.groupOrder = config.groupOrder;
  request.largest = folly::none; // No location specified for basic test
  request.forward = config.forward;
  request.params = config.params;

  return request;
}

bool MoQServerTestFramework::validatePublishOk(
    const moxygen::PublishOk& publishOk,
    const moxygen::PublishRequest& originalRequest) {

  // Validate that the request ID matches
  if (publishOk.requestID.value != originalRequest.requestID.value) {
    XLOG(WARN) << "PUBLISH_OK request ID mismatch: expected "
               << originalRequest.requestID.value
               << ", got " << publishOk.requestID.value;
    return false;
  }

  // Additional validations can be added here
  // For example, checking if the server's response parameters are reasonable

  return true;
}

bool MoQServerTestFramework::validatePublishError(
    const moxygen::PublishError& publishError,
    const moxygen::PublishRequest& originalRequest) {

  // Validate that the request ID matches
  if (publishError.requestID.value != originalRequest.requestID.value) {
    XLOG(WARN) << "PUBLISH_ERROR request ID mismatch: expected "
               << originalRequest.requestID.value
               << ", got " << publishError.requestID.value;
    return false;
  }

  // Check if error code is reasonable (not internal server errors for client mistakes)
  // This depends on the specific error codes defined in the MOQ specification

  return true;
}

void MoQServerTestFramework::logTestProgress(
    const std::string& testName,
    const std::string& message) {

  if (resultCallback_) {
    resultCallback_->onTestProgress(testName, message);
  }

  XLOG(INFO) << "[" << testName << "] " << message;
}

// Required Subscriber interface implementations
void MoQServerTestFramework::goaway(moxygen::Goaway goaway) {
  logTestProgress("Session", "Received GOAWAY: " + goaway.reasonPhrase);
}

folly::coro::Task<moxygen::Subscriber::AnnounceResult>
MoQServerTestFramework::announce(
    moxygen::Announce ann,
    std::shared_ptr<moxygen::AnnounceCallback> callback) {

  // For testing purposes, we can accept all announces
  moxygen::AnnounceOk announceOk;
  announceOk.requestID = ann.requestID;
  announceOk.trackNamespace = ann.trackNamespace;

  co_return moxygen::Subscriber::AnnounceResult{
    folly::makeExpected<moxygen::AnnounceError>(announceOk)
  };
}

// ============================================================================
// ConsoleTestResultCallback Implementation
// ============================================================================

void ConsoleTestResultCallback::onTestComplete(const TestExecutionResult& result) {
  std::string status;
  std::string color;

  switch (result.result) {
    case TestResult::PASS:
      status = "PASS";
      color = "\033[32m"; // Green
      break;
    case TestResult::FAIL:
      status = "FAIL";
      color = "\033[31m"; // Red
      break;
    case TestResult::TIMEOUT:
      status = "TIMEOUT";
      color = "\033[33m"; // Yellow
      break;
    case TestResult::ERROR:
      status = "ERROR";
      color = "\033[35m"; // Magenta
      break;
  }

  std::cout << color << "[" << status << "]\033[0m "
            << std::setw(20) << std::left << result.testName
            << " (" << result.duration.count() << "ms) - "
            << result.message << std::endl;
}

void ConsoleTestResultCallback::onTestProgress(
    const std::string& testName,
    const std::string& message) {

  std::cout << "\033[36m[INFO]\033[0m "
            << std::setw(20) << std::left << testName
            << " - " << message << std::endl;
}

// ============================================================================
// PublishTestBuilder Implementation
// ============================================================================

PublishTestBuilder& PublishTestBuilder::withTestName(const std::string& name) {
  config_.testName = name;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withTrackNamespace(const std::string& ns) {
  config_.trackNamespace = moxygen::TrackNamespace{ns};
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withTrackName(const std::string& name) {
  config_.trackName = name;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withTrackAlias(moxygen::TrackAlias alias) {
  config_.trackAlias = alias;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withGroupOrder(moxygen::GroupOrder order) {
  config_.groupOrder = order;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withForward(bool forward) {
  config_.forward = forward;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withTimeout(std::chrono::milliseconds timeout) {
  config_.timeout = timeout;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withExpectSuccess(bool expectSuccess) {
  config_.expectSuccess = expectSuccess;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withDescription(const std::string& description) {
  config_.description = description;
  return *this;
}

PublishTestBuilder& PublishTestBuilder::withParameter(
    const moxygen::TrackRequestParameter& param) {
  config_.params.push_back(param);
  return *this;
}

PublishTestConfig PublishTestBuilder::build() {
  return config_;
}

// ============================================================================
// StandardTestSuites Implementation
// ============================================================================

PublishTestConfig StandardTestSuites::basicPublishTest() {
  return PublishTestBuilder()
      .withTestName("BasicPublish")
      .withTrackNamespace("test")
      .withTrackName("basic-track")
      .withTrackAlias(moxygen::TrackAlias{1})
      .withDescription("Basic PUBLISH test with minimal valid parameters")
      .build();
}

std::vector<PublishTestConfig> StandardTestSuites::trackNamespaceTests() {
  std::vector<PublishTestConfig> tests;

  std::vector<std::string> namespaces = {
    "live", "vod", "chat", "metadata", "test-namespace"
  };

  for (size_t i = 0; i < namespaces.size(); ++i) {
    tests.push_back(
        PublishTestBuilder()
            .withTestName("TrackNamespace_" + namespaces[i])
            .withTrackNamespace(namespaces[i])
            .withTrackName("test-track")
            .withTrackAlias(moxygen::TrackAlias{static_cast<uint64_t>(i + 10)})
            .withDescription("PUBLISH test with namespace: " + namespaces[i])
            .build()
    );
  }

  return tests;
}

std::vector<PublishTestConfig> StandardTestSuites::groupOrderTests() {
  std::vector<PublishTestConfig> tests;

  tests.push_back(
      PublishTestBuilder()
          .withTestName("GroupOrder_Default")
          .withGroupOrder(moxygen::GroupOrder::Default)
          .withTrackAlias(moxygen::TrackAlias{20})
          .withDescription("PUBLISH test with default group order")
          .build()
  );

  tests.push_back(
      PublishTestBuilder()
          .withTestName("GroupOrder_OldestFirst")
          .withGroupOrder(moxygen::GroupOrder::OldestFirst)
          .withTrackAlias(moxygen::TrackAlias{21})
          .withDescription("PUBLISH test with oldest first group order")
          .build()
  );

  tests.push_back(
      PublishTestBuilder()
          .withTestName("GroupOrder_NewestFirst")
          .withGroupOrder(moxygen::GroupOrder::NewestFirst)
          .withTrackAlias(moxygen::TrackAlias{22})
          .withDescription("PUBLISH test with newest first group order")
          .build()
  );

  return tests;
}

std::vector<PublishTestConfig> StandardTestSuites::forwardTests() {
  std::vector<PublishTestConfig> tests;

  tests.push_back(
      PublishTestBuilder()
          .withTestName("Forward_True")
          .withForward(true)
          .withTrackAlias(moxygen::TrackAlias{30})
          .withDescription("PUBLISH test with forward=true")
          .build()
  );

  tests.push_back(
      PublishTestBuilder()
          .withTestName("Forward_False")
          .withForward(false)
          .withTrackAlias(moxygen::TrackAlias{31})
          .withDescription("PUBLISH test with forward=false")
          .build()
  );

  return tests;
}

std::vector<PublishTestConfig> StandardTestSuites::invalidPublishTests() {
  std::vector<PublishTestConfig> tests;

  // Test with empty track namespace (should fail)
  tests.push_back(
      PublishTestBuilder()
          .withTestName("Invalid_EmptyNamespace")
          .withTrackNamespace("")
          .withTrackName("test-track")
          .withTrackAlias(moxygen::TrackAlias{40})
          .withExpectSuccess(false)
          .withDescription("PUBLISH test with empty namespace (expecting error)")
          .build()
  );

  // Test with empty track name (should fail)
  tests.push_back(
      PublishTestBuilder()
          .withTestName("Invalid_EmptyTrackName")
          .withTrackNamespace("test")
          .withTrackName("")
          .withTrackAlias(moxygen::TrackAlias{41})
          .withExpectSuccess(false)
          .withDescription("PUBLISH test with empty track name (expecting error)")
          .build()
  );

  return tests;
}

std::vector<PublishTestConfig> StandardTestSuites::comprehensiveTestSuite() {
  std::vector<PublishTestConfig> allTests;

  // Add basic test
  allTests.push_back(basicPublishTest());

  // Add namespace tests
  auto nsTests = trackNamespaceTests();
  allTests.insert(allTests.end(), nsTests.begin(), nsTests.end());

  // Add group order tests
  auto groupTests = groupOrderTests();
  allTests.insert(allTests.end(), groupTests.begin(), groupTests.end());

  // Add forward tests
  auto fwdTests = forwardTests();
  allTests.insert(allTests.end(), fwdTests.begin(), fwdTests.end());

  // Add invalid tests
  auto invalidTests = invalidPublishTests();
  allTests.insert(allTests.end(), invalidTests.begin(), invalidTests.end());

  return allTests;
}

} // namespace moq_test_framework