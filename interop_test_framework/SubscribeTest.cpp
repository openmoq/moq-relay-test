#include "SubscribeTest.h"
#include <chrono>
#include <thread>
#include <folly/coro/BlockingWait.h>

namespace interop_test {

// TestTrackConsumer implementation

folly::Expected<folly::Unit, moxygen::MoQPublishError> 
TestTrackConsumer::setTrackAlias(moxygen::TrackAlias alias) {
    std::cout << "TestTrackConsumer::setTrackAlias: " << alias.value << std::endl;
    return folly::unit;
}

folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>, moxygen::MoQPublishError>
TestTrackConsumer::beginSubgroup(uint64_t groupID, uint64_t subgroupID, moxygen::Priority priority) {
    std::cout << "TestTrackConsumer::beginSubgroup - Group: " << groupID 
              << ", Subgroup: " << subgroupID << std::endl;
    return folly::makeUnexpected(
        moxygen::MoQPublishError(moxygen::MoQPublishError::API_ERROR, "not implemented"));
}

folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
TestTrackConsumer::awaitStreamCredit() {
    return folly::makeSemiFuture(folly::unit);
}

folly::Expected<folly::Unit, moxygen::MoQPublishError> 
TestTrackConsumer::objectStream(
    const moxygen::ObjectHeader& header,
    moxygen::Payload payload) {
    std::cout << "TestTrackConsumer::objectStream - Group: " << header.group 
              << ", Object: " << header.id << std::endl;
    return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError> 
TestTrackConsumer::datagram(
    const moxygen::ObjectHeader& header,
    moxygen::Payload payload) {
    std::cout << "TestTrackConsumer::datagram - Group: " << header.group 
              << ", Object: " << header.id << std::endl;
    return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError> 
TestTrackConsumer::groupNotExists(
    uint64_t groupID,
    uint64_t subgroup,
    moxygen::Priority pri,
    moxygen::Extensions extensions) {
    std::cout << "TestTrackConsumer::groupNotExists - Group: " << groupID << std::endl;
    return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError> 
TestTrackConsumer::subscribeDone(moxygen::SubscribeDone subDone) {
    std::cout << "TestTrackConsumer::subscribeDone" << std::endl;
    return folly::unit;
}

// SubscribeTest implementation

TestResult SubscribeTest::runTest(const SubscribeTestConfig& config) {
    try {
        std::cout << "Starting MOQ subscribe test for track: "
                  << config.trackNamespace << "/" << config.trackName << std::endl;

        // Step 1: Create publisher connection and send publish request
        std::cout << "\n=== Step 1: Establishing publisher connection ===" << std::endl;
        publisherInterface_ = std::make_shared<moq_interface::MoQInterface>(eventBase_);

        bool publisherConnected = folly::coro::blockingWait(
            publisherInterface_->connect(config.serverUrl));

        if (!publisherConnected) {
            lastError_ = "Failed to establish publisher MoQ session";
            return TestResult::ERROR;
        }

        // Create subscription handle for publisher
        if (!subscriptionHandle_) {
            subscriptionHandle_ = std::make_shared<TestSubscriptionHandle>();
        }

        // Send publish request
        std::cout << "Sending publish request..." << std::endl;
        bool publishResult = folly::coro::blockingWait(
            publisherInterface_->publish(
                config.trackNamespace,
                config.trackName,
                subscriptionHandle_
            )
        );

        if (!publishResult) {
            lastError_ = "Publish request failed";
            return TestResult::FAIL;
        }

        std::cout << "Publish request successful" << std::endl;

        // Give the relay time to process the publish request
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Step 2: Create subscriber connection and subscribe
        std::cout << "\n=== Step 2: Establishing subscriber connection ===" << std::endl;
        subscriberInterface_ = std::make_shared<moq_interface::MoQInterface>(eventBase_);

        bool subscriberConnected = folly::coro::blockingWait(
            subscriberInterface_->connect(config.serverUrl));

        if (!subscriberConnected) {
            lastError_ = "Failed to establish subscriber MoQ session";
            return TestResult::ERROR;
        }

        // Create track consumer
        if (!trackConsumer_) {
            trackConsumer_ = std::make_shared<TestTrackConsumer>();
        }

        // Subscribe to the track
        std::cout << "Subscribing to track..." << std::endl;
        bool subscribed = folly::coro::blockingWait(
            subscriberInterface_->subscribe(
                config.trackNamespace,
                config.trackName,
                trackConsumer_
            )
        );

        if (!subscribed) {
            lastError_ = "Failed to subscribe to track";
            return TestResult::FAIL;
        }

        std::cout << "\nSubscribe test completed successfully" << std::endl;
        std::cout << "Publisher and subscriber are using separate connections" << std::endl;
        return TestResult::PASS;

    } catch (const std::exception& ex) {
        lastError_ = ex.what();
        std::cout << "Subscribe test failed: " << lastError_ << std::endl;
        return TestResult::ERROR;
    }
}

void SubscribeTest::cleanup() {
    std::cout << "Starting cleanup..." << std::endl;

    // Clean up track consumer first
    if (trackConsumer_) {
        std::cout << "Cleaning up track consumer..." << std::endl;
        trackConsumer_.reset();
    }

    // Clean up subscription handle
    if (subscriptionHandle_) {
        std::cout << "Cleaning up subscription handle..." << std::endl;
        subscriptionHandle_.reset();
    }

    // Clean up subscriber interface
    if (subscriberInterface_) {
        std::cout << "Cleaning up subscriber MoQ interface..." << std::endl;
        try {
            auto client = subscriberInterface_->getClient();
            if (eventBase_ && client && client->moqSession_) {
                eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait([client]() {
                    try {
                        std::cout << "Closing subscriber MoQ session gracefully..." << std::endl;
                        client->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
                        std::cout << "Subscriber MoQ session closed" << std::endl;
                    } catch (const std::exception& ex) {
                        std::cout << "Exception during subscriber session close: " << ex.what() << std::endl;
                    }
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            subscriberInterface_.reset();
        } catch (const std::exception& ex) {
            std::cout << "Exception during subscriber cleanup: " << ex.what() << std::endl;
        }
    }

    // Clean up publisher interface
    if (publisherInterface_) {
        std::cout << "Cleaning up publisher MoQ interface..." << std::endl;
        try {
            auto client = publisherInterface_->getClient();
            if (eventBase_ && client && client->moqSession_) {
                eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait([client]() {
                    try {
                        std::cout << "Closing publisher MoQ session gracefully..." << std::endl;
                        client->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
                        std::cout << "Publisher MoQ session closed" << std::endl;
                    } catch (const std::exception& ex) {
                        std::cout << "Exception during publisher session close: " << ex.what() << std::endl;
                    }
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            publisherInterface_.reset();
        } catch (const std::exception& ex) {
            std::cout << "Exception during publisher cleanup: " << ex.what() << std::endl;
        }
    }

    eventBase_ = nullptr;
    std::cout << "Cleanup completed" << std::endl;
}
} // namespace interop_test