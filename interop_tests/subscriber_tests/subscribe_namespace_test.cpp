#include "test_commons.h"
#include "test_registry.h"
#include "base/base_test.h"
#include "base/test_fixture.h"
#include "moxygen_interface.h"
#include <folly/coro/BlockingWait.h>

namespace interop_test {
class SubscribeNamespaceTest : public BaseTest {
public:
    explicit SubscribeNamespaceTest(const TestContext &context) : BaseTest(context) {}
    ~SubscribeNamespaceTest() override = default;
    std::string getName() const override { return "SubscribeNamespaceTest"; }
    std::string getDescription() const override {
        return "Verifies that a client can subscribe to a namespace via the relay";
    }
    TestCategory getCategory() const override { return TestCategory::ALL; }
protected:
    TestResult execute() override;
private:    
    std::string trackNamespace_{"test1/namespace"};
};

REGISTER_TEST(SubscribeNamespaceTest);

TestResult SubscribeNamespaceTest::execute() {
    log("Subscribing to namespace: " + trackNamespace_);

    // First publish a track in the namespace to ensure it exists
    // log("Publishing track in namespace");
    // auto publisher = fixture_->getPublisher();
    // assertNotNull(publisher.get(), "Publisher interface should not be null");
    // assertTrue(publisher->isConnected(), "Publisher should be connected");
    // std::string trackName = "interop-track";
    // auto subscriptionHandle = fixture_->createSubscriptionHandle();
    // bool announceResult = folly::coro::blockingWait(
    //   publisher->announce(trackNamespace_));
    // assertTrue(announceResult, "Announce request should succeed");
    // log("Announce successful");

    // Get subscriber connection from fixture
    auto subscriber = fixture_->getSubscriber();
    assertNotNull(subscriber.get(), "Subscriber interface should not be null");
    assertTrue(subscriber->isConnected(), "Subscriber should be connected");

    // Create subscription handle
    auto trackConsumer = std::make_shared<TestTrackConsumer>();
    assertNotNull(trackConsumer.get(), "Track consumer should not be null");

    // Send subscribe request
    log("Sending subscribe namespace request...");
    bool subscribeResult = folly::coro::blockingWait(
        subscriber->subscribeAnnounces(trackNamespace_));

    // Verify subscribe succeeded
    assertTrue(subscribeResult, "Subscribe request should succeed");
    log("Subscribe to namespace successful");

    return TestResult::PASS;
}

} // namespace interop_test