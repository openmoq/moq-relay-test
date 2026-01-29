#include "base/base_test.h"
#include "base/fixture_interface.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <folly/coro/BlockingWait.h>
#include <memory>
#include <string>

namespace interop_test {

/**
 * Goaway test - verifies that a client can successfully send a goaway signal
 * after publishing a track to the relay
 */
class GoawayTest : public BaseTest {
public:
  explicit GoawayTest(const TestContext &context) : BaseTest(context) {}
  ~GoawayTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "GoawayTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can successfully send a goaway signal "
           "after publishing a track to the relay";
  }
  TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
  TestResult execute() override;
};

// Auto-register this test
// REGISTER_TEST(GoawayTest);

TestResult GoawayTest::execute() {
  log("Executing goaway sequence");

  // Get publisher connection from fixture
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  // Execute goaway sequence (publishes a dummy track and sends goaway)
  log("Sending goaway sequence...");
  bool goawayResult = folly::coro::blockingWait(publisher->goaway_sequence());

  // Verify goaway sequence succeeded
  assertTrue(goawayResult, "Goaway sequence should succeed");

  log("Goaway sequence completed successfully");
  return TestResult::PASS;
}

} // namespace interop_test
