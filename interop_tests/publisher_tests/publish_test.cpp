#include "test_commons.h"
#include "test_registry.h"
#include "base/base_test.h"
#include "base/test_fixture.h"
#include "moq_interface.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <string>

namespace interop_test {

/**
 * Basic publish test - verifies that a client can successfully publish a track
 * to the relay
 */
class PublishTest : public BaseTest {
public:
  explicit PublishTest(const TestContext &context) : BaseTest(context) {}
  ~PublishTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "PublishTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can successfully publish a track to the "
           "relay";
  }
  TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test"};
  std::string trackName_{"interop-track"};
};

// Auto-register this test
REGISTER_TEST(PublishTest);

TestResult PublishTest::execute() {
  log("Publishing track: " + trackNamespace_ + "/" + trackName_);

  // Get publisher connection from fixture
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  // Create subscription handle
  auto subscriptionHandle = fixture_->createSubscriptionHandle();
  assertNotNull(subscriptionHandle.get(),
                "Subscription handle should not be null");

  // Send publish request
  log("Sending publish request...");
  bool publishResult = folly::coro::blockingWait(
      publisher->publish(trackNamespace_, trackName_, subscriptionHandle));

  // Verify publish succeeded
  assertTrue(publishResult, "Publish request should succeed");

  log("Publish completed successfully");
  return TestResult::PASS;
}

} // namespace interop_test