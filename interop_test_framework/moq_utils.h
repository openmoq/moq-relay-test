#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <folly/coro/Task.h>
#include <folly/io/async/EventBase.h>
#include <moxygen/MoQClient.h>
#include <moxygen/Publisher.h>
#include <moxygen/Subscriber.h>
#include <moxygen/events/MoQFollyExecutorImpl.h>
#include <proxygen/lib/utils/URL.h>
#include <fizz/tool/CertificateVerifiers.h>

namespace moq_utils {

// Minimal stub Publisher implementation for testing
class StubPublisher : public moxygen::Publisher {
public:
    ~StubPublisher() override = default;

    folly::coro::Task<SubscribeResult> subscribe(
        moxygen::SubscribeRequest sub,
        std::shared_ptr<moxygen::TrackConsumer> callback) override {
        co_return folly::makeUnexpected(
            moxygen::SubscribeError{
                sub.requestID,
                moxygen::SubscribeErrorCode::NOT_SUPPORTED,
                "Test stub - no publishing supported"});
    }
};

// Minimal stub Subscriber implementation for testing
class StubSubscriber : public moxygen::Subscriber {
public:
    ~StubSubscriber() override = default;

    PublishResult publish(
        moxygen::PublishRequest pub,
        std::shared_ptr<SubscriptionHandle> handle) override {
        return folly::makeUnexpected(
            moxygen::PublishError{
                pub.requestID,
                moxygen::PublishErrorCode::NOT_SUPPORTED,
                "Test stub - no subscribing supported"});
    }
};

// Configuration structure for MoQ session creation
struct MoQSessionConfig {
    std::string url = "https://localhost:4433/moq";
    std::chrono::milliseconds connectTimeout = std::chrono::milliseconds(30000);
    std::chrono::milliseconds transactionTimeout = std::chrono::milliseconds(30000);
    bool useInsecureVerifier = true;
    std::shared_ptr<moxygen::Publisher> publishHandler = nullptr;
    std::shared_ptr<moxygen::Subscriber> subscribeHandler = nullptr;
};

/**
 * Creates a configured MoQ client and establishes a session
 *
 * @param eventBase The event base to use for the client
 * @param config Configuration options for the MoQ session
 * @return Task that resolves to a shared_ptr to MoQClient on success
 */
folly::coro::Task<std::shared_ptr<moxygen::MoQClient>> createMoQSession(
    folly::EventBase* eventBase,
    const MoQSessionConfig& config = {});

/**
 * Creates a MoQ session with default stub handlers
 * This is a convenience function for testing scenarios
 *
 * @param eventBase The event base to use for the client
 * @param url The MoQ server URL (defaults to localhost:4433/moq)
 * @return Task that resolves to a shared_ptr to MoQClient on success
 */
folly::coro::Task<std::shared_ptr<moxygen::MoQClient>> createMoQSessionWithStubHandlers(
    folly::EventBase* eventBase,
    const std::string& url = "https://localhost:4433/moq");

} // namespace moq_utils