#include "base/base_test.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <string>
#include <thread>

namespace interop_test {

/**
 * Basic subscribe test - verifies that a client can subscribe to a published
 * track
 */
class SubscribeTest : public BaseTest {
public:
  explicit SubscribeTest(const TestContext &context) : BaseTest(context) {}

  ~SubscribeTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "SubscribeTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can subscribe to a published track via the "
           "relay";
  }
  TestCategory getCategories() const override { return TestCategory::SUBSCRIBER; }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test"};
  std::string trackName_{"interop-track"};
};

// Auto-register this test
REGISTER_TEST(SubscribeTest);

TestResult SubscribeTest::execute() {
  log("Testing publish and subscribe for track: " + trackNamespace_ + "/" +
      trackName_);

  // First publish a track
  log("Publishing track");
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  bool publishResult = publisher->publish(trackNamespace_, trackName_);
  assertTrue(publishResult, "Publish request should succeed");
  log("Publish successful");

  // Give the relay time to process the publish request
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Subscribe to the track from a different connection
  log("Subscribing to track");
  auto subscriber = fixture_->getSubscriber();
  assertNotNull(subscriber.get(), "Subscriber interface should not be null");
  assertTrue(subscriber->isConnected(), "Subscriber should be connected");

  bool subscribed = subscriber->subscribe(trackNamespace_, trackName_);
  assertTrue(subscribed, "Subscribe request should succeed");
  log("Subscribe successful");

  log("Publisher and subscriber are using separate connections");
  return TestResult::PASS;
}
} // namespace interop_test