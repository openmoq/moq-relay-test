#include "moxygen_interface.h"
#include "moxygen_mocks.h"
#include "type_conversions.h"
#include <folly/futures/Future.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Sleep.h>
#include <iostream>

namespace interop_test {

MoxygenInterface::MoxygenInterface(folly::EventBase *eventBase)
    : eventBase_(eventBase),
      publishHandler_(std::make_shared<MockPublisher>(eventBase)),
      subscribeHandler_(std::make_shared<MockSubscriber>()) {
  if (!eventBase_) {
    throw std::invalid_argument("EventBase cannot be null");
  }
}

MoxygenInterface::~MoxygenInterface() {
  // Reset in proper order to ensure cleanup doesn't deadlock
  relaySession_.reset();
  client_.reset();

  // Note: When using EventBaseThread, we don't need to manually drive the 
  // EventBase as it's already being driven by a background thread.
  // Just reset the executor.
  executor_.reset();
}

bool MoxygenInterface::connect(
    const std::string &url, std::chrono::milliseconds connectTimeout,
    std::chrono::milliseconds transactionTimeout, bool useInsecureVerifier) {
  
  try {
    // Create executor from the provided event base and store as member
    executor_ = std::make_shared<moxygen::MoQFollyExecutorImpl>(eventBase_);

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
    auto sessionFactory = moxygen::MoQRelaySession::createRelaySessionFactory();
    client_ = std::make_shared<moxygen::MoQClient>(
        folly::getKeepAliveToken(executor_.get()),
        std::move(parsedUrl),
        std::move(sessionFactory),
        verifier);

    // Set up MoQ session synchronously using blockingWait
    // publishHandler: handles incoming SUBSCRIBE requests (after we ANNOUNCE/PUBLISH)
    // subscribeHandler: handles incoming PUBLISH requests (not used right now)
    // Note: EventBase is driven by EventBaseThread, not by blockingWait
    folly::coro::blockingWait(
        client_->setupMoQSession(connectTimeout, transactionTimeout,
                                publishHandler_, subscribeHandler_, {}));

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
    std::shared_ptr<MockSubscriptionHandle> externalHandle,
    bool forward) {

  // _doPublish coroutine started
  std::cout << "[_doPublish] Starting: namespace=" << trackNamespace << " trackName=" << trackName << " forward=" << forward << std::endl;
  try {
    if (!isConnected()) {
      std::cerr << "No MoQ session available" << std::endl;
      co_return false;
    }
    // Session is connected

    auto session = client_->moqSession_;

    // Create publish request
    moxygen::PublishRequest publishReq;
    publishReq.fullTrackName =
        moxygen::FullTrackName{.trackNamespace = moxygen::TrackNamespace(
                                   std::vector<std::string>{trackNamespace}),
                               .trackName = trackName};
    publishReq.requestID = moxygen::RequestID{0};
    publishReq.groupOrder = moxygen::GroupOrder::Default;
    publishReq.forward = forward;

    std::cout << "[_doPublish] Created publish request: requestID=" << publishReq.requestID.value 
              << " forward=" << publishReq.forward << std::endl;

    subscriptionHandle_ = externalHandle
                                  ? externalHandle
                                  : std::make_shared<MockSubscriptionHandle>();

    std::cout << "[_doPublish] Sending publish request..." << std::endl;
    // Send publish request via session's publish method
    auto publishResult = session->publish(publishReq, subscriptionHandle_);

    std::cout << "[_doPublish] Got publish result, hasValue=" << publishResult.hasValue() << std::endl;
    if (publishResult.hasValue()) {
      auto publishConsumerAndReply = std::move(publishResult.value());

      // Store the track consumer for streaming data later
      publishTrackConsumer_ = std::move(publishConsumerAndReply.consumer);

      // Inject the track consumer into the subscription handle for subscribe updates
      subscriptionHandle_->setTrackConsumer(publishTrackConsumer_); 

      // Handle the PUBLISH_OK or PUBLISH_ERROR
      auto replyTask = std::move(publishConsumerAndReply.reply);
      auto replyResult = co_await std::move(replyTask);

      std::cout << "[_doPublish] Got reply result, hasValue=" << replyResult.hasValue() << std::endl;
      if (!replyResult.hasValue()) {
        auto error = replyResult.error();
        std::cerr << "[_doPublish] ERROR: Reply has error: " << error.reasonPhrase << std::endl;
        publishTrackConsumer_.reset();
        co_return false;
      }
      
      // Log the raw reply result before parsing
      std::cout << "[_doPublish] Reply result has value, size of PublishOk object: " << sizeof(replyResult.value()) << std::endl;
      
      auto publishOk = replyResult.value();
        std::cout << "[_doPublish] PublishOK received: requestID=" << publishOk.requestID.value 
                  << " forward=" << publishOk.forward 
                  << " groupOrder=" << static_cast<int>(publishOk.groupOrder)
                  << " locType=" << static_cast<int>(publishOk.locType) << std::endl;
        
        // Check if relay accepted forward mode - only send data if both requested and accepted
        if (forward && publishOk.forward && publishTrackConsumer_) {
          std::cout << "[_doPublish] Forward mode confirmed - starting to send data immediately" << std::endl;
          
          // Send data and await completion
          co_await sendMockDataViaObjectStream(publishTrackConsumer_, publishReq.requestID);
          std::cout << "[_doPublish] Mock data transmission completed" << std::endl;
        }
        co_return true;
    } else {
      auto error = publishResult.error();
      std::cerr << "[_doPublish] ERROR: Publish request failed: " << error.reasonPhrase
                << std::endl;
      co_return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "[_doPublish] EXCEPTION: " << ex.what() << std::endl;
    publishTrackConsumer_.reset();
    co_return false;
  }
}

bool MoxygenInterface::publish(
  const std::string &trackNamespace, const std::string &trackName, bool forward) {
  // Execute coroutine synchronously using blockingWait
  // EventBase is driven by EventBaseThread, not by blockingWait
  return folly::coro::blockingWait(
      _doPublish(trackNamespace, trackName, nullptr, forward));
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
    auto result = co_await session->subscribe(subscribeReq, trackConsumer);
    
    co_return result;
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
      subscriptionHandle_.reset();
      return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in subscribe: " << ex.what() << std::endl;
    subscriptionHandle_.reset();
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
    
    // If we have a publish track consumer, inject it so forward subscribeUpdate can use it
    if (publishTrackConsumer_) {
      auto mockHandle = std::dynamic_pointer_cast<MockSubscriptionHandle>(subscriptionHandle);
      if (mockHandle) {
        mockHandle->setTrackConsumer(publishTrackConsumer_);
      }
    }
    
    // Convert interop_test types to moxygen types
    auto moxygenStart = toMoxygenAbsoluteLocation(start);
    
    // Send subscribe update request
    moxygen::SubscribeUpdate subscribeUpdate;
    subscribeUpdate.requestID = moxygen::RequestID{2};
    subscribeUpdate.existingRequestID = moxygen::RequestID{0};
    // For draft < 15, start and endGroup are required
    subscribeUpdate.start = moxygenStart;
    subscribeUpdate.endGroup = endGroup;  
    subscribeUpdate.priority = priority;
    subscribeUpdate.forward = true;

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


bool MoxygenInterface::fetch(const std::string &trackNamespace, const std::string &trackName) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    // Create fetch request
    moxygen::Fetch fetchReq(
        moxygen::RequestID{0},
        moxygen::FullTrackName{.trackNamespace = moxygen::TrackNamespace(
                                   std::vector<std::string>{trackNamespace}),
                               .trackName = trackName},
        moxygen::AbsoluteLocation{0, 0},  // start location
        moxygen::AbsoluteLocation{},  // end location None
        128,  // priority
        moxygen::GroupOrder::OldestFirst  // groupOrder
    );
    
    std::cout << "Sending fetch request for track: " << trackNamespace << "/"
              << trackName << std::endl;
    
    // Create and store fetch consumer to keep it alive for async callbacks
    fetchConsumer_ = std::make_shared<MockFetchConsumer>();
    
    auto fetchResult = folly::coro::blockingWait(relaySession_->fetch(fetchReq, fetchConsumer_));
    if (fetchResult.hasValue()) {
      auto fetchHandle = fetchResult.value();
      std::cout << "Fetch OK received." << std::endl;
      return true;
    } else {
      auto error = fetchResult.error();
      std::cerr << "Fetch failed: " << error.reasonPhrase << std::endl;
      fetchConsumer_.reset();
      return false;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Exception in fetch: " << ex.what() << std::endl;
    fetchConsumer_.reset();
    return false;
  }
}

folly::coro::Task<moxygen::Subscriber::PublishNamespaceResult>
MoxygenInterface::_doPublishNamespace(const std::string &trackNamespace) {
  // Create publishNamespace request
  moxygen::PublishNamespace publishNamespace;
  publishNamespace.requestID = moxygen::RequestID{1};
  publishNamespace.trackNamespace =
      moxygen::TrackNamespace(std::vector<std::string>{trackNamespace});

  std::cout << "Sending publishNamespace request for namespace: " << trackNamespace
            << std::endl;

  co_return co_await relaySession_->publishNamespace(publishNamespace);
}

bool
MoxygenInterface::publish_namespace(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      std::cerr << "No MoQ relay session available" << std::endl;
      return false;
    }

    auto publishNamespaceResult = folly::coro::blockingWait(
      _doPublishNamespace(trackNamespace));

    if (publishNamespaceResult.hasValue()) {
      auto publishNamespaceHandle = std::move(publishNamespaceResult.value());
      std::cout << "PublishNamespace OK received for namespace: " << trackNamespace
                << std::endl;
      return true;
    } else {
      auto error = publishNamespaceResult.error();
      std::cerr << "PublishNamespace failed: " << error.reasonPhrase << std::endl;
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

    // First publishNamespace
    auto publishNamespaceResult = folly::coro::blockingWait(
      _doPublishNamespace(trackNamespace));
    if (!publishNamespaceResult.hasValue()) {
      auto error = publishNamespaceResult.error();
      std::cerr << "PublishNamespace failed before publishNamespaceDone: " << error.reasonPhrase
                << std::endl;
      return false;
    }

    auto publishNamespaceHandle_ = std::move(publishNamespaceResult.value());

    std::cout << "Sending publishNamespaceDone request for namespace: " << trackNamespace
              << std::endl;
    // Send publishNamespaceDone request
    publishNamespaceHandle_->publishNamespaceDone();
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

    // Create subscribe namespace request
    moxygen::SubscribeNamespace subscribeNamespace;
    subscribeNamespace.requestID = moxygen::RequestID{1};
    subscribeNamespace.trackNamespacePrefix =
        moxygen::TrackNamespace(std::vector<std::string>{trackNamespace});

    auto subscribeNamespaceResult_ = folly::coro::blockingWait(
      relaySession_->subscribeNamespace(subscribeNamespace, nullptr));

    if (!subscribeNamespaceResult_.hasValue()) {
      auto error = subscribeNamespaceResult_.error();
      std::cerr << "Subscribe namespace failed: " << error.reasonPhrase
                << std::endl;
      return false;
    } else {
      std::cout << "Subscribe namespace succeeded for namespace: "
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
                               .trackName = trackName},
                           .priority = 128,
                           .groupOrder = moxygen::GroupOrder::Default,
                           .forward = false,
                           .locType = moxygen::LocationType::LargestGroup,
                           .start = std::nullopt,
                           .endGroup = 0}));

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

    // Wait for the session to process the goaway and close gracefully.
    // This prevents a race condition where the relay closes the session
    // while we're still trying to clean it up in tearDown().
    // Using blockingWait on a small sleep to allow event base processing.
    std::cout << "Waiting for goaway to be processed..." << std::endl;
    folly::coro::blockingWait(folly::coro::sleep(std::chrono::milliseconds(100)));

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