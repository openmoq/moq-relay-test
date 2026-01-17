#include "base/base_test.h"
#include "test_commons.h"
#include "test_registry.h"
#include "base/test_fixture.h"
#include "moq_interface.h"
#include <folly/coro/BlockingWait.h>

namespace interop_test {

class PublishNamespaceTest : public BaseTest {
public:
  explicit PublishNamespaceTest(const TestContext &context) : BaseTest(context) {}
  ~PublishNamespaceTest() override = default;

  std::string getName() const override { return "PublishNamespaceTest"; }
  std::string getDescription() const override {
    return "Verifies that a publisher can announce a namespace to the relay";
  }
  TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test/namespace"};
};

REGISTER_TEST(PublishNamespaceTest);

TestResult PublishNamespaceTest::execute() {
  log("Publishing namespace: " + trackNamespace_);

  // Get publisher connection from fixture
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  // Send announce request
  log("Sending announce request...");
  bool announceResult = folly::coro::blockingWait(
      publisher->announce(trackNamespace_));

  // Verify announce succeeded
  assertTrue(announceResult, "Announce request should succeed");
  log("Announce successful");

  return TestResult::PASS;
}

} // namespace interop_test