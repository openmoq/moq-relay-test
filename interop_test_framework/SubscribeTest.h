#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <folly/Expected.h>
#include <folly/Unit.h>
#include <folly/futures/Future.h>
#include <moxygen/MoQConsumers.h>
#include "base/BaseTest.h"
#include "TestCommon.h"

namespace interop_test {

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
        moxygen::Priority pri) override;

    folly::Expected<folly::Unit, moxygen::MoQPublishError> subscribeDone(
        moxygen::SubscribeDone subDone) override;
};

/**
 * Basic subscribe test - verifies that a client can subscribe to a published track
 */
class SubscribeTest : public BaseTest {
public:
    explicit SubscribeTest(const TestContext& context);
    ~SubscribeTest() override = default;

    // BaseTest interface
    std::string getName() const override { return "SubscribeTest"; }
    std::string getDescription() const override {
        return "Verifies that a client can subscribe to a published track via the relay";
    }
    TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
    TestResult execute() override;

private:
    std::string trackNamespace_{"test"};
    std::string trackName_{"interop-track"};
    std::shared_ptr<TestTrackConsumer> trackConsumer_;
};

} // namespace interop_test