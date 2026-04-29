#include "moxygen_interface.h"
#include <glog/logging.h>
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
    LOG(INFO) << "[connect] Starting connection to " << url;

    // blockingWait must NOT be called from the EventBase thread as the
    // coroutine relies on the EventBase to drive I/O completions.
    if (eventBase_->isInEventBaseThread()) {
      LOG(ERROR) << "[connect] FATAL: connect() called from EventBase thread - "
                   "blockingWait would deadlock!";
      return false;
    }

    // Create executor from the provided event base and store as member
    executor_ = std::make_shared<moxygen::MoQFollyExecutorImpl>(eventBase_);
    LOG(INFO) << "[connect] Created MoQFollyExecutorImpl";

    proxygen::URL parsedUrl(url);
    if (!parsedUrl.isValid()) {
      LOG(ERROR) << "Invalid URL: " << url;
      return false;
    }
    LOG(INFO) << "[connect] URL parsed successfully";

    std::shared_ptr<fizz::CertificateVerifier> verifier = nullptr;
    if (useInsecureVerifier) {
      verifier = std::make_shared<fizz::InsecureAcceptAnyCertificate>();
      LOG(INFO) << "[connect] Using insecure certificate verifier";
    }

    // Create MoQ client with MoQRelaySession factory for announcement support
    auto sessionFactory = moxygen::MoQRelaySession::createRelaySessionFactory();
    LOG(INFO) << "[connect] Created relay session factory";
    
    client_ = std::make_shared<moxygen::MoQClient>(
        executor_,
        std::move(parsedUrl),
        std::move(sessionFactory),
        verifier);
    LOG(INFO) << "[connect] Created MoQClient";

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
      LOG(ERROR) << "Failed to cast to MoQRelaySession";
      client_.reset();
      return false;
    }
    LOG(INFO) << "[connect] Cast successful";

    LOG(INFO) << "MoQ session established successfully";
    return true;

  } catch (const std::exception &ex) {
    LOG(ERROR) << "Failed to establish MoQ session: " << ex.what();
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
      LOG(ERROR) << "No MoQ session available";
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
        LOG(ERROR) << "ERROR: subscriptionHandle_ is null";
        co_return false;
      }

      // Handle the PUBLISH_OK or PUBLISH_ERROR
      // Use co_awaitTry to safely handle any exceptions thrown by the reply coroutine
      auto replyResultTry = co_await folly::coro::co_awaitTry(std::move(publishConsumerAndReply.reply));
      
      if (replyResultTry.hasException()) {
        LOG(ERROR) << "ERROR: Exception waiting for publish reply";
        co_return false;
      }
      
      // Check if reply had an error (PUBLISH_ERROR response from relay)
      if (replyResultTry->hasError()) {
        LOG(ERROR) << "ERROR: Publish was rejected by relay";
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
      LOG(ERROR) << "ERROR: Publish request failed: " << error.reasonPhrase;
      co_return false;
    }
  } catch (const std::exception &ex) {
    LOG(ERROR) << "[_doPublish] EXCEPTION: " << ex.what();
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

    LOG(INFO) << "Sending subscribe request for track: " << trackNamespace
              << "/" << trackName;

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
      LOG(ERROR) << "No MoQ session available";
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
      LOG(INFO) << "Subscribe OK received. Track alias: "
                << subscriptionHandle->subscribeOk().trackAlias.value;
      return true;
    } else {
      auto error = subscribeResult.error();
      LOG(ERROR) << "Subscribe failed: " << error.reasonPhrase;
      subscriptionHandle_.reset();
      return false;
    }
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in subscribe: " << ex.what();
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
      LOG(ERROR) << "No MoQ relay session available";
      return false;
    }

    // Schedule on EventBase thread: session->subscribe() writes to controlWriteBuf_.
    auto subscribeResult = folly::coro::blockingWait(
        _doSubscribe(trackNamespace, trackName, 128, moxygen::GroupOrder::OldestFirst)
            .scheduleOn(eventBase_)
            .start());

    if (!subscribeResult.hasValue()) {
      auto error = subscribeResult.error();
      LOG(ERROR) << "Subscribe failed before subscribe update: " << error.reasonPhrase;
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

    LOG(INFO) << "Sending subscribe update request";
    auto result = folly::coro::blockingWait(
        subscriptionHandle->requestUpdate(subscribeUpdate)
            .scheduleOn(eventBase_)
            .start());
    LOG(INFO) << "Subscribe update sent successfully";
    if (!result.hasValue()) {
      auto error = result.error();
      LOG(ERROR) << "Subscribe update failed: " << error.reasonPhrase;
      return false;
    } else {
      LOG(INFO) << "Subscribe update succeeded";
      return true;
    }
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in subscribe update: " << ex.what();
    return false;
  }
} 


bool MoxygenInterface::fetch(const std::string &trackNamespace, const std::string &trackName) {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
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
    
    LOG(INFO) << "Sending fetch request for track: " << trackNamespace << "/"
              << trackName;
    
    // Create and store fetch consumer to keep it alive for async callbacks
    fetchConsumer_ = std::make_shared<MockFetchConsumer>();
    
    auto fetchResult = folly::coro::blockingWait(
        relaySession_->fetch(fetchReq, fetchConsumer_)
            .scheduleOn(eventBase_)
            .start());
    if (fetchResult.hasValue()) {
      auto fetchHandle = fetchResult.value();
      LOG(INFO) << "Fetch OK received.";
      return true;
    } else {
      auto error = fetchResult.error();
      LOG(ERROR) << "Fetch failed: " << error.reasonPhrase;
      fetchConsumer_.reset();
      return false;
    }
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in fetch: " << ex.what();
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

  LOG(INFO) << "Sending publishNamespace request for namespace: " << trackNamespace;

  co_return co_await relaySession_->publishNamespace(publishNamespace);
}

bool
MoxygenInterface::publishNamespace(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
      return false;
    }

    auto publishNamespaceResult = folly::coro::blockingWait(
      _doPublishNamespace(trackNamespace)
          .scheduleOn(eventBase_)
          .start());

    if (publishNamespaceResult.hasValue()) {
      auto publishNamespaceHandle = std::move(publishNamespaceResult.value());
      LOG(INFO) << "PublishNamespace OK received for namespace: " << trackNamespace;
      return true;
    } else {
      auto error = publishNamespaceResult.error();
      LOG(ERROR) << "PublishNamespace failed: " << error.reasonPhrase;
      return false;
    }
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in publishNamespace: " << ex.what();
    return false;
  }
}

bool
MoxygenInterface::publishNamespaceDone(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
      return false;
    }

    // First publishNamespace
    auto publishNamespaceResult = folly::coro::blockingWait(
      _doPublishNamespace(trackNamespace)
          .scheduleOn(eventBase_)
          .start());
    if (!publishNamespaceResult.hasValue()) {
      auto error = publishNamespaceResult.error();
      LOG(ERROR) << "PublishNamespace failed before publishNamespaceDone: " << error.reasonPhrase;
      return false;
    }

    auto publishNamespaceHandle_ = std::move(publishNamespaceResult.value());

    LOG(INFO) << "Sending publishNamespaceDone request for namespace: " << trackNamespace;
    // Send publishNamespaceDone request
    publishNamespaceHandle_->publishNamespaceDone();
    return true;

  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in publishNamespaceDone: " << ex.what();
    return false;
  }
}

bool
MoxygenInterface::subscribeNamespace(const std::string &trackNamespace) {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
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
      LOG(ERROR) << "Subscribe namespace failed: " << error.reasonPhrase;
      return false;
    } else {
      LOG(INFO) << "Subscribe namespace succeeded for namespace: "
                << trackNamespace;
      return true;
    }

  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in subscribeNamespace: " << ex.what();
    return false;
  }
}

bool
MoxygenInterface::trackStatus(const std::string &trackNamespace,
                              const std::string &trackName) {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
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
      LOG(INFO) << "Track Status OK received for track: " << trackNamespace
                << "/" << trackName;
      return true;
    } else {
      auto error = result.error();
      LOG(ERROR) << "Track Status failed: " << error.reasonPhrase;
      return false;
    }
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in trackStatus: " << ex.what();
    return false;
  }
}

bool MoxygenInterface::goaway() {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
      return false;
    }

    LOG(INFO) << "Sending goaway after publish";
    moxygen::Goaway goaway_inp{.newSessionUri = ""};

    // goaway() writes to the non-thread-safe controlWriteBuf_;
    // dispatch it to the EventBase thread.
    auto session = relaySession_;
    eventBase_->runInEventBaseThreadAndWait(
        [session, goaway_inp]() mutable { session->goaway(goaway_inp); });
    LOG(INFO) << "Goaway sent successfully";

    return true;
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in goaway: " << ex.what();
    return false;
  }
}

bool MoxygenInterface::goawaySequence() {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
      return false;
    }

    // First, publish with a dummy track name
    LOG(INFO) << "Publishing dummy track before goaway";
    auto subscriptionHandle_ = std::make_shared<MockSubscriptionHandle>();
    bool publishSuccess = folly::coro::blockingWait(
      _doPublish("dummy_namespace", "dummy_track", subscriptionHandle_)
          .scheduleOn(eventBase_)
          .start());
    if (!publishSuccess) {
      LOG(ERROR) << "Failed to publish dummy track";
      return false;
    }

    // Then, send goaway
    bool goawaySuccess = goaway();
    if (!goawaySuccess) {
      LOG(ERROR) << "Failed to send goaway";
      return false;
    }

    // Wait for the session to process the goaway and close gracefully.
    // This prevents a race condition where the relay closes the session
    // while we're still trying to clean it up in tearDown().
    // Using blockingWait on a small sleep to allow event base processing.
    LOG(INFO) << "Waiting for goaway to be processed...";
    folly::coro::blockingWait(folly::coro::sleep(std::chrono::milliseconds(100)));

    LOG(INFO) << "Goaway sequence completed successfully";

    // After goaway, the session may close immediately, so we can't reliably
    // check if unsubscribe was called. Just return success if goaway sent.
    // Note that the protocol recommends waiting for a short period for
    // the unsubscribes.
    return true;

  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in goawaySequence: " << ex.what();
    return false;
  }
}

bool
MoxygenInterface::setMaxConcurrentRequests(uint32_t maxConcurrentRequests) {
  try {
    if (!isConnected() || !relaySession_) {
      LOG(ERROR) << "No MoQ relay session available";
      return false;
    }

    // setMaxConcurrentRequests() calls sendMaxRequestID which writes to
    // controlWriteBuf_; dispatch to EventBase thread.
    auto session = relaySession_;
    eventBase_->runInEventBaseThreadAndWait(
        [session, maxConcurrentRequests]() {
          session->setMaxConcurrentRequests(maxConcurrentRequests);
        });
    LOG(INFO) << "Set max concurrent requests to: " << maxConcurrentRequests;
    return true;

  } catch (const std::exception &ex) {
    LOG(ERROR) << "Exception in setMaxConcurrentRequests: " << ex.what();
    return false;
  }
}

} // namespace interop_test