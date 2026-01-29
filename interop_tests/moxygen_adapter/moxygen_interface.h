#pragma once

#include "base/moqt_interface.h"
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
#include <moxygen/MoQFramer.h>
#include <proxygen/lib/utils/URL.h>
#include <string>

namespace interop_test {

// Forward declaration
class MockSubscriptionHandle;

/**
 * MoxygenInterface - Moxygen implementation of MoqtInterface
 * Provides MoQ operations using the moxygen library
 */
class MoxygenInterface : public MoqtInterface {
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
                                  bool useInsecureVerifier = true) override;

  /**
   * Sends a publish request to the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @param subscriptionHandle Optional subscription handle
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool>
  publish(const std::string &trackNamespace, const std::string &trackName) override;

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
            uint8_t priority = 128,
            GroupOrder groupOrder = GroupOrder::OldestFirst) override;

  folly::coro::Task<bool> subscribeUpdate(const std::string &trackNamespace, const std::string &trackName,
            uint8_t priority = 128,
            GroupOrder groupOrder = GroupOrder::OldestFirst,
            AbsoluteLocation start = AbsoluteLocation{0,0},
            uint8_t endGroup = 0) override;

  /**
   * Announces a namespace to the MoQ relay
   * @param trackNamespace The namespace to announce (e.g., "video/conference")
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> publish_namespace(const std::string &trackNamespace) override;

  /**
   * Signals publish done for a namespace to the MoQ relay
   * @param trackNamespace The namespace to unannounce (e.g.,
   * "video/conference")
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> publish_namespace_done(const std::string &trackNamespace) override;

  /**
   * Subscribes to announces for a given namespace prefix
   * @param trackNamespace The namespace prefix to subscribe to (e.g., "video/")
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> subscribe_namespace(const std::string &trackNamespace) override;

  /**
   * Sends a track status request to the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> trackStatus(const std::string &trackNamespace,
                                      const std::string &trackName) override;

  /**
   * Sends a goaway signal to the MoQ relay
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> goaway() override;

  /**
   * Sends a goaway signal after publishing a dummy track
   * @return Task that resolves to true on success
   */
  folly::coro::Task<bool> goaway_sequence() override;

  folly::coro::Task<bool>
  setMaxConcurrentRequests(uint32_t maxConcurrentRequests) override;

  std::shared_ptr<moxygen::MoQClient> getClient() const { return client_; }

  bool isConnected() const override { return client_ && client_->moqSession_; }

private:
  folly::coro::Task<moxygen::MoQRelaySession::SubscribeResult> _doSubscribe(
      const std::string &trackNamespace,
      const std::string &trackName,
      uint8_t priority,
      moxygen::GroupOrder groupOrder);

  folly::coro::Task<folly::Expected<std::shared_ptr<moxygen::MoQRelaySession::AnnounceHandle>, moxygen::AnnounceError>>
  _doPublishNamespace(const std::string &trackNamespace);

  folly::coro::Task<bool>
  _doPublish(const std::string &trackNamespace, const std::string &trackName,
          std::shared_ptr<MockSubscriptionHandle> externalHandle = nullptr);

  folly::EventBase *eventBase_;
  std::shared_ptr<moxygen::MoQClient> client_;
  std::shared_ptr<moxygen::MoQRelaySession>
      relaySession_; // Cache the casted session
};

} // namespace interop_test