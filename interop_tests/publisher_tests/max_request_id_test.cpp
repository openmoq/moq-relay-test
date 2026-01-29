#include "base/base_test.h"
#include "base/fixture_interface.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <string>

namespace interop_test {

/**
 * MaxRequestId test - verifies that a client can successfully
 * set the maximum concurrent requests on a MoQ session
 */
class MaxRequestIdTest : public BaseTest {
public:
  explicit MaxRequestIdTest(const TestContext &context) : BaseTest(context) {}
  ~MaxRequestIdTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "MaxRequestIdTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can successfully set the maximum "
           "concurrent requests on a MoQ session";
  }
  TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
  TestResult execute() override;

private:
  uint32_t maxConcurrentRequests_{10};
};

// Auto-register this test
REGISTER_TEST(MaxRequestIdTest);

TestResult MaxRequestIdTest::execute() {
  log("Setting max concurrent requests to: " +
      std::to_string(maxConcurrentRequests_));

  // Get publisher connection from fixture
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  // Set max concurrent requests
  log("Calling setMaxConcurrentRequests...");
  bool result = publisher->setMaxConcurrentRequests(maxConcurrentRequests_);

  // Verify the call succeeded
  assertTrue(result, "setMaxConcurrentRequests should succeed");

  log("setMaxConcurrentRequests completed successfully");
  return TestResult::PASS;
}

} // namespace interop_test
