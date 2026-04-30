#include "base/base_test.h"
#include "base/fixture_interface.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>

namespace interop_test {
class PublishNamespaceDoneTest : public BaseTest {
public:
  explicit PublishNamespaceDoneTest(const TestContext &context)
      : BaseTest(context) {}
  ~PublishNamespaceDoneTest() override = default;

  std::string getName() const override { return "PublishNamespaceDoneTest"; }
  std::string getDescription() const override {
    return "Verifies that a publisher can signal publish done for a namespace "
           "to the relay";
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

REGISTER_TEST(PublishNamespaceDoneTest);

TestResult PublishNamespaceDoneTest::execute() {
  log("Publishing namespace then done: " + trackNamespace_);

  // Get publisher connection from fixture
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  // Must announce the namespace before sending done
  log("Sending announce request...");
  bool announceResult = publisher->publishNamespace(trackNamespace_);
  assertTrue(announceResult, "Publish namespace request should succeed");
  log("Announce successful");

  // Now signal that the namespace announcement is complete
  log("Sending publish namespace done request...");
  bool publishDoneResult = publisher->publishNamespaceDone(trackNamespace_);
  assertTrue(publishDoneResult,
             "Publish namespace done request should succeed");
  log("Publish namespace done successful");

  return TestResult::PASS;
}

} // namespace interop_test