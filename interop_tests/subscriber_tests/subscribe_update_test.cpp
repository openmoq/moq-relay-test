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
 * Subscribe Update test - verifies that a client can send a subscribe update
 * after subscribing to a track
 */
class SubscribeUpdateTest : public BaseTest {
public:
  explicit SubscribeUpdateTest(const TestContext &context) : BaseTest(context) {}

  ~SubscribeUpdateTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "SubscribeUpdateTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can send a subscribe update message after "
           "subscribing to a track";
  }
  TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test"};
  std::string trackName_{"interop-track"};
};

// Auto-register this test
REGISTER_TEST(SubscribeUpdateTest);

TestResult SubscribeUpdateTest::execute() {
  log("Testing subscribe update for track: " + trackNamespace_ + "/" +
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

  // Send subscribe update
  log("Sending subscribe update");
  auto subscriber = fixture_->getSubscriber();
  bool updateResult = subscriber->subscribeUpdate(trackNamespace_, trackName_);
  assertTrue(updateResult, "Subscribe update request should succeed");
  log("Subscribe update successful");

  return TestResult::PASS;
}
} // namespace interop_test
