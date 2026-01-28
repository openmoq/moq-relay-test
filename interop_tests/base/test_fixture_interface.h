#pragma once

#include <chrono>
#include <memory>
#include <string>

// Forward declarations to avoid pulling in dependencies
namespace folly {
class EventBase;
}

namespace interop_test {

// Forward declarations
class MoqtInterface;
class TestSubscriptionHandle;

/**
 * Abstract interface for test fixtures
 * 
 * This interface allows BaseTest to be independent of specific fixture
 * implementations (e.g., moxygen-based fixtures). Different MoQ
 * implementations can provide their own fixture implementations.
 */
class ITestFixture {
public:
  virtual ~ITestFixture() = default;

  /**
   * Initialize the fixture (called before test execution)
   */
  virtual void setUp() = 0;

  /**
   * Cleanup all resources (called after test execution)
   */
  virtual void tearDown() = 0;

  /**
   * Get or create a publisher connection
   * @return Shared pointer to MoQ interface configured as publisher
   */
  virtual std::shared_ptr<MoqtInterface> getPublisher() = 0;

  /**
   * Get or create a subscriber connection
   * @return Shared pointer to MoQ interface configured as subscriber
   */
  virtual std::shared_ptr<MoqtInterface> getSubscriber() = 0;

  /**
   * Create a fresh MoQ interface (not cached)
   * @param autoConnect If true, automatically connects to relay
   * @return New MoqtInterface instance
   */
  virtual std::shared_ptr<MoqtInterface>
  createMoQInterface(bool autoConnect = false) = 0;

  /**
   * Get the event base for async operations
   */
  virtual folly::EventBase *getEventBase() const = 0;

  /**
   * Get the relay URL
   */
  virtual const std::string &getRelayUrl() const = 0;

  /**
   * Get the default timeout
   */
  virtual std::chrono::milliseconds getTimeout() const = 0;

  /**
   * Check if a resource is ready
   */
  virtual bool hasPublisher() const = 0;
  virtual bool hasSubscriber() const = 0;

  /**
   * Reset specific resources (for tests that need to reconnect)
   */
  virtual void resetPublisher() = 0;
  virtual void resetSubscriber() = 0;
};

} // namespace interop_test
