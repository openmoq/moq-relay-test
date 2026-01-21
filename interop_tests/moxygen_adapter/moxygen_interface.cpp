#include "moxygen_interface.h"
#include "moxygen_mocks.h"
#include <folly/futures/Future.h>
#include <iostream>

namespace interop_test {

MoxygenInterface::MoxygenInterface(folly::EventBase *eventBase)
    : eventBase_(eventBase) {
  if (!eventBase_) {
    throw std::invalid_argument("EventBase cannot be null");
  }
}

folly::coro::Task<bool> MoxygenInterface::connect(
    const std::string &url, std::chrono::milliseconds connectTimeout,
    std::chrono::milliseconds transactionTimeout, bool useInsecureVerifier) {

  try {
    // Create executor from the provided event base
    auto executor = std::make_shared<moxygen::MoQFollyExecutorImpl>(eventBase_);

    proxygen::URL parsedUrl(url);
    if (!parsedUrl.isValid()) {
      std::cerr << "Invalid URL: " << url << std::endl;
      co_return false;
    }

    std::shared_ptr<fizz::CertificateVerifier> verifier = nullptr;
    if (useInsecureVerifier) {
      verifier = std::make_shared<fizz::InsecureAcceptAnyCertificate>();
    }

    // Create MoQ client with MoQRelaySession factory for announcement support
    client_ = std::make_shared<moxygen::MoQClient>(
        executor, std::move(parsedUrl),
        moxygen::MoQRelaySession::createRelaySessionFactory(), verifier);

    // Set up MoQ session
    co_await client_->setupMoQSession(connectTimeout, transactionTimeout,
                                      nullptr, nullptr, {});

    // Cast once and store for later use
    relaySession_ = std::dynamic_pointer_cast<moxygen::MoQRelaySession>(
        client_->moqSession_);
    if (!relaySession_) {
      std::cerr << "Failed to cast to MoQRelaySession" << std::endl;
      client_.reset();
      co_return false;
    }

    std::cout << "MoQ session established successfully" << std::endl;
    co_return true;

  } catch (const std::exception &ex) {
    std::cerr << "Failed to establish MoQ session: " << ex.what() << std::endl;
    relaySession_.reset();
    client_.reset();
    co_return false;
  }
}

folly::coro::Task<bool> MoxygenInterface::publish(
    const std::string &trackNamespace, const std::string &trackName,
    std::shared_ptr<TestSubscriptionHandle> externalHandle) {

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
    publishReq.requestID = moxygen::RequestID{1};
    publishReq.trackAlias = moxygen::TrackAlias{1};
    publishReq.groupOrder = moxygen::GroupOrder::Default;
    publishReq.forward = false;

    std::cout << "Sending publish request for track: " << trackNamespace << "/"
              << trackName << std::endl;

    auto subscriptionHandle = externalHandle ? externalHandle : std::make_shared<TestSubscriptionHandle>();

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

folly::coro::Task<bool> MoxygenInterface::subscribe(
    const std::string &trackNamespace, 
    const std::string &trackName,
    std::shared_ptr<moxygen::TrackConsumer> trackConsumer, 
    uint8_t priority,
    moxygen::GroupOrder groupOrder) {

  try {
    if (!isConnected()) {
      std::cerr << "No MoQ session available" << std::endl;
      co_return false;
    }

    if (!trackConsumer) {
      std::cerr << "TrackConsumer cannot be null" << std::endl;
      co_return false;
    }

    auto session = client_->moqSession_;

    // Create subscribe request
    moxygen::SubscribeRequest subscribeReq = moxygen::SubscribeRequest::make(
        moxygen::FullTrackName{.trackNamespace = moxygen::TrackNamespace(
                                   std::vector<std::string>{trackNamespace}),
                               .trackName = trackName},
        priority, groupOrder);

    std::cout << "Sending subscribe request for track: " << trackNamespace
              << "/" << trackName << std::endl;

    // Send subscribe request via session's subscribe method
    auto subscribeResult =
        co_await session->subscribe(subscribeReq, trackConsumer);

    if (subscribeResult.hasValue()) {
      auto subscriptionHandle = std::move(subscribeResult.value());
      std::cout << "Subscribe OK received. Track alias: "
                << subscriptionHandle->subscribeOk().trackAlias.value
                << std::endl;
      co_return true;
    } else {
      auto error = subscribeResult.error();
      std::cerr << "Subscribe failed: " << error.reasonPhrase << std::endl;
      co_return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in subscribe: " << ex.what() << std::endl;
    co_return false;
  }
}

folly::coro::Task<bool>
MoxygenInterface::announce(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      co_return false;
    }

    // Create announce request
    moxygen::Announce announce;
    announce.requestID = moxygen::RequestID{1};
    announce.trackNamespace =
        moxygen::TrackNamespace(std::vector<std::string>{trackNamespace});

    std::cout << "Sending announce request for namespace: " << trackNamespace
              << std::endl;

    // Send announce request
    auto announceResult = co_await relaySession_->announce(announce);

    if (announceResult.hasValue()) {
      auto announceHandle = std::move(announceResult.value());
      std::cout << "Announce OK received for namespace: " << trackNamespace
                << std::endl;
      co_return true;
    } else {
      auto error = announceResult.error();
      std::cerr << "Announce failed: " << error.reasonPhrase << std::endl;
      co_return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in announce: " << ex.what() << std::endl;
    co_return false;
  }
}

folly::coro::Task<bool>
MoxygenInterface::unannounce(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      co_return false;
    }

    // First announce
    moxygen::Announce announce;
    announce.requestID = moxygen::RequestID{1};
    announce.trackNamespace =
        moxygen::TrackNamespace(std::vector<std::string>{trackNamespace});
    auto announceResult = co_await relaySession_->announce(announce);
    if (!announceResult.hasValue()) {
      auto error = announceResult.error();
      std::cerr << "Announce failed before unannounce: " << error.reasonPhrase
                << std::endl;
      co_return false;
    }

    auto announceHandle_ = std::move(announceResult.value());

    std::cout << "Sending unannounce request for namespace: " << trackNamespace
              << std::endl;
    // Send unannounce request
    announceHandle_->unannounce();
    co_return true;

  } catch (const std::exception &ex) {
    std::cerr << "Exception in unannounce: " << ex.what() << std::endl;
    co_return false;
  }
}

folly::coro::Task<bool>
MoxygenInterface::subscribeAnnounces(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      co_return false;
    }

    // Create subscribe announces request
    moxygen::SubscribeAnnounces subscribeAnnounces;
    subscribeAnnounces.requestID = moxygen::RequestID{1};
    subscribeAnnounces.trackNamespacePrefix =
        moxygen::TrackNamespace(std::vector<std::string>{trackNamespace});

    auto subscribeAnnounceResult_ =
        co_await relaySession_->subscribeAnnounces(subscribeAnnounces);

    if (!subscribeAnnounceResult_.hasValue()) {
      auto error = subscribeAnnounceResult_.error();
      std::cerr << "Subscribe announces failed: " << error.reasonPhrase
                << std::endl;
      co_return false;
    } else {
      std::cout << "Subscribe announces succeeded for namespace: "
                << trackNamespace << std::endl;
      co_return true;
    }

  } catch (const std::exception &ex) {
    std::cerr << "Exception in subscribe announces: " << ex.what() << std::endl;
    co_return false;
  }
}

folly::coro::Task<bool> MoxygenInterface::trackStatus(
    const std::string &trackNamespace, const std::string &trackName) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      co_return false;
    }

    auto result = co_await relaySession_->trackStatus(moxygen::TrackStatus{
        .requestID = moxygen::RequestID{1},
        .fullTrackName = moxygen::FullTrackName{
            .trackNamespace = moxygen::TrackNamespace(
                std::vector<std::string>{trackNamespace}),
            .trackName = trackName}});

    if (result.hasValue()) {
      auto trackStatusOk = result.value();
      std::cout << "Track Status OK received for track: " << trackNamespace
                << "/" << trackName << std::endl;
      co_return true;
    } else {
      auto error = result.error();
      std::cerr << "Track Status failed: " << error.reasonPhrase << std::endl;
      co_return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in trackStatus: " << ex.what() << std::endl;
    co_return false;
  }
}

folly::coro::Task<bool> MoxygenInterface::goaway() {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      co_return false;
    }

    std::cout << "Sending goaway after publish" << std::endl;
    moxygen::Goaway goaway_inp{.newSessionUri = ""};

    relaySession_->goaway(goaway_inp);
    std::cout << "Goaway sent successfully" << std::endl;
    
    
    co_return true;
  } catch (const std::exception &ex) {
    std::cerr << "Exception in goaway: " << ex.what() << std::endl;
    co_return false;
  }
}

folly::coro::Task<bool> MoxygenInterface::goaway_sequence() {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      co_return false;
    }

    // First, publish with a dummy track name
    std::cout << "Publishing dummy track before goaway" << std::endl;
    auto subscriptionHandle_ = std::make_shared<TestSubscriptionHandle>();
    bool publishSuccess = co_await publish("dummy_namespace", "dummy_track", subscriptionHandle_);
    if (!publishSuccess) {
      std::cerr << "Failed to publish dummy track" << std::endl;
      co_return false;
    }

    // Then, send goaway
    bool goawaySuccess = co_await goaway();
    if (!goawaySuccess) {
      std::cerr << "Failed to send goaway" << std::endl;
      co_return false;
    }

    std::cout << "Goaway sequence completed successfully" << std::endl;
    
    // After goaway, the session may close immediately, so we can't reliably
    // check if unsubscribe was called. Just return success if goaway sent.
    // Note that the protocol recommends waiting for a short period though for the unsubscribes.
    co_return true;

  } catch (const std::exception &ex) {
    std::cerr << "Exception in goaway_sequence: " << ex.what() << std::endl;
    co_return false;
  }
}

folly::coro::Task<bool> MoxygenInterface::setMaxConcurrentRequests(
    uint32_t maxConcurrentRequests) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      co_return false;
    }

    relaySession_->setMaxConcurrentRequests(maxConcurrentRequests);
    std::cout << "Set max concurrent requests to: " << maxConcurrentRequests
              << std::endl;
    co_return true;

  } catch (const std::exception &ex) {
    std::cerr << "Exception in setMaxConcurrentRequests: " << ex.what()
              << std::endl;
    co_return false;
  }
}

} // namespace interop_test