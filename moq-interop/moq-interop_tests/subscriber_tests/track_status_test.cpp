#include "base/base_test.h"
#include "base/fixture_interface.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <string>

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
  TestCategory getCategories() const override { return TestCategory::SUBSCRIBER; }

protected:
  TestResult execute() override;

private:
  const std::string suffix_{std::to_string(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count())};
  std::string trackNamespace_{"test-" + suffix_};
  std::string trackName_{"status-track-" + suffix_};
};

// Auto-register this test
REGISTER_TEST(TrackStatusTest);

TestResult TrackStatusTest::execute() {
  log("Testing track status for track: " + trackNamespace_ + "/" + trackName_);

  // First publish a track
  log("Publishing track");
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  // Announce the namespace so the relay can route subscriptions
  bool nsResult = publisher->publishNamespace(trackNamespace_);
  assertTrue(nsResult, "PublishNamespace request should succeed");
  log("Namespace announced");

  // Publish the track (forward=false: relay will call us back on subscribe)
  bool publishResult = publisher->publish(trackNamespace_, trackName_, false);
  assertTrue(publishResult, "Publish request should succeed");
  log("Publish successful");

  // Subscribe from a different connection to create an active forwarding
  // subscription in the relay (required for relay to answer trackStatus
  // from local state)
  auto subscriber = fixture_->getSubscriber();
  assertNotNull(subscriber.get(), "Subscriber interface should not be null");
  assertTrue(subscriber->isConnected(), "Subscriber should be connected");

  bool subscribeResult = subscriber->subscribe(trackNamespace_, trackName_);
  assertTrue(subscribeResult, "Subscribe request should succeed");
  log("Subscribe successful - relay now has active forwarding subscription");

  // Small delay to allow relay to establish forwarding state
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Request track status from the subscriber connection
  log("Requesting track status");
  bool trackStatusResult = subscriber->trackStatus(trackNamespace_, trackName_);
  assertTrue(trackStatusResult, "Track status request should succeed");
  log("Track status request successful");

  log("Publisher and subscriber are using separate connections");
  return TestResult::PASS;
}
} // namespace interop_test
