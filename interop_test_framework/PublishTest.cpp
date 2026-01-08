#include "PublishTest.h"
#include <chrono>
#include <thread>
#include <folly/coro/BlockingWait.h>

namespace interop_test {

TestResult PublishTest::runTest(const PublishTestConfig& config) {
    try {
        std::cout << "Starting MOQ publish test for track: "
                  << config.trackNamespace << "/" << config.trackName << std::endl;

        // Create MoQ interface
        moqInterface_ = std::make_shared<moq_interface::MoQInterface>(eventBase_);

        // Establish MOQ session
        bool connected = folly::coro::blockingWait(moqInterface_->connect(config.serverUrl));

        if (!connected) {
            lastError_ = "Failed to establish MoQ session";
            return TestResult::ERROR;
        }

        // Create subscription handle
        if (!subscriptionHandle_) {
            subscriptionHandle_ = std::make_shared<TestSubscriptionHandle>();
        }

        // Send publish request
        bool publishResult = folly::coro::blockingWait(moqInterface_->publish(
            config.trackNamespace,
            config.trackName,
            subscriptionHandle_
        ));

        if (!publishResult) {
            lastError_ = "Publish request failed";
            return TestResult::FAIL;
        }

        std::cout << "Publish test completed successfully" << std::endl;
        return TestResult::PASS;

    } catch (const std::exception& ex) {
        lastError_ = ex.what();
        std::cout << "Publish test failed: " << lastError_ << std::endl;
        return TestResult::ERROR;
    }
}

void PublishTest::cleanup() {
    std::cout << "Starting cleanup..." << std::endl;

    // Clean up subscription handle first
    if (subscriptionHandle_) {
        std::cout << "Cleaning up subscription handle..." << std::endl;
        subscriptionHandle_.reset();
    }

    if (moqInterface_) {
        std::cout << "Cleaning up MoQ interface..." << std::endl;
        try {
            auto client = moqInterface_->getClient();
            if (eventBase_ && client && client->moqSession_) {
                eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait([client]() {
                    try {
                        std::cout << "Closing MoQ session gracefully..." << std::endl;
                        client->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
                        std::cout << "MoQ session closed" << std::endl;
                    } catch (const std::exception& ex) {
                        std::cout << "Exception during session close: " << ex.what() << std::endl;
                    }
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            moqInterface_.reset();
        } catch (const std::exception& ex) {
            std::cout << "Exception during cleanup: " << ex.what() << std::endl;
        }
    }

    eventBase_ = nullptr;
    std::cout << "Cleanup completed" << std::endl;
}} // namespace interop_test