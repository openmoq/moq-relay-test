#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <folly/coro/Task.h>
#include <folly/coro/BlockingWait.h>
#include <folly/io/async/EventBase.h>
#include <moxygen/Publisher.h>
#include "moq_interface.h"

namespace interop_test {

enum class TestResult {
    PASS,
    FAIL,
    TIMEOUT,
    ERROR
};

struct PublishTestConfig {
    std::string trackNamespace{"test"};
    std::string trackName{"test-track"};
    std::chrono::milliseconds timeout{5000};
    std::string serverUrl{"https://localhost:4433/moq"};
};

// Simple test subscription handle for publish tests
class TestSubscriptionHandle : public moxygen::SubscriptionHandle {
public:
    TestSubscriptionHandle() = default;

    TestSubscriptionHandle(moxygen::SubscribeOk ok)
        : moxygen::SubscriptionHandle(std::move(ok)) {}

    void unsubscribe() override {
        std::cout << "TestSubscriptionHandle::unsubscribe() called" << std::endl;
    }

    void subscribeUpdate(moxygen::SubscribeUpdate subUpdate) override {
        std::cout << "TestSubscriptionHandle::subscribeUpdate() called with request ID: "
                  << subUpdate.requestID << std::endl;
    }
};

class PublishTest {
public:
    PublishTest(folly::EventBase* eventBase) : eventBase_(eventBase) {}

    // Destructor to ensure proper cleanup
    ~PublishTest() {
        cleanup();
    }

    TestResult runTest(const PublishTestConfig& config);
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;
    std::shared_ptr<moq_interface::MoQInterface> moqInterface_;
    std::shared_ptr<TestSubscriptionHandle> subscriptionHandle_;
    folly::EventBase* eventBase_;

    void cleanup();
};
}