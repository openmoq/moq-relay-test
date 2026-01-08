#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <folly/Expected.h>
#include <folly/Unit.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <moxygen/MoQConsumers.h>
#include "moq_interface.h"
#include "TestCommon.h"

namespace interop_test {

struct SubscribeTestConfig {
    std::string trackNamespace{"test"};
    std::string trackName{"test-track"};
    std::chrono::milliseconds timeout{5000};
    std::string serverUrl{"https://localhost:4433/moq"};
};

// Simple TrackConsumer implementation for testing
class TestTrackConsumer : public moxygen::TrackConsumer {
public:
    TestTrackConsumer() = default;
    ~TestTrackConsumer() override = default;

    // TrackConsumer interface implementation
    folly::Expected<folly::Unit, moxygen::MoQPublishError> setTrackAlias(
        moxygen::TrackAlias alias) override;

    folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>, moxygen::MoQPublishError>
    beginSubgroup(uint64_t groupID, uint64_t subgroupID, moxygen::Priority priority) override;

    folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
    awaitStreamCredit() override;

    folly::Expected<folly::Unit, moxygen::MoQPublishError> objectStream(
        const moxygen::ObjectHeader& header,
        moxygen::Payload payload) override;

    folly::Expected<folly::Unit, moxygen::MoQPublishError> datagram(
        const moxygen::ObjectHeader& header,
        moxygen::Payload payload) override;

    folly::Expected<folly::Unit, moxygen::MoQPublishError> groupNotExists(
        uint64_t groupID,
        uint64_t subgroup,
        moxygen::Priority pri,
        moxygen::Extensions extensions) override;

    folly::Expected<folly::Unit, moxygen::MoQPublishError> subscribeDone(
        moxygen::SubscribeDone subDone) override;
};

class SubscribeTest {
public:
    SubscribeTest(folly::EventBase* eventBase) : eventBase_(eventBase) {}

    // Destructor to ensure proper cleanup
    ~SubscribeTest() {
        cleanup();
    }

    TestResult runTest(const SubscribeTestConfig& config);
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;
    std::shared_ptr<moq_interface::MoQInterface> publisherInterface_;
    std::shared_ptr<moq_interface::MoQInterface> subscriberInterface_;
    std::shared_ptr<TestSubscriptionHandle> subscriptionHandle_;
    std::shared_ptr<TestTrackConsumer> trackConsumer_;
    folly::EventBase* eventBase_;

    void cleanup();
};
}