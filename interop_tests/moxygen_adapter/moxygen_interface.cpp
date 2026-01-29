#include "moxygen_interface.h"
#include "moxygen_mocks.h"
#include "type_conversions.h"
#include <folly/futures/Future.h>
#include <folly/coro/BlockingWait.h>
#include <iostream>

namespace interop_test {

MoxygenInterface::MoxygenInterface(folly::EventBase *eventBase)
    : eventBase_(eventBase) {
  if (!eventBase_) {
    throw std::invalid_argument("EventBase cannot be null");
  }
}

bool MoxygenInterface::connect(
    const std::string &url, std::chrono::milliseconds connectTimeout,
    std::chrono::milliseconds transactionTimeout, bool useInsecureVerifier) {

  try {
    // Create executor from the provided event base
    auto executor = std::make_shared<moxygen::MoQFollyExecutorImpl>(eventBase_);

    proxygen::URL parsedUrl(url);
    if (!parsedUrl.isValid()) {
      std::cerr << "Invalid URL: " << url << std::endl;
      return false;
    }

    std::shared_ptr<fizz::CertificateVerifier> verifier = nullptr;
    if (useInsecureVerifier) {
      verifier = std::make_shared<fizz::InsecureAcceptAnyCertificate>();
    }

    // Create MoQ client with MoQRelaySession factory for announcement support
    client_ = std::make_shared<moxygen::MoQClient>(
        executor, std::move(parsedUrl),
        moxygen::MoQRelaySession::createRelaySessionFactory(), verifier);

    // Set up MoQ session using blockingWait
    folly::coro::blockingWait(
        client_->setupMoQSession(connectTimeout, transactionTimeout,
                                nullptr, nullptr, {}));

    // Cast once and store for later use
    relaySession_ = std::dynamic_pointer_cast<moxygen::MoQRelaySession>(
        client_->moqSession_);
    if (!relaySession_) {
      std::cerr << "Failed to cast to MoQRelaySession" << std::endl;
      client_.reset();
      return false;
    }

    std::cout << "MoQ session established successfully" << std::endl;
    return true;

  } catch (const std::exception &ex) {
    std::cerr << "Failed to establish MoQ session: " << ex.what() << std::endl;
    relaySession_.reset();
    client_.reset();
    return false;
  }
}

folly::coro::Task<bool> MoxygenInterface::_doPublish(
    const std::string &trackNamespace, const std::string &trackName,
    std::shared_ptr<MockSubscriptionHandle> externalHandle) {

  try {
    if (!isConnected()) {
      std::cerr << "No MoQ session available" << std::endl;
      co_return false;
    }

    auto session = client_->moqSession_;

    // Create publish request
    moxygen::PublishRequest publishReq;
    publishReq.fullTrackName =
        moxygen::FullTrackName{.trackNamespace = moxygen::TrackNamespace(
                                   std::vector<std::string>{trackNamespace}),
                               .trackName = trackName};
    publishReq.requestID = moxygen::RequestID{0};
    publishReq.groupOrder = moxygen::GroupOrder::Default;
    publishReq.forward = false;

    std::cout << "Sending publish request for track: " << trackNamespace << "/"
              << trackName << std::endl;

    auto subscriptionHandle = externalHandle
                                  ? externalHandle
                                  : std::make_shared<MockSubscriptionHandle>();

    // Send publish request via session's publish method
    auto publishResult = session->publish(publishReq, subscriptionHandle);

    if (publishResult.hasValue()) {
      auto publishConsumerAndReply = std::move(publishResult.value());

      // Handle the PUBLISH_OK or PUBLISH_ERROR
      auto replyTask = std::move(publishConsumerAndReply.reply);
      auto replyResult = co_await std::move(replyTask);

      if (replyResult.hasValue()) {
        auto publishOk = replyResult.value();
        std::cout << "Publish OK received. Request ID: "
                  << publishOk.requestID.value << std::endl;
        co_return true;
      } else {
        auto error = replyResult.error();
        std::cerr << "Publish failed: " << error.reasonPhrase << std::endl;
        co_return false;
      }
    } else {
      auto error = publishResult.error();
      std::cerr << "Publish request failed: " << error.reasonPhrase
                << std::endl;
      co_return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in publish: " << ex.what() << std::endl;
    co_return false;
  }
}

bool MoxygenInterface::publish(
  const std::string &trackNamespace, const std::string &trackName) {
  return folly::coro::blockingWait(_doPublish(trackNamespace, trackName, nullptr));
}


folly::coro::Task<moxygen::MoQRelaySession::SubscribeResult> MoxygenInterface::_doSubscribe(
  const std::string &trackNamespace,
  const std::string &trackName,
  uint8_t priority,
  moxygen::GroupOrder groupOrder) {

    // Create subscribe request
    moxygen::SubscribeRequest subscribeReq = moxygen::SubscribeRequest::make(
        moxygen::FullTrackName{.trackNamespace = moxygen::TrackNamespace(
                                   std::vector<std::string>{trackNamespace}),
                               .trackName = trackName},
        priority, groupOrder);

    std::cout << "Sending subscribe request for track: " << trackNamespace
              << "/" << trackName << std::endl;

      std::shared_ptr<MockTrackConsumer> trackConsumer = std::make_shared<MockTrackConsumer>();

    auto session = client_->moqSession_;
    co_return co_await session->subscribe(subscribeReq, trackConsumer);
}


bool MoxygenInterface::subscribe(
    const std::string &trackNamespace, const std::string &trackName,
    uint8_t priority,
    GroupOrder groupOrder) {

  try {
    if (!isConnected()) {
      std::cerr << "No MoQ session available" << std::endl;
      return false;
    }

    // Convert interop_test types to moxygen types
    auto moxygenGroupOrder = toMoxygenGroupOrder(groupOrder);

    // Send subscribe request via session's subscribe method
    auto subscribeResult = folly::coro::blockingWait(
        _doSubscribe(trackNamespace, trackName, priority, moxygenGroupOrder));

    if (subscribeResult.hasValue()) {
      auto subscriptionHandle = std::move(subscribeResult.value());
      std::cout << "Subscribe OK received. Track alias: "
                << subscriptionHandle->subscribeOk().trackAlias.value
                << std::endl;
      return true;
    } else {
      auto error = subscribeResult.error();
      std::cerr << "Subscribe failed: " << error.reasonPhrase << std::endl;
      return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in subscribe: " << ex.what() << std::endl;
    return false;
  }
}


bool MoxygenInterface::subscribeUpdate(
    const std::string &trackNamespace, const std::string &trackName,
    uint8_t priority,
    GroupOrder groupOrder,
    AbsoluteLocation start,
    uint8_t endGroup
) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    // Get a subscription handle and use that to send the subscribe update
    auto subscribeResult = folly::coro::blockingWait(_doSubscribe(
        trackNamespace,
        trackName,
        128,
        moxygen::GroupOrder::OldestFirst));

    if (!subscribeResult.hasValue()) {
      auto error = subscribeResult.error();
      std::cerr << "Subscribe failed before subscribe update: " << error.reasonPhrase
                << std::endl;
      return false;
    }
    auto subscriptionHandle = std::move(subscribeResult.value());
    
    // Convert interop_test types to moxygen types
    auto moxygenStart = toMoxygenAbsoluteLocation(start);
    
    // Send subscribe update request
    moxygen::SubscribeUpdate subscribeUpdate;
    subscribeUpdate.requestID = moxygen::RequestID{2};
    subscribeUpdate.subscriptionRequestID = moxygen::RequestID{0};
    // For draft < 15, start and endGroup are required
    subscribeUpdate.start = moxygenStart;
    subscribeUpdate.endGroup = endGroup;  
    subscribeUpdate.priority = priority;
    subscribeUpdate.forward = false;

    std::cout << "Sending subscribe update request" << std::endl;
    auto result = folly::coro::blockingWait(subscriptionHandle->subscribeUpdate(subscribeUpdate));
    std::cout << "Subscribe update sent successfully" << std::endl;
    if (!result.hasValue()) {
      auto error = result.error();
      std::cerr << "Subscribe update failed: " << error.reasonPhrase << std::endl;
      return false;
    } else {
      std::cout << "Subscribe update succeeded" << std::endl;
      return true;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in subscribe update: " << ex.what() << std::endl;
    return false;
  }
} 

folly::coro::Task<folly::Expected<std::shared_ptr<moxygen::MoQRelaySession::AnnounceHandle>, moxygen::AnnounceError>>
MoxygenInterface::_doPublishNamespace(const std::string &trackNamespace) {
  // Create announce request
  moxygen::Announce announce;
  announce.requestID = moxygen::RequestID{1};
  announce.trackNamespace =
      moxygen::TrackNamespace(std::vector<std::string>{trackNamespace});

  std::cout << "Sending announce request for namespace: " << trackNamespace
            << std::endl;

  co_return co_await relaySession_->announce(announce);
}

bool
MoxygenInterface::publish_namespace(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    auto announceResult = folly::coro::blockingWait(
        _doPublishNamespace(trackNamespace));

    if (announceResult.hasValue()) {
      auto announceHandle = std::move(announceResult.value());
      std::cout << "Announce OK received for namespace: " << trackNamespace
                << std::endl;
      return true;
    } else {
      auto error = announceResult.error();
      std::cerr << "Announce failed: " << error.reasonPhrase << std::endl;
      return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in publish_namespace: " << ex.what() << std::endl;
    return false;
  }
}

bool
MoxygenInterface::publish_namespace_done(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    // First announce
    auto announceResult = folly::coro::blockingWait(
        _doPublishNamespace(trackNamespace));
    if (!announceResult.hasValue()) {
      auto error = announceResult.error();
      std::cerr << "Announce failed before unannounce: " << error.reasonPhrase
                << std::endl;
      return false;
    }

    auto announceHandle_ = std::move(announceResult.value());

    std::cout << "Sending unannounce request for namespace: " << trackNamespace
              << std::endl;
    // Send unannounce request
    announceHandle_->unannounce();
    return true;

  } catch (const std::exception &ex) {
    std::cerr << "Exception in publish_namespace_done: " << ex.what() << std::endl;
    return false;
  }
}

bool
MoxygenInterface::subscribe_namespace(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    // Create subscribe announces request
    moxygen::SubscribeAnnounces subscribeAnnounces;
    subscribeAnnounces.requestID = moxygen::RequestID{1};
    subscribeAnnounces.trackNamespacePrefix =
        moxygen::TrackNamespace(std::vector<std::string>{trackNamespace});

    auto subscribeAnnounceResult_ = folly::coro::blockingWait(
        relaySession_->subscribeAnnounces(subscribeAnnounces));

    if (!subscribeAnnounceResult_.hasValue()) {
      auto error = subscribeAnnounceResult_.error();
      std::cerr << "Subscribe announces failed: " << error.reasonPhrase
                << std::endl;
      return false;
    } else {
      std::cout << "Subscribe announces succeeded for namespace: "
                << trackNamespace << std::endl;
      return true;
    }

  } catch (const std::exception &ex) {
    std::cerr << "Exception in subscribe_namespace: " << ex.what() << std::endl;
    return false;
  }
}

bool
MoxygenInterface::trackStatus(const std::string &trackNamespace,
                              const std::string &trackName) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    auto result = folly::coro::blockingWait(relaySession_->trackStatus(
        moxygen::TrackStatus{.requestID = moxygen::RequestID{1},
                             .fullTrackName = moxygen::FullTrackName{
                                 .trackNamespace = moxygen::TrackNamespace(
                                     std::vector<std::string>{trackNamespace}),
                                 .trackName = trackName}}));

    if (result.hasValue()) {
      auto trackStatusOk = result.value();
      std::cout << "Track Status OK received for track: " << trackNamespace
                << "/" << trackName << std::endl;
      return true;
    } else {
      auto error = result.error();
      std::cerr << "Track Status failed: " << error.reasonPhrase << std::endl;
      return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in trackStatus: " << ex.what() << std::endl;
    return false;
  }
}

bool MoxygenInterface::goaway() {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    std::cout << "Sending goaway after publish" << std::endl;
    moxygen::Goaway goaway_inp{.newSessionUri = ""};

    relaySession_->goaway(goaway_inp);
    std::cout << "Goaway sent successfully" << std::endl;

    return true;
  } catch (const std::exception &ex) {
    std::cerr << "Exception in goaway: " << ex.what() << std::endl;
    return false;
  }
}

bool MoxygenInterface::goaway_sequence() {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    // First, publish with a dummy track name
    std::cout << "Publishing dummy track before goaway" << std::endl;
    auto subscriptionHandle_ = std::make_shared<MockSubscriptionHandle>();
    bool publishSuccess = folly::coro::blockingWait(
        _doPublish("dummy_namespace", "dummy_track", subscriptionHandle_));
    if (!publishSuccess) {
      std::cerr << "Failed to publish dummy track" << std::endl;
      return false;
    }

    // Then, send goaway
    bool goawaySuccess = goaway();
    if (!goawaySuccess) {
      std::cerr << "Failed to send goaway" << std::endl;
      return false;
    }

    std::cout << "Goaway sequence completed successfully" << std::endl;

    // After goaway, the session may close immediately, so we can't reliably
    // check if unsubscribe was called. Just return success if goaway sent.
    // Note that the protocol recommends waiting for a short period for
    // the unsubscribes.
    return true;

  } catch (const std::exception &ex) {
    std::cerr << "Exception in goaway_sequence: " << ex.what() << std::endl;
    return false;
  }
}

bool
MoxygenInterface::setMaxConcurrentRequests(uint32_t maxConcurrentRequests) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    relaySession_->setMaxConcurrentRequests(maxConcurrentRequests);
    std::cout << "Set max concurrent requests to: " << maxConcurrentRequests
              << std::endl;
    return true;

  } catch (const std::exception &ex) {
    std::cerr << "Exception in setMaxConcurrentRequests: " << ex.what()
              << std::endl;
    return false;
  }
}

} // namespace interop_test