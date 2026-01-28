#include "base/base_test.h"
#include "moxygen_adapter/moxygen_fixture.h"
#include "moxygen_adapter/moxygen_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <string>
#include <thread>

namespace interop_test {

/**
 * Subscribe Update test - verifies that a client can send a subscribe update
 * after subscribing to a track
 */
class SubscribeUpdateErrorTest : public BaseTest {
public:
  explicit SubscribeUpdateErrorTest(const TestContext &context) : BaseTest(context) {}

  ~SubscribeUpdateErrorTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "SubscribeUpdateErrorTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can send an erroneous subscribe update message after "
           "subscribing to a track";
  }
  TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test"};
  std::string trackName_{"interop-track"};
  std::shared_ptr<TestTrackConsumer> trackConsumer_ =
      std::make_shared<TestTrackConsumer>();
};

// Auto-register this test
// This leads to segmentation fault in the test moxygen relay. Will enable after fixing the issue.
// REGISTER_TEST(SubscribeUpdateErrorTest);

TestResult SubscribeUpdateErrorTest::execute() {
  log("Testing subscribe update for track: " + trackNamespace_ + "/" +
      trackName_);

  // First publish a track
  log("Publishing track");
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  bool publishResult = folly::coro::blockingWait(
      publisher->publish(trackNamespace_, trackName_));
  assertTrue(publishResult, "Publish request should succeed");
  log("Publish successful");

  // Give the relay time to process the publish request
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Send subscribe update
  log("Sending subscribe update");
  auto subscriber = fixture_->getSubscriber();
  bool updateResult = folly::coro::blockingWait(subscriber->subscribeUpdate(trackNamespace_, trackName_, trackConsumer_, 260, moxygen::GroupOrder::NewestFirst));
  assertFalse(updateResult, "Subscribe update request should fail with invalid priority");

  return TestResult::PASS;
}
} // namespace interop_test
