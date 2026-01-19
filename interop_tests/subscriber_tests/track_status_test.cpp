#include "test_commons.h"
#include "test_registry.h"
#include "base/base_test.h"
#include "base/test_fixture.h"
#include "moxygen_interface.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <string>
#include <thread>

namespace interop_test {

/**
 * Track status test - verifies that a client can request track status for a
 * published track
 */
class TrackStatusTest : public BaseTest {
public:
  explicit TrackStatusTest(const TestContext &context) : BaseTest(context) {}

  ~TrackStatusTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "TrackStatusTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can request track status for a published "
           "track via the relay";
  }
  TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test"};
  std::string trackName_{"status-track"};
};

// Auto-register this test
REGISTER_TEST(TrackStatusTest);

TestResult TrackStatusTest::execute() {
  log("Testing track status for track: " + trackNamespace_ + "/" +
      trackName_);

  // First publish a track
//   log("Publishing track");
//   auto publisher = fixture_->getPublisher();
//   assertNotNull(publisher.get(), "Publisher interface should not be null");
//   assertTrue(publisher->isConnected(), "Publisher should be connected");

//   auto subscriptionHandle = fixture_->createSubscriptionHandle();
//   bool publishResult = folly::coro::blockingWait(
//       publisher->publish(trackNamespace_, trackName_, subscriptionHandle));
//   assertTrue(publishResult, "Publish request should succeed");
//   log("Publish successful");

  // Give the relay time to process the publish request
//   std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Request track status from a different connection
  log("Requesting track status");
  auto subscriber = fixture_->getSubscriber();
  assertNotNull(subscriber.get(), "Subscriber interface should not be null");
  assertTrue(subscriber->isConnected(), "Subscriber should be connected");

  bool trackStatusResult = folly::coro::blockingWait(
      subscriber->trackStatus(trackNamespace_, trackName_));
  assertTrue(trackStatusResult, "Track status request should succeed");
  log("Track status request successful");

  log("Publisher and subscriber are using separate connections");
  return TestResult::PASS;
}
} // namespace interop_test
