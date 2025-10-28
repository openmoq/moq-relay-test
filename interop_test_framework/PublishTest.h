#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <moxygen/MoQWebTransportClient.h>
#include <moxygen/MoQSession.h>
#include <moxygen/Subscriber.h>
#include <moxygen/MoQConsumers.h>
#include <moxygen/events/MoQFollyExecutorImpl.h>
#include <folly/coro/Task.h>
#include <folly/coro/BlockingWait.h>

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
    std::string serverUrl{"moq://localhost:4433"};
};

class PublishTest : public moxygen::Subscriber,
                    public std::enable_shared_from_this<PublishTest> {
public:
    PublishTest() = default;

    TestResult runTest(const PublishTestConfig& config);
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;
    std::shared_ptr<moxygen::MoQWebTransportClient> client_;
    std::shared_ptr<moxygen::MoQSession> session_;
    std::shared_ptr<moxygen::MoQFollyExecutorImpl> executor_;

    folly::coro::Task<void> connectToServer(const std::string& serverUrl);
    folly::coro::Task<bool> sendPublishRequest(const PublishTestConfig& config);
    void cleanup();
};
}