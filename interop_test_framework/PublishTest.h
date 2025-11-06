#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <folly/coro/Task.h>
#include <folly/coro/BlockingWait.h>
#include <folly/io/async/EventBase.h>
#include <moxygen/MoQClient.h>
#include <moxygen/MoQSession.h>
#include <moxygen/MoQConsumers.h>
#include <moxygen/MoQFramer.h>
#include <moxygen/Publisher.h>
#include "moq_utils.h"

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

// Simple test track consumer for receiving subscription data
class TestTrackConsumer : public moxygen::TrackConsumer {
public:
    TestTrackConsumer() = default;
    ~TestTrackConsumer() override = default;

    // Explicitly delete copy constructor and assignment operator
    TestTrackConsumer(const TestTrackConsumer&) = delete;
    TestTrackConsumer& operator=(const TestTrackConsumer&) = delete;

    // Allow move constructor and assignment operator
    TestTrackConsumer(TestTrackConsumer&&) = default;
    TestTrackConsumer& operator=(TestTrackConsumer&&) = default;

    // Implement pure virtual methods from TrackConsumer
    folly::Expected<folly::Unit, moxygen::MoQPublishError> setTrackAlias(
        moxygen::TrackAlias alias) override {
        std::cout << "Set track alias: " << alias << std::endl;
        return folly::Unit{};
    }

    folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>, moxygen::MoQPublishError>
    beginSubgroup(uint64_t groupID, uint64_t subgroupID, moxygen::Priority priority) override {
        std::cout << "Begin subgroup: group=" << groupID << " subgroup=" << subgroupID << std::endl;
        return nullptr; // Return null for test - not actually implementing subgroup handling
    }

    folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
    awaitStreamCredit() override {
        return folly::makeSemiFuture<folly::Unit>(folly::Unit{});
    }

    folly::Expected<folly::Unit, moxygen::MoQPublishError> objectStream(
        const moxygen::ObjectHeader& header,
        moxygen::Payload payload) override {
        std::cout << "Received object stream: group=" << header.group
                  << " subgroup=" << header.subgroup << " id=" << header.id << std::endl;
        return folly::Unit{};
    }

    folly::Expected<folly::Unit, moxygen::MoQPublishError> datagram(
        const moxygen::ObjectHeader& header,
        moxygen::Payload payload) override {
        std::cout << "Received datagram: group=" << header.group
                  << " subgroup=" << header.subgroup << " id=" << header.id << std::endl;
        return folly::Unit{};
    }

    folly::Expected<folly::Unit, moxygen::MoQPublishError> groupNotExists(
        uint64_t groupID,
        uint64_t subgroup,
        moxygen::Priority pri,
        moxygen::Extensions extensions = moxygen::noExtensions()) override {
        std::cout << "Group not exists: group=" << groupID << " subgroup=" << subgroup << std::endl;
        return folly::Unit{};
    }

    folly::Expected<folly::Unit, moxygen::MoQPublishError> subscribeDone(
        moxygen::SubscribeDone subDone) override {
        std::cout << "Subscribe done" << std::endl;
        return folly::Unit{};
    }

    void onHeader(const moxygen::ObjectHeader& header) {
        // Test implementation - just log
        std::cout << "Received object header for group " << header.group
                  << " subgroup " << header.subgroup << " id " << header.id << std::endl;
    }

    void onObject(const moxygen::ObjectHeader& header, std::unique_ptr<folly::IOBuf> payload) {
        // Test implementation - log received data
        if (payload) {
            std::string data = payload->to<std::string>();
            std::cout << "Received object data: " << data << std::endl;
        }
    }

    void onError(moxygen::ResetStreamErrorCode error) {
        std::cout << "Track consumer error: " << static_cast<uint64_t>(error) << std::endl;
    }
};class PublishTest {
public:
    PublishTest(folly::EventBase* eventBase) : eventBase_(eventBase) {}

    TestResult runTest(const PublishTestConfig& config);
    std::string getLastError() const { return lastError_; }

private:
    std::string lastError_;
    std::shared_ptr<moxygen::MoQClient> client_;
    std::shared_ptr<TestSubscriptionHandle> subscriptionHandle_;
    folly::EventBase* eventBase_;

    folly::coro::Task<bool> establishSession(const std::string& serverUrl);
    folly::coro::Task<bool> createSubscription(const PublishTestConfig& config);
    folly::coro::Task<bool> sendPublishRequest(const PublishTestConfig& config);
    void cleanup();
};
}