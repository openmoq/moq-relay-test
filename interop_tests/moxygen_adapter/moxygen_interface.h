#pragma once

#include <chrono>
#include <fizz/tool/CertificateVerifiers.h>
#include <folly/coro/Task.h>
#include <folly/io/async/EventBase.h>
#include <memory>
#include <moxygen/MoQClient.h>
#include <moxygen/MoQRelaySession.h>
#include <moxygen/MoQSession.h>
#include <moxygen/Publisher.h>
#include <moxygen/Subscriber.h>
#include <moxygen/events/MoQFollyExecutorImpl.h>
#include <proxygen/lib/utils/URL.h>
#include <string>

namespace interop_test {

// Forward declaration
class TestSubscriptionHandle;

/**
 * MoQ Interface - Provides a simplified interface for MoQ operations
 */
class MoxygenInterface {
public:
  explicit MoxygenInterface(folly::EventBase *eventBase);
  ~MoxygenInterface() = default;

  /**
   * Establishes a MoQ session with the given URL
   * @param url The MoQ relay server URL
   * @param connectTimeout Connection timeout
   * @param transactionTimeout Transaction timeout
   * @param useInsecureVerifier Whether to use insecure certificate verification
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> connect(const std::string &url,
                                  std::chrono::milliseconds connectTimeout =
                                      std::chrono::milliseconds(30000),
                                  std::chrono::milliseconds transactionTimeout =
                                      std::chrono::milliseconds(30000),
                                  bool useInsecureVerifier = true);

  /**
   * Sends a publish request to the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @param subscriptionHandle Optional subscription handle
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool>
  publish(const std::string &trackNamespace, const std::string &trackName,
          std::shared_ptr<TestSubscriptionHandle> externalHandle = nullptr);

  /**
   * Subscribes to a track on the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @param trackConsumer Callback to receive track data
   * @param priority Subscribe priority (default: 128)
   * @param groupOrder Group ordering preference (default: OldestFirst)
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool>
  subscribe(const std::string &trackNamespace, const std::string &trackName,
            std::shared_ptr<moxygen::TrackConsumer> trackConsumer,
            uint8_t priority = 128,
            moxygen::GroupOrder groupOrder = moxygen::GroupOrder::OldestFirst);

  folly::coro::Task<bool> subscribeUpdate(const std::string &trackNamespace, const std::string &trackName,
            std::shared_ptr<moxygen::TrackConsumer> trackConsumer,
            uint8_t priority = 128,
            moxygen::GroupOrder groupOrder = moxygen::GroupOrder::OldestFirst);

  /**
   * Announces a namespace to the MoQ relay
   * @param trackNamespace The namespace to announce (e.g., "video/conference")
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> announce(const std::string &trackNamespace);

  /**
   * Signals publish done for a namespace to the MoQ relay
   * @param trackNamespace The namespace to unannounce (e.g.,
   * "video/conference")
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> unannounce(const std::string &trackNamespace);

  /**
   * Subscribes to announces for a given namespace prefix
   * @param trackNamespace The namespace prefix to subscribe to (e.g., "video/")
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> subscribeAnnounces(const std::string &trackNamespace);

  /**
   * Sends a track status request to the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> trackStatus(const std::string &trackNamespace,
                                      const std::string &trackName);

  /**
   * Sends a goaway signal to the MoQ relay
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> goaway();

  /**
   * Sends a goaway signal after publishing a dummy track
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> goaway_sequence();

  folly::coro::Task<bool>
  setMaxConcurrentRequests(uint32_t maxConcurrentRequests);

  std::shared_ptr<moxygen::MoQClient> getClient() const { return client_; }

  bool isConnected() const { return client_ && client_->moqSession_; }

private:
  folly::coro::Task<moxygen::MoQRelaySession::SubscribeResult> _subscribe(
      moxygen::SubscribeRequest subscribeReq,
      std::shared_ptr<moxygen::TrackConsumer> trackConsumer);

  folly::EventBase *eventBase_;
  std::shared_ptr<moxygen::MoQClient> client_;
  std::shared_ptr<moxygen::MoQRelaySession>
      relaySession_; // Cache the casted session
};

} // namespace interop_test