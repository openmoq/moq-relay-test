#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <folly/coro/Task.h>
#include <folly/io/async/EventBase.h>
#include <moxygen/MoQClient.h>
#include <moxygen/MoQSession.h>
#include <moxygen/Publisher.h>
#include <moxygen/Subscriber.h>
#include <moxygen/events/MoQFollyExecutorImpl.h>
#include <proxygen/lib/utils/URL.h>
#include <fizz/tool/CertificateVerifiers.h>

namespace moq_interface {

/**
 * MoQ Interface - Provides a simplified interface for MoQ operations
 */
class MoQInterface {
public:
    explicit MoQInterface(folly::EventBase* eventBase);
    ~MoQInterface() = default;

    /**
     * Establishes a MoQ session with the given URL
     * @param url The MoQ relay server URL
     * @param connectTimeout Connection timeout
     * @param transactionTimeout Transaction timeout
     * @param useInsecureVerifier Whether to use insecure certificate verification
     * @return Task that resolves to true on success
     */
    folly::coro::Task<bool> connect(
        const std::string& url,
        std::chrono::milliseconds connectTimeout = std::chrono::milliseconds(30000),
        std::chrono::milliseconds transactionTimeout = std::chrono::milliseconds(30000),
        bool useInsecureVerifier = true);

    /**
     * Sends a publish request to the MoQ relay
     * @param trackNamespace The track namespace
     * @param trackName The track name
     * @param subscriptionHandle Optional subscription handle
     * @return Task that resolves to true on success
     */
    folly::coro::Task<bool> publish(
        const std::string& trackNamespace,
        const std::string& trackName,
        std::shared_ptr<moxygen::SubscriptionHandle> subscriptionHandle = nullptr);


    /**
     * Subscribes to a track on the MoQ relay
     * @param trackNamespace The track namespace
     * @param trackName The track name
     * @param trackConsumer Callback to receive track data
     * @param priority Subscribe priority (default: 128)
     * @param groupOrder Group ordering preference (default: OldestFirst)
     * @return Task that resolves to true on success
     */
    folly::coro::Task<bool> subscribe(
        const std::string& trackNamespace,
        const std::string& trackName,
        std::shared_ptr<moxygen::TrackConsumer> trackConsumer,
        uint8_t priority = 128,
        moxygen::GroupOrder groupOrder = moxygen::GroupOrder::OldestFirst);

    std::shared_ptr<moxygen::MoQClient> getClient() const { return client_; }
    
    bool isConnected() const { return client_ && client_->moqSession_; }

private:
    folly::EventBase* eventBase_;
    std::shared_ptr<moxygen::MoQClient> client_;
};

} // namespace moq_interface