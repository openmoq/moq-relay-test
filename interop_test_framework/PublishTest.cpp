#include "PublishTest.h"
#include "base/TestFixture.h"
#include "moq_interface.h"
#include "TestRegistry.h"
#include <folly/coro/BlockingWait.h>

namespace interop_test {

// Auto-register this test
REGISTER_TEST(PublishTest);

PublishTest::PublishTest(const TestContext& context)
    : BaseTest(context) {
}

TestResult PublishTest::execute() {
    log("Publishing track: " + trackNamespace_ + "/" + trackName_);

    // Get publisher connection from fixture
    auto publisher = fixture_->getPublisher();
    assertNotNull(publisher.get(), "Publisher interface should not be null");
    assertTrue(publisher->isConnected(), "Publisher should be connected");

    // Create subscription handle
    auto subscriptionHandle = fixture_->createSubscriptionHandle();
    assertNotNull(subscriptionHandle.get(), "Subscription handle should not be null");

    // Send publish request
    log("Sending publish request...");
    bool publishResult = folly::coro::blockingWait(publisher->publish(
        trackNamespace_,
        trackName_,
        subscriptionHandle
    ));

    // Verify publish succeeded
    assertTrue(publishResult, "Publish request should succeed");

    log("Publish completed successfully");
    return TestResult::PASS;
}

} // namespace interop_test