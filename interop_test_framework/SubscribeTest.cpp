#include "SubscribeTest.h"
#include "base/TestFixture.h"
#include "moq_interface.h"
#include "TestRegistry.h"
#include <thread>
#include <folly/coro/BlockingWait.h>

namespace interop_test {

// Auto-register this test
REGISTER_TEST(SubscribeTest);

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
    moxygen::Priority pri) {
    std::cout << "TestTrackConsumer::groupNotExists - Group: " << groupID << std::endl;
    return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError> 
TestTrackConsumer::subscribeDone(moxygen::SubscribeDone subDone) {
    std::cout << "TestTrackConsumer::subscribeDone" << std::endl;
    return folly::unit;
}

// SubscribeTest implementation

SubscribeTest::SubscribeTest(const TestContext& context)
    : BaseTest(context)
    , trackConsumer_(std::make_shared<TestTrackConsumer>()) {
}

TestResult SubscribeTest::execute() {
    log("Testing publish and subscribe for track: " + trackNamespace_ + "/" + trackName_);

    // Step 1: Publish a track
    log("Step 1: Publishing track");
    auto publisher = fixture_->getPublisher();
    assertNotNull(publisher.get(), "Publisher interface should not be null");
    assertTrue(publisher->isConnected(), "Publisher should be connected");

    auto subscriptionHandle = fixture_->createSubscriptionHandle();
    bool publishResult = folly::coro::blockingWait(publisher->publish(
        trackNamespace_,
        trackName_,
        subscriptionHandle
    ));
    assertTrue(publishResult, "Publish request should succeed");
    log("Publish successful");

    // Give the relay time to process the publish request
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Step 2: Subscribe to the track from a different connection
    log("Step 2: Subscribing to track");
    auto subscriber = fixture_->getSubscriber();
    assertNotNull(subscriber.get(), "Subscriber interface should not be null");
    assertTrue(subscriber->isConnected(), "Subscriber should be connected");
    assertNotNull(trackConsumer_.get(), "Track consumer should not be null");

    bool subscribed = folly::coro::blockingWait(subscriber->subscribe(
        trackNamespace_,
        trackName_,
        trackConsumer_
    ));
    assertTrue(subscribed, "Subscribe request should succeed");
    log("Subscribe successful");

    log("Publisher and subscriber are using separate connections");
    return TestResult::PASS;
}
} // namespace interop_test