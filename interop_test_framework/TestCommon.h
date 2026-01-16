#pragma once

#include <iostream>
#include <memory>
#include <folly/coro/Task.h>
#include <moxygen/Publisher.h>

namespace interop_test {

enum class TestResult {
    PASS,
    FAIL,
    TIMEOUT,
    ERROR
};

// Simple test subscription handle for publish/subscribe tests
class TestSubscriptionHandle : public moxygen::SubscriptionHandle {
public:
    TestSubscriptionHandle() = default;

    TestSubscriptionHandle(moxygen::SubscribeOk ok)
        : moxygen::SubscriptionHandle(std::move(ok)) {}

    void unsubscribe() override {
        std::cout << "TestSubscriptionHandle::unsubscribe() called" << std::endl;
    }

    folly::coro::Task<SubscribeUpdateResult> subscribeUpdate(moxygen::SubscribeUpdate subUpdate) override {
        std::cout << "TestSubscriptionHandle::subscribeUpdate() called with request ID: "
                  << subUpdate.requestID << std::endl;
        
        // Return a successful result
        co_return moxygen::SubscribeUpdateOk{subUpdate.requestID};
    }
};

} // namespace interop_test
