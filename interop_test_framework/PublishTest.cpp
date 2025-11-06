#include "PublishTest.h"
#include <iostream>
#include <thread>

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

        // // Create a subscription to get a subscription handle
        // auto subscriptionResult = folly::coro::blockingWait(createSubscription(config));
        // if (!subscriptionResult) {
        //     return TestResult::FAIL;
        // }

        // std::cout << "Test subscription created successfully" << std::endl;

        // Send publish request using MOQ session with subscription handle
        auto publishResult = folly::coro::blockingWait(sendPublishRequest(config));
        if (!publishResult) {
            return TestResult::FAIL;
        }

        std::cout << "Publish request completed successfully" << std::endl;

        // Wait for any remaining operations to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        cleanup();

        std::cout << "Publish test completed successfully" << std::endl;
        return TestResult::PASS;

    } catch (const std::exception& ex) {
        lastError_ = ex.what();
        std::cout << "Publish test failed: " << lastError_ << std::endl;
        cleanup();
        return TestResult::ERROR;
    }
}

folly::coro::Task<bool> PublishTest::establishSession(const std::string& serverUrl) {
    try {
        std::cout << "Establishing MOQ session to: " << serverUrl << std::endl;

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

folly::coro::Task<bool> PublishTest::createSubscription(const PublishTestConfig& config) {
    try {
        if (!client_ || !client_->moqSession_) {
            lastError_ = "No MOQ session available";
            co_return false;
        }

        auto session = client_->moqSession_;

        // Create a subscription request for the same track we want to publish to
        moxygen::FullTrackName fullTrackName{
            .trackNamespace = moxygen::TrackNamespace(std::vector<std::string>{config.trackNamespace}),
            .trackName = config.trackName
        };

        // Use the make method to create SubscribeRequest with proper defaults
        auto subReq = moxygen::SubscribeRequest::make(
            fullTrackName,
            128, // priority
            moxygen::GroupOrder::Default,
            true, // forward
            moxygen::LocationType::LargestGroup,
            folly::none, // start location
            0 // endGroup
        );

        // Set the request ID and track alias
        subReq.requestID = moxygen::RequestID{2}; // Different from publish request ID
        subReq.trackAlias = moxygen::TrackAlias{2};

        std::cout << "Creating test subscription for track: "
                  << config.trackNamespace << "/" << config.trackName << std::endl;

        // Create a test track consumer
        auto trackConsumer = std::make_shared<TestTrackConsumer>();

        // Send subscription request
        auto subscribeResult = co_await session->subscribe(subReq, trackConsumer);

        if (subscribeResult.hasError()) {
            auto error = subscribeResult.error();
            lastError_ = "Subscribe failed: " + error.reasonPhrase;
            std::cout << "Error: " << lastError_ << std::endl;
            co_return false;
        }

        // Store the subscription handle for use in publish
        // subscriptionHandle_ = subscribeResult.value();
        // std::cout << "Test subscription created successfully" << std::endl;

        co_return true;

    } catch (const std::exception& ex) {
        lastError_ = "Subscription creation failed: " + std::string(ex.what());
        std::cout << "Exception in createSubscription: " << ex.what() << std::endl;
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
            auto replyResult = folly::coro::blockingWait(std::move(replyTask).scheduleOn(eventBase_));

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
    // Clean up subscription handle
    if (subscriptionHandle_) {
        try {
            subscriptionHandle_->unsubscribe();
        } catch (const std::exception& ex) {
            std::cout << "Warning: Exception during subscription cleanup: " << ex.what() << std::endl;
        }
        subscriptionHandle_.reset();
    }

    if (client_) {
        // Close session gracefully if available
        if (client_->moqSession_) {
            try {
                client_->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
            } catch (const std::exception& ex) {
                std::cout << "Warning: Exception during session close: " << ex.what() << std::endl;
            }
        }
        client_.reset();
    }

    if (eventBase_) {
        // Process any remaining events
        try {
            eventBase_->loopOnce();
        } catch (const std::exception& ex) {
            std::cout << "Warning: Exception during event base cleanup: " << ex.what() << std::endl;
        }
        // Don't reset eventBase_ since we don't own it
    }
}

} // namespace interop_test