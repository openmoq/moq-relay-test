#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <folly/io/async/EventBase.h>
#include <moxygen/Publisher.h>
#include "moq_interface.h"
#include "TestCommon.h"

namespace interop_test {

struct PublishTestConfig {
    std::string trackNamespace{"test"};
    std::string trackName{"test-track"};
    std::chrono::milliseconds timeout{5000};
    std::string serverUrl{"https://localhost:4433/moq"};
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