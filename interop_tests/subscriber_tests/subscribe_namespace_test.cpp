#include "base/base_test.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>

namespace interop_test {
class SubscribeNamespaceTest : public BaseTest {
public:
  explicit SubscribeNamespaceTest(const TestContext &context)
      : BaseTest(context) {}
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

  // Get subscriber connection from fixture
  auto subscriber = fixture_->getSubscriber();
  assertNotNull(subscriber.get(), "Subscriber interface should not be null");
  assertTrue(subscriber->isConnected(), "Subscriber should be connected");

  // Send subscribe request
  log("Sending subscribe namespace request...");
  bool subscribeResult = folly::coro::blockingWait(
      subscriber->subscribe_namespace(trackNamespace_));

  // Verify subscribe succeeded
  assertTrue(subscribeResult, "Subscribe request should succeed");
  log("Subscribe to namespace successful");

  return TestResult::PASS;
}

} // namespace interop_test