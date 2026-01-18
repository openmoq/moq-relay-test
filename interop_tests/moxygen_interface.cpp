#include "moxygen_interface.h"
#include <iostream>

namespace moxygen_interface {

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
    std::shared_ptr<moxygen::SubscriptionHandle> subscriptionHandle) {

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
    const std::string &trackNamespace, const std::string &trackName,
    std::shared_ptr<moxygen::TrackConsumer> trackConsumer, uint8_t priority,
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

} // namespace moxygen_interface