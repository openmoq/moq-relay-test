#include "moxygen_interface.h"
#include "moxygen_mocks.h"
#include "type_conversions.h"
#include <folly/futures/Future.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Sleep.h>
#include <folly/coro/Coroutine.h>
#include <folly/Executor.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

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
  // Destroy QUIC-owned objects on the EventBase thread first, then drop the
  // executor (which just wraps the EventBase pointer and is safe to reset from
  // any thread).
  // resetClientOnEventBase();
  executor_.reset();
}

void MoxygenInterface::resetClientOnEventBase() {
  // Move shared_ptrs out so member variables are null immediately.
  // The actual destruction is deferred to the EventBase thread so that any
  // in-flight QUIC timer callbacks (e.g. idleTimeoutExpired -> onSessionEnd ->
  // ~MoQSession) complete before the objects are freed.
  auto client = std::move(client_);
  auto relaySession = std::move(relaySession_);

  if (!client) {
    return;
  }

  if (eventBase_) {
    // runImmediatelyOrRunInEventBaseThreadAndWait:
    //   - If called from the EventBase thread: runs inline (no deadlock).
    //   - If called from any other thread: posts to EventBase and blocks
    //     until the lambda returns, guaranteeing the destructor finishes
    //     before we proceed.
    eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait(
        [c = std::move(client), rs = std::move(relaySession)]() mutable {
          rs.reset();
          c.reset();
        });
  }
  // If eventBase_ is null, client/relaySession are destroyed here (stack
  // unwind) which is fine since there is no EventBase driving QUIC timers.
}

bool MoxygenInterface::connect(
    const std::string &url, std::chrono::milliseconds connectTimeout,
    std::chrono::milliseconds transactionTimeout, bool useInsecureVerifier) {
  
  try {
    std::cout << "[connect] Starting connection to " << url << std::endl;

    // blockingWait must NOT be called from the EventBase thread as the
    // coroutine relies on the EventBase to drive I/O completions.
    if (eventBase_->isInEventBaseThread()) {
      std::cerr << "[connect] FATAL: connect() called from EventBase thread - "
                   "blockingWait would deadlock!" << std::endl;
      return false;
    }

    // Create executor from the provided event base and store as member
    executor_ = std::make_shared<moxygen::MoQFollyExecutorImpl>(eventBase_);
    std::cout << "[connect] Created MoQFollyExecutorImpl" << std::endl;

    proxygen::URL parsedUrl(url);
    if (!parsedUrl.isValid()) {
      std::cerr << "Invalid URL: " << url << std::endl;
      return false;
    }
    std::cout << "[connect] URL parsed successfully" << std::endl;

    std::shared_ptr<fizz::CertificateVerifier> verifier = nullptr;
    if (useInsecureVerifier) {
      verifier = std::make_shared<fizz::InsecureAcceptAnyCertificate>();
      std::cout << "[connect] Using insecure certificate verifier" << std::endl;
    }

    // Create MoQ client with MoQRelaySession factory for announcement support
    auto sessionFactory = moxygen::MoQRelaySession::createRelaySessionFactory();
    std::cout << "[connect] Created relay session factory" << std::endl;
    
    client_ = std::make_shared<moxygen::MoQClient>(
        executor_,
        std::move(parsedUrl),
        std::move(sessionFactory),
        verifier);
    std::cout << "[connect] Created MoQClient" << std::endl;

    // Set up MoQ session synchronously using blockingWait.
    // NOTE: setupMoQSession internally uses EventBaseThreadTimekeeper in
    // QuicConnector::connectQuic, which shifts coroutine continuations onto
    // the EventBase thread.  blockingWait() is baton-based so it can pick up
    // completions posted from any thread — but we must NOT be on the EventBase
    // thread ourselves (checked above).
    folly::coro::blockingWait(
        client_->setupMoQSession(connectTimeout, transactionTimeout,
                                publishHandler_, subscribeHandler_, {},
                                {"moqt-16"}));

    // Cast once and store for later use
    relaySession_ = std::dynamic_pointer_cast<moxygen::MoQRelaySession>(
        client_->moqSession_);
    if (!relaySession_) {
      std::cerr << "Failed to cast to MoQRelaySession" << std::endl;
      client_.reset();
      return false;
    }
    std::cout << "[connect] Cast successful" << std::endl;

    std::cout << "MoQ session established successfully" << std::endl;
    return true;

  } catch (const std::exception &ex) {
    std::cerr << "Failed to establish MoQ session: " << ex.what() << std::endl;
    resetClientOnEventBase();
    return false;
  }
}
folly::coro::Task<bool> MoxygenInterface::_doPublish(
    const std::string &trackNamespace, const std::string &trackName,
    std::shared_ptr<MockSubscriptionHandle> externalHandle,
    bool forward) {

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

    subscriptionHandle_ = externalHandle
                                  ? externalHandle
                                  : std::make_shared<MockSubscriptionHandle>();
    
    // Send publish request via session's publish method
    auto publishResult = session->publish(publishReq, subscriptionHandle_);

    // Successful publish result with consumer and reply task
    if (publishResult.hasValue()) {
      // Keep the entire result alive
      auto publishConsumerAndReply = std::move(publishResult.value());

      // Store the track consumer for streaming data later
      publishTrackConsumer_ = std::move(publishConsumerAndReply.consumer);

      // Inject the track consumer into the subscription handle for subscribe updates
      if (subscriptionHandle_) {
        subscriptionHandle_->setTrackConsumer(publishTrackConsumer_);
      } else {
        std::cerr << "ERROR: subscriptionHandle_ is null" << std::endl;
        co_return false;
      }

      // Handle the PUBLISH_OK or PUBLISH_ERROR
      // Use co_awaitTry to safely handle any exceptions thrown by the reply coroutine
      auto replyResultTry = co_await folly::coro::co_awaitTry(std::move(publishConsumerAndReply.reply));
      
      if (replyResultTry.hasException()) {
        std::cerr << "ERROR: Exception waiting for publish reply" << std::endl;
        co_return false;
      }
      
      // Check if reply had an error (PUBLISH_ERROR response from relay)
      if (replyResultTry->hasError()) {
        std::cerr << "ERROR: Publish was rejected by relay" << std::endl;
        co_return false;
      }
      
      // At this point, we successfully got a PUBLISH_OK response
      // Check if relay accepted forward mode - only send data if requested
      if (forward && publishTrackConsumer_) {
        // Send mock data to the relay
        co_await sendMockDataViaObjectStream(publishTrackConsumer_, publishReq.requestID);
      }
      
      co_return true;
    } else {
      auto error = publishResult.error();
      std::cerr << "ERROR: Publish request failed: " << error.reasonPhrase
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
  // Schedule _doPublish on the EventBase thread so that session->publish()
  // (which writes to the non-thread-safe controlWriteBuf_) runs safely.
  // blockingWait is baton-based and can unblock when the coroutine posts its
  // result from the EventBase thread.
  return folly::coro::blockingWait(
      _doPublish(trackNamespace, trackName, nullptr, forward)
          .scheduleOn(eventBase_)
          .start());
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

    // Schedule on EventBase thread: session->subscribe() writes to the
    // non-thread-safe controlWriteBuf_ and must run on the EventBase thread.
    auto subscribeResult = folly::coro::blockingWait(
        _doSubscribe(trackNamespace, trackName, priority, moxygenGroupOrder)
            .scheduleOn(eventBase_)
            .start());

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

    // Schedule on EventBase thread: session->subscribe() writes to controlWriteBuf_.
    auto subscribeResult = folly::coro::blockingWait(
        _doSubscribe(trackNamespace, trackName, 128, moxygen::GroupOrder::OldestFirst)
            .scheduleOn(eventBase_)
            .start());

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
    auto result = folly::coro::blockingWait(
        subscriptionHandle->requestUpdate(subscribeUpdate)
            .scheduleOn(eventBase_)
            .start());
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
    
    auto fetchResult = folly::coro::blockingWait(
        relaySession_->fetch(fetchReq, fetchConsumer_)
            .scheduleOn(eventBase_)
            .start());
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
      _doPublishNamespace(trackNamespace)
          .scheduleOn(eventBase_)
          .start());

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
      _doPublishNamespace(trackNamespace)
          .scheduleOn(eventBase_)
          .start());
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
      relaySession_->subscribeNamespace(subscribeNamespace, nullptr)
          .scheduleOn(eventBase_)
          .start());

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

    auto result = folly::coro::blockingWait(
      relaySession_->trackStatus(
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
                           .endGroup = 0})
          .scheduleOn(eventBase_)
          .start());

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

    // goaway() writes to the non-thread-safe controlWriteBuf_;
    // dispatch it to the EventBase thread.
    auto session = relaySession_;
    eventBase_->runInEventBaseThreadAndWait(
        [session, goaway_inp]() mutable { session->goaway(goaway_inp); });
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
      _doPublish("dummy_namespace", "dummy_track", subscriptionHandle_)
          .scheduleOn(eventBase_)
          .start());
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

    // setMaxConcurrentRequests() calls sendMaxRequestID which writes to
    // controlWriteBuf_; dispatch to EventBase thread.
    auto session = relaySession_;
    eventBase_->runInEventBaseThreadAndWait(
        [session, maxConcurrentRequests]() {
          session->setMaxConcurrentRequests(maxConcurrentRequests);
        });
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