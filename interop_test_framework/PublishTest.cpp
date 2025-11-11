#include "PublishTest.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace interop_test {

TestResult PublishTest::runTest(const PublishTestConfig& config) {
    try {
        std::cout << "Starting MOQ publish test for track: "
                  << config.trackNamespace << "/" << config.trackName << std::endl;

        // Establish MOQ session using moq_utils
        auto sessionResult = folly::coro::blockingWait(establishSession(config.serverUrl));
        if (!sessionResult) {
            return TestResult::FAIL;
        }

        std::cout << "MOQ session established successfully" << std::endl;


        // std::cout << "Test subscription created successfully" << std::endl;

        // Send publish request using MOQ session with subscription handle
        auto publishResult = folly::coro::blockingWait(sendPublishRequest(config));
        if (!publishResult) {
            return TestResult::FAIL;
        }

        std::cout << "Publish request completed successfully" << std::endl;

        // Wait for any remaining operations to complete
        // Use a longer wait to ensure all async operations finish
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        std::cout << "Publish test completed successfully" << std::endl;
        return TestResult::PASS;

    } catch (const std::exception& ex) {
        lastError_ = ex.what();
        std::cout << "Publish test failed: " << lastError_ << std::endl;
        return TestResult::ERROR;
    }
}

folly::coro::Task<bool> PublishTest::establishSession(const std::string& serverUrl) {
    try {
        std::cout << "Establishing MOQ session to: " << serverUrl << std::endl;

        // Check if EventBase is still valid
        if (!eventBase_) {
            lastError_ = "EventBase is no longer available";
            co_return false;
        }

        // Use moq_utils to create the session with default stub handlers
        client_ = co_await moq_utils::createMoQSessionWithStubHandlers(
            eventBase_,
            serverUrl
        );

        if (!client_ || !client_->moqSession_) {
            lastError_ = "Failed to establish MOQ session";
            std::cout << "Error: " << lastError_ << std::endl;
            co_return false;
        }

        std::cout << "MOQ session established successfully" << std::endl;
        co_return true;

    } catch (const std::exception& ex) {
        lastError_ = "Session establishment failed: " + std::string(ex.what());
        std::cout << "Detailed session error: " << ex.what() << std::endl;
        co_return false;
    }
}


folly::coro::Task<bool> PublishTest::sendPublishRequest(const PublishTestConfig& config) {
    try {
        if (!client_ || !client_->moqSession_) {
            lastError_ = "No MOQ session available";
            co_return false;
        }

        if (!subscriptionHandle_) {
           subscriptionHandle_ = std::make_shared<TestSubscriptionHandle>();
        }

        auto session = client_->moqSession_;

        // Create publish request
        moxygen::PublishRequest publishReq;
        publishReq.fullTrackName = moxygen::FullTrackName{
            .trackNamespace = moxygen::TrackNamespace(std::vector<std::string>{config.trackNamespace}),
            .trackName = config.trackName
        };
        publishReq.requestID = moxygen::RequestID{1}; // Request ID
        publishReq.trackAlias = moxygen::TrackAlias{1}; // Simple alias
        publishReq.groupOrder = moxygen::GroupOrder::Default;
        publishReq.forward = true;

        std::cout << "Sending publish request for track: "
                  << config.trackNamespace << "/" << config.trackName << std::endl;

        // Send publish request via session's publish method with subscription handle
        auto publishResult = session->publish(publishReq, subscriptionHandle_);

        if (publishResult.hasValue()){
            auto publishConsumerAndReply = std::move(publishResult.value());

            // Start the reply task to handle the PUBLISH_OK or PUBLISH_ERROR
            auto replyTask = std::move(publishConsumerAndReply.reply);
            auto replyResult = co_await std::move(replyTask);

            if (replyResult.hasValue()) {
                auto publishOk = replyResult.value();
                std::cout << "Publish OK received. Request ID: "
                          << publishOk.requestID.value << std::endl;
                co_return true;
            } else {
                auto error = replyResult.error();
                lastError_ = "Publish failed: " + error.reasonPhrase;
                std::cout << "Error: " << lastError_ << std::endl;
                co_return false;
            }

        } else {
            auto error = publishResult.error();
            lastError_ = "Publish request failed: " + error.reasonPhrase;
            std::cout << "Error: " << lastError_ << std::endl;
            co_return false;
        }

        std::cout << "Publish completed successfully using MOQ session" << std::endl;
        co_return true;

    } catch (const std::exception& ex) {
        lastError_ = "Publish request failed: " + std::string(ex.what());
        std::cout << "Exception in sendPublishRequest: " << ex.what() << std::endl;
        co_return false;
    }
}

void PublishTest::cleanup() {
    std::cout << "Starting cleanup..." << std::endl;

    // Clean up subscription handle first
    if (subscriptionHandle_) {
        std::cout << "Cleaning up subscription handle..." << std::endl;
        subscriptionHandle_.reset();
    }

    if (client_) {
        std::cout << "Cleaning up MOQ client..." << std::endl;
        try {
            // Try to gracefully close the client session if possible schedule this on the EventBase to avoid threading issues
            if (eventBase_) {
                eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait([this]() {
                    if (client_ && client_->moqSession_) {
                        // Explicitly close the session with NO_ERROR before resetting
                        try {
                            std::cout << "Closing MoQ session gracefully..." << std::endl;
                            client_->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
                            std::cout << "MoQ session closed" << std::endl;
                        } catch (const std::exception& ex) {
                            std::cout << "Exception during session close: " << ex.what() << std::endl;
                        }

                        // Give the session a moment to clean up after close
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        try {
                            client_->moqSession_.reset();
                        } catch (const std::exception& ex) {
                            std::cout << "Exception during session cleanup: " << ex.what() << std::endl;
                        }
                    }
                    if (client_) {
                        client_.reset();
                    }
                });

                // Additional wait to allow any pending operations to complete
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

        } catch (const std::exception& ex) {
            std::cout << "Exception during client cleanup: " << ex.what() << std::endl;
            // Force reset if graceful cleanup fails
            try {
                client_.reset();
            } catch (...) {
                std::cout << "Force cleanup completed with exceptions" << std::endl;
            }
        }
    }

    eventBase_ = nullptr;

    std::cout << "Cleanup completed" << std::endl;
}} // namespace interop_test