#include "base/base_test.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>

// verify and delete this??

namespace interop_test {
class SubscribeNamespaceErrorTest : public BaseTest {
public:
  explicit SubscribeNamespaceErrorTest(const TestContext &context)
      : BaseTest(context) {}
  ~SubscribeNamespaceErrorTest() override = default;
  std::string getName() const override { return "SubscribeNamespaceErrorTest"; }
  std::string getDescription() const override {
    return "Verifies that subscribing to a non-announced namespace fails "
           "appropriately";
  }
  TestCategory getCategories() const override { 
    return TestCategory::SUBSCRIBER | TestCategory::NAMESPACE | TestCategory::ERROR_HANDLING; 
  }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test/namespace/error"};
};

// REGISTER_TEST(SubscribeNamespaceErrorTest);

TestResult SubscribeNamespaceErrorTest::execute() {
  log("Attempting to subscribe to namespace without announcing: " +
      trackNamespace_);

  // Get subscriber connection from fixture
  auto subscriber = fixture_->getSubscriber();
  assertNotNull(subscriber.get(), "Subscriber interface should not be null");
  assertTrue(subscriber->isConnected(), "Subscriber should be connected");

  // Send subscribe request without announcing first
  log("Sending subscribe namespace request without prior announce...");
  bool subscribeResult = subscriber->subscribe_namespace(trackNamespace_);

  // Verify subscribe failed as expected
  assertFalse(subscribeResult,
              "Subscribe request should fail for non-announced namespace");
  log("Subscribe to non-announced namespace correctly failed");

  return TestResult::PASS;
}

} // namespace interop_test
