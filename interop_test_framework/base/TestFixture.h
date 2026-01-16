#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <folly/io/async/EventBase.h>
#include "../moq_interface.h"
#include "../TestCommon.h"

namespace interop_test {

// Forward declaration
struct TestContext;

/**
 * Test Fixture - Provides common setup/teardown and resource management for tests
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
class TestFixture {
public:
    explicit TestFixture(const TestContext& context);
    ~TestFixture() = default;

    /**
     * Initialize the fixture (called before test execution)
     */
    void setUp();

    /**
     * Cleanup all resources (called after test execution)
     */
    void tearDown();

    /**
     * Get or create a publisher connection
     * The connection is established on first call and reused
     * @return Shared pointer to MoQInterface configured as publisher
     */
    std::shared_ptr<moq_interface::MoQInterface> getPublisher();

    /**
     * Get or create a subscriber connection
     * The connection is established on first call and reused
     * @return Shared pointer to MoQInterface configured as subscriber
     */
    std::shared_ptr<moq_interface::MoQInterface> getSubscriber();

    /**
     * Create a fresh MoQ interface (not cached)
     * Useful for tests that need multiple connections
     * @param autoConnect If true, automatically connects to relay
     * @return New MoQInterface instance
     */
    std::shared_ptr<moq_interface::MoQInterface> createMoQInterface(bool autoConnect = false);

    /**
     * Create a test subscription handle
     * @return New TestSubscriptionHandle instance
     */
    std::shared_ptr<TestSubscriptionHandle> createSubscriptionHandle();

    /**
     * Get the event base for async operations
     */
    folly::EventBase* getEventBase() const { return eventBase_; }

    /**
     * Get the relay URL
     */
    const std::string& getRelayUrl() const { return relayUrl_; }

    /**
     * Get the default timeout
     */
    std::chrono::milliseconds getTimeout() const { return timeout_; }

    /**
     * Check if a resource is ready
     */
    bool hasPublisher() const { return publisher_ && publisher_->isConnected(); }
    bool hasSubscriber() const { return subscriber_ && subscriber_->isConnected(); }

    /**
     * Reset specific resources (for tests that need to reconnect)
     */
    void resetPublisher();
    void resetSubscriber();

private:
    // Configuration
    folly::EventBase* eventBase_;
    std::string relayUrl_;
    std::chrono::milliseconds timeout_;

    // Managed resources
    std::shared_ptr<moq_interface::MoQInterface> publisher_;
    std::shared_ptr<moq_interface::MoQInterface> subscriber_;
    std::vector<std::shared_ptr<moq_interface::MoQInterface>> additionalInterfaces_;

    // Helper methods
    void cleanupInterface(std::shared_ptr<moq_interface::MoQInterface>& interface, 
                         const std::string& name);
};

} // namespace interop_test
