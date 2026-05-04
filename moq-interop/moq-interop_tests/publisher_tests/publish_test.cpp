#include "base/base_test.h"
#include "base/fixture_interface.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
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
  TestCategory getCategories() const override { return TestCategory::PUBLISHER; }

protected:
  TestResult execute() override;

private:
  const std::string suffix_{std::to_string(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count())};
  std::string trackNamespace_{"test-" + suffix_};
  std::string trackName_{"interop-track-" + suffix_};
};

// Auto-register this test
REGISTER_TEST(PublishTest);

TestResult PublishTest::execute() {
  logAlways("Publishing track: " + trackNamespace_ + "/" + trackName_);

  // Get publisher connection from fixture
  log("Calling getPublisher()...");
  auto publisher = fixture_->getPublisher();
  log("getPublisher() returned");

  assertNotNull(publisher.get(), "Publisher interface should not be null");
  log("assertNotNull passed");

  assertTrue(publisher->isConnected(), "Publisher should be connected");
  log("isConnected assertion passed");

  // Send publish request (forward=true to start sending data immediately)
  logAlways("Sending publish request...");
  log("About to call publisher->publish()");
  bool publishResult = publisher->publish(trackNamespace_, trackName_, true);
  log("publish() returned: " + std::to_string(publishResult));

  // Verify publish succeeded
  assertTrue(publishResult, "Publish request should succeed");

  logAlways("Publish completed successfully");
  return TestResult::PASS;
}

} // namespace interop_test