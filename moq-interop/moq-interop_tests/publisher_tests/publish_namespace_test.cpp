#include "base/base_test.h"
#include "base/fixture_interface.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>

namespace interop_test {

class PublishNamespaceTest : public BaseTest {
public:
  explicit PublishNamespaceTest(const TestContext &context)
      : BaseTest(context) {}
  ~PublishNamespaceTest() override = default;

  std::string getName() const override { return "PublishNamespaceTest"; }
  std::string getDescription() const override {
    return "Verifies that a publisher can announce a namespace to the relay";
  }
  TestCategory getCategories() const override { 
    return TestCategory::PUBLISHER | TestCategory::NAMESPACE; 
  }

protected:
  TestResult execute() override;

private:
  const std::string suffix_{std::to_string(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count())};
  std::string trackNamespace_{"test-namespace-" + suffix_};
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
  bool announceResult = publisher->publishNamespace(trackNamespace_);

  // Verify announce succeeded
  assertTrue(announceResult, "Announce request should succeed");
  log("Announce successful");

  return TestResult::PASS;
}

} // namespace interop_test