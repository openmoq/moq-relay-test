#include "base/base_test.h"
#include "base/moqt_interface.h"
#include "test_registry.h"
#include <memory>
#include <string>
#include <thread>

namespace interop_test {

/**
 * Basic fetch test - verifies that a client can fetch a published track
 */
class FetchTest : public BaseTest {
public:
  explicit FetchTest(const TestContext &context) : BaseTest(context) {}

  ~FetchTest() override = default;

  // BaseTest interface
  std::string getName() const override { return "FetchTest"; }
  std::string getDescription() const override {
    return "Verifies that a client can fetch a published track via the relay";
  }
  TestCategory getCategories() const override { return TestCategory::SUBSCRIBER; }

protected:
  TestResult execute() override;

private:
  std::string trackNamespace_{"test"};
  std::string trackName_{"fetch-track"};
};

// Auto-register this test
REGISTER_TEST(FetchTest);

TestResult FetchTest::execute() {
  log("Testing publish, subscribe, and fetch for track: " + trackNamespace_ + "/" +
      trackName_);

  // First publish a track
  log("Publishing track");
  auto publisher = fixture_->getPublisher();
  assertNotNull(publisher.get(), "Publisher interface should not be null");
  assertTrue(publisher->isConnected(), "Publisher should be connected");

  bool publishResult = publisher->publish(trackNamespace_, trackName_);
  assertTrue(publishResult, "Publish request should succeed");
  log("Publish successful");

  // Give the relay time to process the publish request
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  auto subscriber = fixture_->getSubscriber();
  assertNotNull(subscriber.get(), "Subscriber interface should not be null");
  assertTrue(subscriber->isConnected(), "Subscriber should be connected");

  // Now fetch historical data for the subscribed track
  log("Fetching track data");
  bool fetchResult = subscriber->fetch(trackNamespace_, trackName_);
  assertTrue(fetchResult, "Fetch request should succeed");
  log("Fetch successful");

  log("Publisher and subscriber are using separate connections");
  return TestResult::PASS;
}

} // namespace interop_test
