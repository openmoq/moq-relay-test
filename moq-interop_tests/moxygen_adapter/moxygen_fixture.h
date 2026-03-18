#pragma once

#include "base/fixture_interface.h"
#include "base/moqt_interface.h"
#include "moxygen_adapter/moxygen_mocks.h"
#include "moxygen_adapter/moxygen_interface.h"
#include <chrono>
#include <folly/io/async/EventBase.h>
#include <memory>
#include <string>

namespace interop_test {

// Forward declaration
struct TestContext;

/**
 * Test Fixture - Provides common setup/teardown and resource management for
 * tests
 *
 * Purpose:
 * - Eliminates code duplication across tests
 * - Manages MoQ connections (publisher/subscriber)
 * - Provides reusable test resources (subscription handles, track consumers)
 * - Ensures proper cleanup of resources
 *
 * Usage:
 *   auto publisher = fixture_->getPublisher();
 *   auto subscriber = fixture_->getSubscriber();
 *   // Resources are automatically cleaned up
 */
class MoxygenTestFixture : public ITestFixture {
public:
  explicit MoxygenTestFixture(const TestContext &context);
  ~MoxygenTestFixture() = default;

  /**
   * Initialize the fixture (called before test execution)
   */
  void setUp() override;

  /**
   * Cleanup all resources (called after test execution)
   */
  void tearDown() override;

  /**
   * Get or create a publisher connection
   * The connection is established on first call and reused
   * @return Shared pointer to MoqtInterface configured as publisher
   */
  std::shared_ptr<MoqtInterface> getPublisher() override;

  /**
   * Get or create a subscriber connection
   * The connection is established on first call and reused
   * @return Shared pointer to MoqtInterface configured as subscriber
   */
  std::shared_ptr<MoqtInterface> getSubscriber() override;

  /**
   * Create a fresh MoQ interface (not cached)
   * Useful for tests that need multiple connections
   * @param autoConnect If true, automatically connects to relay
   * @return New MoqtInterface instance
   */
  std::shared_ptr<MoqtInterface>
  createMoQInterface(bool autoConnect = false) override;

  /**
   * Get the event base for async operations
   */
  folly::EventBase *getEventBase() const override { return eventBase_; }

  /**
   * Get the relay URL
   */
  const std::string &getRelayUrl() const override { return relayUrl_; }

  /**
   * Get the default timeout
   */
  std::chrono::milliseconds getTimeout() const override { return timeout_; }

  /**
   * Check if a resource is ready
   */
  bool hasPublisher() const override {
    return publisher_ && publisher_->isConnected();
  }
  bool hasSubscriber() const override {
    return subscriber_ && subscriber_->isConnected();
  }

  /**
   * Reset specific resources (for tests that need to reconnect)
   */
  void resetPublisher() override;
  void resetSubscriber() override;

private:
  // Configuration
  folly::EventBase *eventBase_;
  std::string relayUrl_;
  std::chrono::milliseconds timeout_;

  // Managed resources
  std::shared_ptr<MoxygenInterface> publisher_;
  std::shared_ptr<MoxygenInterface> subscriber_;
  std::vector<std::shared_ptr<MoxygenInterface>> additionalInterfaces_;

  // Helper methods
  void cleanupInterface(std::shared_ptr<MoxygenInterface> &interface,
                        const std::string &name);
};

} // namespace interop_test
