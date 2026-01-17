#include "test_commons.h"
#include "test_registry.h"
#include "base/base_test.h"
#include "base/test_fixture.h"
#include "moq_interface.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <string>
#include <thread>

namespace interop_test {

class SubscribeErrorTest : public BaseTest {
public:
  explicit SubscribeErrorTest(const TestContext &context) : BaseTest(context) {}
  ~SubscribeErrorTest() override = default;

  std::string getName() const override { return "SubscribeErrorTest"; }
  std::string getDescription() const override {
    return "Verifies that a client receives an error when subscribing to a non-existent track";
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

REGISTER_TEST(SubscribeErrorTest);

TestResult SubscribeErrorTest::execute() {
  log("Testing subscribe error for non-existent track: " + trackNamespace_ + "/" +
      trackName_);

  // Attempt to subscribe to a non-existent track
  log("Subscribing to non-existent track");
  auto subscriber = fixture_->getSubscriber();
  assertNotNull(subscriber.get(), "Subscriber interface should not be null");
  assertTrue(subscriber->isConnected(), "Subscriber should be connected");

  bool subscribeResult = folly::coro::blockingWait(
      subscriber->subscribe(trackNamespace_, trackName_, trackConsumer_));
  
  // Verify that the subscribe resulted in an error
  assertFalse(subscribeResult, "Subscribe request should fail for non-existent track");
  log("Subscribe error received as expected");

  return TestResult::PASS;
}
} // namespace interop_test