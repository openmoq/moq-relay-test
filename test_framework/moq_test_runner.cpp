/*
 * MOQ Server Test Runner
 *
 * Example application demonstrating how to use the MOQ Server Test Framework
 * to validate MOQ servers by sending PUBLISH requests and validating responses.
 */

#include "MoQServerTestFramework.h"
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <iostream>

using namespace moq_test_framework;

// Command line flags
DEFINE_string(server_url, "https://localhost:4433/moq",
              "MOQ server URL to test");
DEFINE_bool(run_basic_only, false,
            "Run only basic tests instead of comprehensive suite");
DEFINE_bool(verbose, false,
            "Enable verbose logging");
DEFINE_int32(timeout_ms, 5000,
             "Default timeout for tests in milliseconds");

// Custom callback that saves results to a file
class FileTestResultCallback : public TestResultCallback {
public:
  explicit FileTestResultCallback(const std::string& filename)
      : filename_(filename) {
    logFile_.open(filename_);
    if (!logFile_.is_open()) {
      throw std::runtime_error("Failed to open log file: " + filename_);
    }

    // Write header
    logFile_ << "MOQ Server Test Results\n";
    logFile_ << "=======================\n";
    logFile_ << "Server: " << FLAGS_server_url << "\n";
    logFile_ << "Timestamp: " << getCurrentTimestamp() << "\n\n";
  }

  ~FileTestResultCallback() {
    if (logFile_.is_open()) {
      logFile_.close();
    }
  }

  void onTestComplete(const TestExecutionResult& result) override {
    // Also call console callback
    consoleCallback_.onTestComplete(result);

    // Write to file
    logFile_ << "Test: " << result.testName << "\n";
    logFile_ << "Result: " << resultToString(result.result) << "\n";
    logFile_ << "Duration: " << result.duration.count() << "ms\n";
    logFile_ << "Message: " << result.message << "\n";

    if (result.publishOk.hasValue()) {
      logFile_ << "PUBLISH_OK received:\n";
      logFile_ << "  Request ID: " << result.publishOk->requestID.value << "\n";
      logFile_ << "  Forward: " << (result.publishOk->forward ? "true" : "false") << "\n";
      logFile_ << "  Subscriber Priority: " << static_cast<int>(result.publishOk->subscriberPriority) << "\n";
    }

    if (result.publishError.hasValue()) {
      logFile_ << "PUBLISH_ERROR received:\n";
      logFile_ << "  Request ID: " << result.publishError->requestID.value << "\n";
      logFile_ << "  Error Code: " << static_cast<int>(result.publishError->errorCode) << "\n";
      logFile_ << "  Reason: " << result.publishError->reasonPhrase << "\n";
    }

    logFile_ << "\n";
    logFile_.flush();
  }

  void onTestProgress(const std::string& testName, const std::string& message) override {
    if (FLAGS_verbose) {
      consoleCallback_.onTestProgress(testName, message);
      logFile_ << "[PROGRESS] " << testName << ": " << message << "\n";
      logFile_.flush();
    }
  }

private:
  std::string resultToString(TestResult result) {
    switch (result) {
      case TestResult::PASS: return "PASS";
      case TestResult::FAIL: return "FAIL";
      case TestResult::TIMEOUT: return "TIMEOUT";
      case TestResult::ERROR: return "ERROR";
    }
    return "UNKNOWN";
  }

  std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
  }

  std::string filename_;
  std::ofstream logFile_;
  ConsoleTestResultCallback consoleCallback_;
};

// Function to create custom test configurations
std::vector<PublishTestConfig> createCustomTests() {
  std::vector<PublishTestConfig> tests;

  // Test with realistic live streaming scenario
  tests.push_back(
      PublishTestBuilder()
          .withTestName("LiveStream_Video")
          .withTrackNamespace("live/stream1")
          .withTrackName("video")
          .withTrackAlias(moxygen::TrackAlias{100})
          .withGroupOrder(moxygen::GroupOrder::NewestFirst)
          .withForward(true)
          .withTimeout(std::chrono::milliseconds(FLAGS_timeout_ms))
          .withDescription("Live video stream PUBLISH request")
          .build()
  );

  // Test with realistic live streaming scenario - audio
  tests.push_back(
      PublishTestBuilder()
          .withTestName("LiveStream_Audio")
          .withTrackNamespace("live/stream1")
          .withTrackName("audio")
          .withTrackAlias(moxygen::TrackAlias{101})
          .withGroupOrder(moxygen::GroupOrder::NewestFirst)
          .withForward(true)
          .withTimeout(std::chrono::milliseconds(FLAGS_timeout_ms))
          .withDescription("Live audio stream PUBLISH request")
          .build()
  );

  // Test with VOD scenario
  tests.push_back(
      PublishTestBuilder()
          .withTestName("VOD_Content")
          .withTrackNamespace("vod/movie123")
          .withTrackName("video")
          .withTrackAlias(moxygen::TrackAlias{200})
          .withGroupOrder(moxygen::GroupOrder::OldestFirst)
          .withForward(false)
          .withTimeout(std::chrono::milliseconds(FLAGS_timeout_ms))
          .withDescription("VOD content PUBLISH request")
          .build()
  );

  // Test with chat/metadata scenario
  tests.push_back(
      PublishTestBuilder()
          .withTestName("Chat_Messages")
          .withTrackNamespace("chat/room1")
          .withTrackName("messages")
          .withTrackAlias(moxygen::TrackAlias{300})
          .withGroupOrder(moxygen::GroupOrder::NewestFirst)
          .withForward(true)
          .withTimeout(std::chrono::milliseconds(FLAGS_timeout_ms))
          .withDescription("Chat messages PUBLISH request")
          .build()
  );

  return tests;
}

// Main test execution function
folly::coro::Task<int> runTests(folly::EventBase* evb) {
  try {
    // Parse server URL
    proxygen::URL serverUrl(FLAGS_server_url);
    if (!serverUrl.isValid()) {
      std::cerr << "Invalid server URL: " << FLAGS_server_url << std::endl;
      co_return 1;
    }

    std::cout << "MOQ Server Test Runner\n";
    std::cout << "=====================\n";
    std::cout << "Testing server: " << FLAGS_server_url << "\n";
    std::cout << "Timeout: " << FLAGS_timeout_ms << "ms\n\n";

    // Create test result callback (save to file + console)
    auto callback = std::make_shared<FileTestResultCallback>("moq_test_results.log");

    // Create test framework
    auto testFramework = std::make_shared<MoQServerTestFramework>(
        evb, serverUrl, callback);

    // Initialize the framework
    co_await testFramework->initialize();

    // Connect to server
    bool connected = co_await testFramework->connectToServer();
    if (!connected) {
      std::cerr << "Failed to connect to MOQ server" << std::endl;
      co_return 1;
    }

    // Configure tests
    if (FLAGS_run_basic_only) {
      std::cout << "Running basic tests only...\n\n";
      testFramework->addPublishTest(StandardTestSuites::basicPublishTest());
    } else {
      std::cout << "Running comprehensive test suite...\n\n";

      // Add standard test suites
      auto comprehensiveTests = StandardTestSuites::comprehensiveTestSuite();
      for (const auto& test : comprehensiveTests) {
        testFramework->addPublishTest(test);
      }

      // Add custom realistic tests
      auto customTests = createCustomTests();
      for (const auto& test : customTests) {
        testFramework->addPublishTest(test);
      }
    }

    // Run all tests
    auto results = co_await testFramework->runAllTests();

    // Print detailed summary
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "DETAILED TEST SUMMARY\n";
    std::cout << std::string(50, '=') << "\n";

    size_t passed = 0, failed = 0, errors = 0, timeouts = 0;
    std::chrono::milliseconds totalDuration{0};

    for (const auto& result : results) {
      totalDuration += result.duration;

      switch (result.result) {
        case TestResult::PASS: passed++; break;
        case TestResult::FAIL: failed++; break;
        case TestResult::ERROR: errors++; break;
        case TestResult::TIMEOUT: timeouts++; break;
      }

      // Print details for failed tests
      if (result.result != TestResult::PASS) {
        std::cout << "\n❌ " << result.testName << " - " << result.message;

        if (result.publishError.hasValue()) {
          std::cout << "\n   Error Code: " << static_cast<int>(result.publishError->errorCode);
        }
      }
    }

    std::cout << "\n\nFINAL RESULTS:\n";
    std::cout << "Total Tests: " << results.size() << "\n";
    std::cout << "✅ Passed: " << passed << "\n";
    std::cout << "❌ Failed: " << failed << "\n";
    std::cout << "⚠️  Errors: " << errors << "\n";
    std::cout << "⏰ Timeouts: " << timeouts << "\n";
    std::cout << "Total Duration: " << totalDuration.count() << "ms\n";

    double successRate = results.empty() ? 0.0 :
                        (static_cast<double>(passed) / results.size() * 100.0);
    std::cout << "Success Rate: " << std::fixed << std::setprecision(1)
              << successRate << "%\n";

    if (successRate >= 80.0) {
      std::cout << "\n🎉 Server validation PASSED (>= 80% success rate)\n";
    } else {
      std::cout << "\n⚠️  Server validation FAILED (< 80% success rate)\n";
    }

    std::cout << "\nDetailed results saved to: moq_test_results.log\n";

    // Disconnect
    testFramework->disconnect();

    // Return exit code based on success rate
    co_return (successRate >= 80.0) ? 0 : 1;

  } catch (const std::exception& e) {
    std::cerr << "Test execution failed: " << e.what() << std::endl;
    co_return 1;
  }
}

int main(int argc, char* argv[]) {
  // Initialize folly and gflags
  folly::Init init(&argc, &argv);

  if (FLAGS_verbose) {
    // Enable more detailed logging
    FLAGS_minloglevel = 0;
  }

  // Create event base for async operations
  folly::EventBase evb;

  // Run the tests
  auto task = runTests(&evb);
  int result = folly::coro::blockingWait(std::move(task));

  return result;
}