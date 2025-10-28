#include "PublishTest.h"
#include <iostream>
#include <thread>
#include <proxygen/lib/utils/URL.h>
#include <folly/io/async/EventBaseManager.h>

namespace interop_test {

TestResult PublishTest::runTest(const PublishTestConfig& config) {
    try {
        std::cout << "Starting MOQ publish test using WebTransport client for track: "
                  << config.trackNamespace << "/" << config.trackName << std::endl;

        // Connect to MOQ server using WebTransport
        auto connectTask = connectToServer(config.serverUrl);
        folly::coro::blockingWait(std::move(connectTask));

        std::cout << "Connected to server successfully" << std::endl;

        // Send publish request using WebTransport client
        auto publishResult = folly::coro::blockingWait(sendPublishRequest(config));
        if (!publishResult) {
            return TestResult::FAIL;
        }

        std::cout << "Publish request sent successfully using WebTransport client" << std::endl;

        // Wait for response with timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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

folly::coro::Task<void> PublishTest::connectToServer(const std::string& serverUrl) {
    try {
        // Create executor for MoQ operations
        executor_ = std::make_shared<moxygen::MoQFollyExecutorImpl>(
            folly::EventBaseManager::get()->getEventBase());

        // Parse URL
        proxygen::URL url(serverUrl);
        if (!url.isValid()) {
            lastError_ = "Invalid server URL: " + serverUrl;
            std::cout << "URL parsing failed for: " << serverUrl << std::endl;
            throw std::runtime_error("Invalid URL: " + serverUrl);
        }

        std::cout << "Parsed URL - Host: " << url.getHost() << ", Port: " << url.getPort() << ", Path: " << url.getPath() << std::endl;

        // Create MoQ WebTransport client
        client_ = std::make_shared<moxygen::MoQWebTransportClient>(executor_, std::move(url));
        if (!client_) {
            lastError_ = "Failed to create MOQ WebTransport client";
            throw std::runtime_error("Failed to create MOQ WebTransport client");
        }

        std::cout << "Created MoQ WebTransport client successfully" << std::endl;
        std::cout << "Attempting to establish MoQ session via WebTransport..." << std::endl;

        // Establish MoQ session using setupMoQSession via WebTransport - this returns Task<void>
        co_await client_->setupMoQSession(
            std::chrono::milliseconds(15000), // longer connect timeout
            std::chrono::milliseconds(15000), // longer transaction timeout
            nullptr, // publish handler
            shared_from_this(), // subscribe handler (we implement Subscriber)
            quic::TransportSettings() // default transport settings
        );

        std::cout << "MoQ session established successfully via WebTransport" << std::endl;

        // Get the session from the client
        session_ = client_->moqSession_;
        if (!session_) {
            lastError_ = "Failed to get MOQ session from client";
            throw std::runtime_error("Failed to get MOQ session from client");
        }

        co_return;

    } catch (const std::exception& ex) {
        lastError_ = "Connection failed: " + std::string(ex.what());
        std::cout << "Detailed connection error: " << ex.what() << std::endl;
        throw; // Re-throw to be caught by runTest
    }
}

folly::coro::Task<bool> PublishTest::sendPublishRequest(const PublishTestConfig& config) {
    try {
        if (!session_) {
            lastError_ = "No session available";
            co_return false;
        }

        // Create publish request
        moxygen::PublishRequest publishReq;
        publishReq.fullTrackName = moxygen::FullTrackName{
            .trackNamespace = moxygen::TrackNamespace(std::vector<std::string>{config.trackNamespace}),
            .trackName = config.trackName
        };
        publishReq.requestID = 1; // Request ID
        publishReq.trackAlias = 1; // Simple alias
        publishReq.groupOrder = moxygen::GroupOrder::Default;

        std::cout << "Sending publish request using WebTransport MoQSession's Subscriber interface..." << std::endl;

        // Send publish request via session's Subscriber interface over WebTransport
        // The session acts as a Subscriber when we want to publish
        auto publishResult = session_->publish(publishReq);

        if (publishResult.hasError()) {
            auto error = publishResult.error();
            lastError_ = "Publish failed: " + error.reasonPhrase;
            co_return false;
        }

        // Get the consumer and reply task (move to avoid copy)
        auto publishData = std::move(publishResult.value());
        auto trackConsumer = publishData.consumer;

        if (!trackConsumer) {
            lastError_ = "Failed to get track consumer";
            co_return false;
        }

        // Send some test data using proper parameters for beginSubgroup
        uint64_t groupID = 0;
        uint64_t subgroupID = 0;
        moxygen::Priority priority = 128;

        auto subgroupResult = trackConsumer->beginSubgroup(groupID, subgroupID, priority);
        if (subgroupResult.hasError()) {
            lastError_ = "Failed to begin subgroup: " + subgroupResult.error().describe();
            co_return false;
        }

        auto subgroupConsumer = subgroupResult.value();
        if (subgroupConsumer) {
            std::string testData = "Hello from MOQ test using WebTransport client!";
            auto objBuf = folly::IOBuf::copyBuffer(testData);

            // Send object with proper parameters
            auto objectResult = subgroupConsumer->object(0, std::move(objBuf));
            if (objectResult.hasError()) {
                lastError_ = "Failed to send object: " + objectResult.error().describe();
                co_return false;
            }

            // End the subgroup
            auto endResult = subgroupConsumer->endOfSubgroup();
            if (endResult.hasError()) {
                lastError_ = "Failed to end subgroup: " + endResult.error().describe();
                co_return false;
            }
        }

        // Wait for the publish response
        auto replyResult = co_await std::move(publishData.reply);
        if (replyResult.hasError()) {
            auto error = replyResult.error();
            lastError_ = "Publish response error: " + error.reasonPhrase;
            co_return false;
        }

        std::cout << "Publish completed successfully using WebTransport client" << std::endl;
        co_return true;

    } catch (const std::exception& ex) {
        lastError_ = "Publish request failed: " + std::string(ex.what());
        co_return false;
    }
}

void PublishTest::cleanup() {
    if (session_) {
        // Session cleanup is automatic via shared_ptr
        session_.reset();
    }
    if (client_) {
        // Client cleanup is automatic via shared_ptr
        client_.reset();
    }
    if (executor_) {
        executor_.reset();
    }
}

} // namespace interop_test