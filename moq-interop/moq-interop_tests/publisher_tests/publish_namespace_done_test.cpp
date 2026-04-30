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
  std::string trackNamespace_{"test/namespace"};
};

REGISTER_TEST(PublishNamespaceDoneTest);

TestResult PublishNamespaceDoneTest::execute() {
  log("Publishing namespace done: " + trackNamespace_);

  // Get publisher connection from fixture
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  // Send publish done request
  log("Sending publish done request...");
  bool publishDoneResult = publisher->publish_namespace_done(trackNamespace_);

  // Verify publish done succeeded
  assertTrue(publishDoneResult,
             "Publish namespace done request should succeed");
  log("Publish namespace done successful");

  return TestResult::PASS;
}

} // namespace interop_test