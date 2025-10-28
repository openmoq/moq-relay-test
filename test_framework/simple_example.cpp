/*
 * Simple Example: Using MOQ Server Test Framework
 *
 * This example demonstrates the basic usage of the MOQ Server Test Framework
 * to validate a MOQ server with a few simple tests.
 */

#include "MoQServerTestFramework.h"
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <iostream>

using namespace moq_test_framework;

folly::coro::Task<void> runSimpleTest() {
    try {
        // Create event base for async operations
        folly::EventBase evb;

        // Configure server URL (change this to your MOQ server)
        proxygen::URL serverUrl("https://localhost:4433/moq");

        std::cout << "MOQ Server Test - Simple Example\n";
        std::cout << "================================\n";
        std::cout << "Testing server: " << serverUrl.getUrl() << "\n\n";

        // Create test framework with console callback
        auto testFramework = std::make_shared<MoQServerTestFramework>(
            &evb, serverUrl);

        // Initialize the framework
        co_await testFramework->initialize();
        std::cout << "✅ Test framework initialized\n";

        // Connect to the server
        bool connected = co_await testFramework->connectToServer();
        if (!connected) {
            std::cout << "❌ Failed to connect to MOQ server\n";
            co_return;
        }
        std::cout << "✅ Connected to MOQ server\n\n";

        // Add a few simple tests
        std::cout << "Configuring tests...\n";

        // Test 1: Basic PUBLISH request
        testFramework->addPublishTest(
            PublishTestBuilder()
                .withTestName("Basic_PUBLISH")
                .withTrackNamespace("test")
                .withTrackName("basic-track")
                .withTrackAlias(moxygen::TrackAlias{1})
                .withDescription("Basic PUBLISH request test")
                .build()
        );

        // Test 2: Live streaming scenario
        testFramework->addPublishTest(
            PublishTestBuilder()
                .withTestName("Live_Video")
                .withTrackNamespace("live/stream1")
                .withTrackName("video")
                .withTrackAlias(moxygen::TrackAlias{100})
                .withGroupOrder(moxygen::GroupOrder::NewestFirst)
                .withForward(true)
                .withDescription("Live video streaming test")
                .build()
        );

        // Test 3: VOD scenario
        testFramework->addPublishTest(
            PublishTestBuilder()
                .withTestName("VOD_Content")
                .withTrackNamespace("vod/movie123")
                .withTrackName("video")
                .withTrackAlias(moxygen::TrackAlias{200})
                .withGroupOrder(moxygen::GroupOrder::OldestFirst)
                .withForward(false)
                .withDescription("VOD content test")
                .build()
        );

        // Test 4: Invalid request (expecting error)
        testFramework->addPublishTest(
            PublishTestBuilder()
                .withTestName("Invalid_Empty_Namespace")
                .withTrackNamespace("")  // Invalid: empty namespace
                .withTrackName("test-track")
                .withTrackAlias(moxygen::TrackAlias{300})
                .withExpectSuccess(false)  // We expect this to fail
                .withDescription("Invalid request with empty namespace")
                .build()
        );

        std::cout << "✅ Configured 4 test cases\n\n";

        // Run all tests
        std::cout << "Running tests...\n";
        std::cout << std::string(40, '-') << "\n";

        auto results = co_await testFramework->runAllTests();

        // Print summary
        std::cout << "\n" << std::string(40, '=') << "\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << std::string(40, '=') << "\n";

        size_t passed = 0, failed = 0, errors = 0, timeouts = 0;
        for (const auto& result : results) {
            switch (result.result) {
                case TestResult::PASS: passed++; break;
                case TestResult::FAIL: failed++; break;
                case TestResult::ERROR: errors++; break;
                case TestResult::TIMEOUT: timeouts++; break;
            }
        }

        std::cout << "Total Tests: " << results.size() << "\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Errors: " << errors << "\n";
        std::cout << "Timeouts: " << timeouts << "\n";

        double successRate = results.empty() ? 0.0 :
                            (static_cast<double>(passed) / results.size() * 100.0);
        std::cout << "Success Rate: " << std::fixed << std::setprecision(1)
                  << successRate << "%\n";

        if (successRate >= 75.0) {
            std::cout << "\n🎉 Server validation PASSED!\n";
        } else {
            std::cout << "\n⚠️  Server validation FAILED (< 75% success rate)\n";
        }

        // Show details for any failed tests
        bool hasFailures = false;
        for (const auto& result : results) {
            if (result.result != TestResult::PASS) {
                if (!hasFailures) {
                    std::cout << "\nFailed Test Details:\n";
                    std::cout << std::string(20, '-') << "\n";
                    hasFailures = true;
                }

                std::cout << "❌ " << result.testName << ": " << result.message << "\n";

                if (result.publishError.hasValue()) {
                    std::cout << "   Error Code: " << static_cast<int>(result.publishError->errorCode) << "\n";
                    std::cout << "   Reason: " << result.publishError->reasonPhrase << "\n";
                }
            }
        }

        // Disconnect
        testFramework->disconnect();
        std::cout << "\n✅ Disconnected from server\n";

    } catch (const std::exception& e) {
        std::cout << "❌ Test execution failed: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Initialize folly
    folly::Init init(&argc, &argv);

    // Run the simple test
    try {
        folly::coro::blockingWait(runSimpleTest());
        std::cout << "\nSimple test completed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Failed to run tests: " << e.what() << std::endl;
        return 1;
    }
}