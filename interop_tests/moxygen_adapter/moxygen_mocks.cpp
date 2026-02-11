#include "moxygen_mocks.h"

#include <folly/io/async/EventBase.h>
#include <folly/executors/GlobalExecutor.h>
#include <iostream>

namespace interop_test {

// ============================================================================
// MockSubscriptionHandle implementations
// ============================================================================
MockSubscriptionHandle::MockSubscriptionHandle(moxygen::SubscribeOk ok)
    : moxygen::SubscriptionHandle(std::move(ok)) {}

MockSubscriptionHandle::MockSubscriptionHandle(
    moxygen::SubscribeOk ok,
    std::shared_ptr<moxygen::TrackConsumer> consumer)
    : moxygen::SubscriptionHandle(std::move(ok)),
      trackConsumer_(std::move(consumer)) {}

void MockSubscriptionHandle::unsubscribe() {
  std::cout << "MockSubscriptionHandle::unsubscribe() called" << std::endl;
  unsubscribe_called_ = true;
}

folly::coro::Task<MockSubscriptionHandle::SubscribeUpdateResult>
MockSubscriptionHandle::subscribeUpdate(moxygen::SubscribeUpdate subUpdate) {
  std::cout
      << "MockSubscriptionHandle::subscribeUpdate() called with request ID: "
      << subUpdate.requestID
      << ", forward: " << (subUpdate.forward ? "true" : "false")
      << std::endl;

  // If forward subscription and we haven't sent data yet, start sending mock data
  if (subUpdate.forward && !forward_data_sent_ && trackConsumer_) {
    std::cout << "MockSubscriptionHandle: Forward subscription detected, sending mock data"
              << std::endl;
    forward_data_sent_ = true;  // Mark as sent to prevent duplicate sends
    folly::coro::co_withExecutor(
        folly::getGlobalCPUExecutor(),
        sendMockDataViaTrackConsumer(trackConsumer_, subUpdate.requestID))
        .start();
  }

  // Return a successful result
  co_return moxygen::SubscribeUpdateOk{subUpdate.requestID};
}

// ============================================================================
// MockFetchHandle implementations
// ============================================================================
MockFetchHandle::MockFetchHandle(moxygen::FetchOk ok)
    : moxygen::Publisher::FetchHandle(std::move(ok)) {}

void MockFetchHandle::fetchCancel() {
  std::cout << "MockFetchHandle::fetchCancel() called" << std::endl;
  cancel_called_ = true;
}

// ============================================================================
// MockTrackConsumer implementations
// ============================================================================
folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::setTrackAlias(moxygen::TrackAlias alias) {
  std::cout << "MockTrackConsumer::setTrackAlias: " << alias.value << std::endl;
  return folly::unit;
}

folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>,
                moxygen::MoQPublishError>
MockTrackConsumer::beginSubgroup(uint64_t groupID, uint64_t subgroupID,
                                 moxygen::Priority priority,
                                 bool containsLastInGroup) {
  if (publishDone_received_) {
    std::cerr << "MockTrackConsumer::beginSubgroup called after publishDone!" << std::endl;
    return folly::makeUnexpected(moxygen::MoQPublishError(
        moxygen::MoQPublishError::API_ERROR, "Trying to publish after publishDone"));
  }
  std::cout << "MockTrackConsumer::beginSubgroup - Group: " << groupID
            << ", Subgroup: " << subgroupID 
            << ", containsLastInGroup: " << containsLastInGroup << std::endl;
  return std::make_shared<MockSubgroupConsumer>();
}

folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
MockTrackConsumer::awaitStreamCredit() {
  return folly::makeSemiFuture(folly::unit);
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::objectStream(const moxygen::ObjectHeader &header,
                                moxygen::Payload payload,
                                bool lastInGroup) {
  if (publishDone_received_) {
    std::cerr << "MockTrackConsumer::objectStream called after publishDone!" << std::endl;
    return folly::makeUnexpected(moxygen::MoQPublishError(
        moxygen::MoQPublishError::API_ERROR, "Trying to publish after publishDone"));
  }
  uint64_t payloadSize = 0;
  if (payload) {
    payloadSize = payload->computeChainDataLength();
  }
  std::cout << "MockTrackConsumer::objectStream RECEIVED - Group: " << header.group
            << ", Object: " << header.id 
            << ", PayloadSize: " << payloadSize
            << " bytes, lastInGroup: " << lastInGroup << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::datagram(const moxygen::ObjectHeader &header,
                            moxygen::Payload payload,
                            bool lastInGroup) {
  if (publishDone_received_) {
    std::cerr << "MockTrackConsumer::datagram called after publishDone!" << std::endl;
    return folly::makeUnexpected(moxygen::MoQPublishError(
        moxygen::MoQPublishError::API_ERROR, "Trying to publish after publishDone"));
  }
  uint64_t payloadSize = 0;
  if (payload) {
    payloadSize = payload->computeChainDataLength();
  }
  std::cout << "MockTrackConsumer::datagram RECEIVED - Group: " << header.group
            << ", Object: " << header.id 
            << ", PayloadSize: " << payloadSize
            << " bytes, lastInGroup: " << lastInGroup << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::publishDone(moxygen::PublishDone pubDone) {
  std::cout << "MockTrackConsumer::publishDone RECEIVED" << std::endl;
  publishDone_received_ = true;
  return folly::unit;
}

// ============================================================================
// MockSubgroupConsumer implementations
// ============================================================================
folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockSubgroupConsumer::object(uint64_t objectID, moxygen::Payload payload,
                             moxygen::Extensions extensions, bool finSubgroup) {
  uint64_t payloadSize = 0;
  if (payload) {
    payloadSize = payload->computeChainDataLength();
  }
  std::cout << "MockSubgroupConsumer::object RECEIVED - Object: " << objectID
            << ", PayloadSize: " << payloadSize
            << " bytes, finSubgroup: " << finSubgroup << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockSubgroupConsumer::beginObject(uint64_t objectID, uint64_t length,
                                  moxygen::Payload initialPayload,
                                  moxygen::Extensions extensions) {
  uint64_t initialSize = 0;
  if (initialPayload) {
    initialSize = initialPayload->computeChainDataLength();
  }
  std::cout << "MockSubgroupConsumer::beginObject RECEIVED - Object: " << objectID
            << ", Length: " << length
            << ", InitialPayloadSize: " << initialSize << std::endl;
  return folly::unit;
}

folly::Expected<moxygen::ObjectPublishStatus, moxygen::MoQPublishError>
MockSubgroupConsumer::objectPayload(moxygen::Payload payload, bool finSubgroup) {
  uint64_t payloadSize = 0;
  if (payload) {
    payloadSize = payload->computeChainDataLength();
  }
  std::cout << "MockSubgroupConsumer::objectPayload RECEIVED - PayloadSize: "
            << payloadSize << " bytes, finSubgroup: " << finSubgroup << std::endl;
  return moxygen::ObjectPublishStatus::DONE;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockSubgroupConsumer::endOfGroup(uint64_t endOfGroupObjectID) {
  std::cout << "MockSubgroupConsumer::endOfGroup RECEIVED - ObjectID: "
            << endOfGroupObjectID << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockSubgroupConsumer::endOfTrackAndGroup(uint64_t endOfTrackObjectID) {
  std::cout << "MockSubgroupConsumer::endOfTrackAndGroup RECEIVED - ObjectID: "
            << endOfTrackObjectID << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockSubgroupConsumer::endOfSubgroup() {
  std::cout << "MockSubgroupConsumer::endOfSubgroup RECEIVED" << std::endl;
  return folly::unit;
}

void MockSubgroupConsumer::reset(moxygen::ResetStreamErrorCode error) {
  std::cout << "MockSubgroupConsumer::reset - Error: " << static_cast<uint32_t>(error)
            << std::endl;
}

folly::Expected<folly::SemiFuture<uint64_t>, moxygen::MoQPublishError>
MockSubgroupConsumer::awaitReadyToConsume() {
  return folly::makeSemiFuture<uint64_t>(0);
}

// ============================================================================
// MockFetchConsumer implementations
// ============================================================================
folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockFetchConsumer::object(uint64_t groupID, uint64_t subgroupID,
                          uint64_t objectID, moxygen::Payload payload,
                          moxygen::Extensions extensions, bool finFetch) {
  std::cout << "MockFetchConsumer::object - Group: " << groupID
            << ", Subgroup: " << subgroupID << ", Object: " << objectID
            << ", finFetch: " << finFetch << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockFetchConsumer::beginObject(uint64_t groupID, uint64_t subgroupID,
                               uint64_t objectID, uint64_t length,
                               moxygen::Payload initialPayload,
                               moxygen::Extensions extensions) {
  std::cout << "MockFetchConsumer::beginObject - Group: " << groupID
            << ", Subgroup: " << subgroupID << ", Object: " << objectID
            << ", Length: " << length << std::endl;
  return folly::unit;
}

folly::Expected<moxygen::ObjectPublishStatus, moxygen::MoQPublishError>
MockFetchConsumer::objectPayload(moxygen::Payload payload, bool finSubgroup) {
  std::cout << "MockFetchConsumer::objectPayload - finSubgroup: " << finSubgroup
            << std::endl;
  return moxygen::ObjectPublishStatus::DONE;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockFetchConsumer::endOfGroup(uint64_t groupID, uint64_t subgroupID,
                              uint64_t objectID, bool finFetch) {
  std::cout << "MockFetchConsumer::endOfGroup - Group: " << groupID
            << ", Subgroup: " << subgroupID << ", Object: " << objectID
            << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockFetchConsumer::endOfTrackAndGroup(uint64_t groupID, uint64_t subgroupID,
                                      uint64_t objectID) {
  std::cout << "MockFetchConsumer::endOfTrackAndGroup - Group: " << groupID
            << ", Subgroup: " << subgroupID << ", Object: " << objectID
            << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockFetchConsumer::endOfFetch() {
  std::cout << "MockFetchConsumer::endOfFetch" << std::endl;
  return folly::unit;
}

void MockFetchConsumer::reset(moxygen::ResetStreamErrorCode error) {
  std::cout << "MockFetchConsumer::reset - Error: " << static_cast<uint32_t>(error)
            << std::endl;
}

folly::Expected<folly::SemiFuture<uint64_t>, moxygen::MoQPublishError>
MockFetchConsumer::awaitReadyToConsume() {
  return folly::makeSemiFuture<uint64_t>(0);
}

// ============================================================================
// Utility functions to send mock data via TrackConsumer
// ============================================================================

// Using objectStream - for active publishing mode
folly::coro::Task<void> sendMockDataViaObjectStream(
    std::shared_ptr<moxygen::TrackConsumer> consumer,
    moxygen::RequestID requestID) {
  try {
    // Send 3 groups with 2 objects each
    for (uint64_t group = 0; group < 3; ++group) {
      for (uint64_t objectId = 0; objectId < 2; ++objectId) {
        // Create mock payload
        std::string mockData = "MockData-G" + std::to_string(group) + 
                              "-O" + std::to_string(objectId);
        auto payload = folly::IOBuf::copyBuffer(mockData);
        
        // Create object header
        moxygen::ObjectHeader header;
        header.group = group;
        header.subgroup = 0;
        header.id = objectId;
        header.length = payload->computeChainDataLength();
        header.status = moxygen::ObjectStatus::NORMAL;
        
        // Send via objectStream
        auto result = consumer->objectStream(header, std::move(payload));
        
        if (result.hasError()) {
          std::cerr << "Failed to send object: " 
                    << result.error().what() << std::endl;
          co_return;
        }
        
        std::cout << "Mock data sent: Group=" << group 
                  << " Object=" << objectId << std::endl;
      }
    }
  } catch (const std::exception& ex) {
    std::cerr << "Exception in mock data generation: " 
              << ex.what() << std::endl;
  }
}

// Using beginSubgroup - for responding to subscribe requests
folly::coro::Task<void> sendMockDataViaTrackConsumer(
    std::shared_ptr<moxygen::TrackConsumer> consumer,
    moxygen::RequestID requestID) {
  try {
    // Send 3 groups with 2 objects each
    for (uint64_t group = 0; group < 3; ++group) {
      // Begin a subgroup
      auto subgroupResult = consumer->beginSubgroup(group, 0, 128);
      if (subgroupResult.hasError()) {
        std::cerr << "Failed to begin subgroup: " 
                  << subgroupResult.error().what() << std::endl;
        co_return;
      }
      
      auto subgroup = *subgroupResult;
      
      // Send 2 objects in this subgroup
      for (uint64_t objectId = 0; objectId < 2; ++objectId) {
        // Create mock payload
        std::string mockData = "MockData-G" + std::to_string(group) + 
                              "-O" + std::to_string(objectId);
        auto payload = folly::IOBuf::copyBuffer(mockData);
        
        // Send object (finSubgroup = true on last object)
        bool isLast = (objectId == 1);
        auto objectResult = subgroup->object(
            objectId, 
            std::move(payload),
            moxygen::Extensions({}, {}),  // No extensions
            isLast
        );
        
        if (objectResult.hasError()) {
          std::cerr << "Failed to send object: " 
                    << objectResult.error().what() << std::endl;
          co_return;
        }
        
        std::cout << "Mock data sent: Group=" << group 
                  << " Object=" << objectId << std::endl;
      }
    }
    
  } catch (const std::exception& ex) {
    std::cerr << "Exception in mock data generation: " 
              << ex.what() << std::endl;
  }
}

// ============================================================================
// MockPublisher implementations
// ============================================================================
// MockPublisher::trackStatus implementation
// ============================================================================
folly::coro::Task<moxygen::Publisher::TrackStatusResult>
MockPublisher::trackStatus(const moxygen::TrackStatus trackStatus) {
  std::cout << "MockPublisher::trackStatus called for track: "
            << trackStatus.fullTrackName.trackName << " (namespace: "
            << trackStatus.fullTrackName.trackNamespace << ")"
            << " with requestID: " << trackStatus.requestID.value << std::endl;

  // Return a successful TrackStatusOk response
  moxygen::TrackStatusOk statusOk;
  statusOk.requestID = trackStatus.requestID;
  statusOk.trackAlias = moxygen::TrackAlias{trackStatus.requestID.value};
  statusOk.expires = std::chrono::milliseconds(0);
  statusOk.groupOrder = moxygen::GroupOrder::Default;
  statusOk.largest = moxygen::AbsoluteLocation{0, 0};
  statusOk.fullTrackName = trackStatus.fullTrackName;
  statusOk.statusCode = moxygen::TrackStatusCode::IN_PROGRESS;
  
  std::cout << "MockPublisher: Returning TrackStatusOk for request " 
            << trackStatus.requestID.value << std::endl;
  co_return statusOk;
}

// ============================================================================
// This handles an incoming subscribe request by sending some mock data via the TrackConsumer
// ============================================================================
folly::coro::Task<moxygen::Publisher::SubscribeResult>
MockPublisher::subscribe(moxygen::SubscribeRequest sub,
                         std::shared_ptr<moxygen::TrackConsumer> callback) {
  std::cout << "MockPublisher::subscribe called for track: "
            << sub.fullTrackName.trackName << " (namespace: "
            << sub.fullTrackName.trackNamespace << ")" << std::endl;
  std::cout << "  forward: " << (sub.forward ? "true" : "false")
            << std::endl;

  // Create a subscription handle with the callback as the TrackConsumer
  moxygen::SubscribeOk subscribeOk;
  subscribeOk.requestID = sub.requestID;
  subscribeOk.trackAlias = moxygen::TrackAlias(sub.requestID.value);
  subscribeOk.expires = std::chrono::milliseconds(0); // Never expires
  subscribeOk.groupOrder = sub.groupOrder;
  
  auto handle = std::make_shared<MockSubscriptionHandle>(subscribeOk, callback);
  
  // Set track alias on the consumer
  callback->setTrackAlias(subscribeOk.trackAlias);
  
  // Start sending data asynchronously only for forward subscriptions (relay scenario)
  if (sub.forward) {
    if (eventBase_) {
      // Schedule on EventBase thread for realistic behavior (matches real MoQSession)
      eventBase_->runInEventBaseThread([reqId = sub.requestID, callback, eventBase = eventBase_]() mutable {
        std::cout << "MockPublisher: Forward subscription detected, sending mock data" << std::endl;
        folly::coro::co_withExecutor(
            folly::getKeepAliveToken(eventBase),
            sendMockDataViaTrackConsumer(callback, reqId)
        ).start();
      });
    } else {
      // Fallback to CPU executor if no EventBase available
      std::cout << "MockPublisher: Forward subscription detected, sending mock data" << std::endl;
      folly::coro::co_withExecutor(
          folly::getGlobalCPUExecutor(),
          sendMockDataViaTrackConsumer(callback, sub.requestID)
      ).start();
    }
  } else {
    std::cout << "MockPublisher: Direct subscription, not sending mock data" << std::endl;
  }
  
  co_return handle;
}

// MockPublisher fetch implementation
folly::coro::Task<moxygen::Publisher::FetchResult>
MockPublisher::fetch(moxygen::Fetch fetchReq,
                     std::shared_ptr<moxygen::FetchConsumer> fetchCallback) {
  std::cout << "MockPublisher::fetch called for track: "
            << fetchReq.fullTrackName.trackName << " (namespace: "
            << fetchReq.fullTrackName.trackNamespace << ")" << std::endl;
  std::cout << "  requestID: " << fetchReq.requestID.value << std::endl;
  
  // Extract start/end from the fetch args if it's a standalone fetch
  auto [standaloneFetch, joiningFetch] = moxygen::fetchType(fetchReq);
  if (standaloneFetch) {
    std::cout << "  start: group=" << standaloneFetch->start.group 
              << ", object=" << standaloneFetch->start.object << std::endl;
    std::cout << "  end: group=" << standaloneFetch->end.group 
              << ", object=" << standaloneFetch->end.object << std::endl;
  }

  // Create a FetchOk response
  moxygen::FetchOk fetchOk;
  fetchOk.requestID = fetchReq.requestID;
  fetchOk.groupOrder = moxygen::GroupOrder::Default;
  fetchOk.endOfTrack = false;
  
  // Spawn a coroutine to send mock fetch data
  auto sendMockFetchData = [](std::shared_ptr<moxygen::FetchConsumer> consumer,
                               moxygen::Fetch req) 
      -> folly::coro::Task<void> {
    try {
      // Determine range to fetch
      uint64_t startGroup = 0;
      uint64_t endGroup = 2;
      uint64_t startObject = 0;
      
      auto [standaloneFetch, joiningFetch] = moxygen::fetchType(req);
      if (standaloneFetch) {
        startGroup = standaloneFetch->start.group;
        endGroup = standaloneFetch->end.group;
        startObject = standaloneFetch->start.object;
      }
      
      // Send objects for the requested range
      for (uint64_t group = startGroup; group <= endGroup && group <= startGroup + 2; ++group) {
        uint64_t objStart = (group == startGroup) ? startObject : 0;
        
        // Send 2 objects per group
        for (uint64_t objectId = objStart; objectId < objStart + 2; ++objectId) {
          // Create mock payload
          std::string mockData = "MockFetchData-G" + std::to_string(group) + 
                                "-O" + std::to_string(objectId);
          auto payload = folly::IOBuf::copyBuffer(mockData);
          
          // Determine if this is the last object
          bool isLastGroup = (group == endGroup || group == startGroup + 2);
          bool isLastObject = (objectId == objStart + 1);
          bool finFetch = isLastGroup && isLastObject;
          
          // Send object
          auto objectResult = consumer->object(
              group,
              0,  // subgroupID
              objectId,
              std::move(payload),
              moxygen::Extensions({}, {}),  // No extensions
              finFetch
          );
          
          if (objectResult.hasError()) {
            std::cerr << "Failed to send fetch object: " 
                      << objectResult.error().what() << std::endl;
            co_return;
          }
          
          std::cout << "MockPublisher sent fetch object: Group=" << group 
                    << " Object=" << objectId << " finFetch=" << finFetch << std::endl;
        }
      }
      
      // Signal end of fetch
      std::cout << "MockPublisher: Sending endOfFetch" << std::endl;
      consumer->endOfFetch();
      
    } catch (const std::exception& ex) {
      std::cerr << "Exception in MockPublisher fetch data generation: " 
                << ex.what() << std::endl;
    }
  };
  
  // Start sending fetch data asynchronously
  if (eventBase_) {
    // Schedule on EventBase thread for realistic behavior (matches real MoQSession)
    eventBase_->runInEventBaseThread([callback = fetchCallback, req = fetchReq, sendMockFetchData, eventBase = eventBase_]() mutable {
      folly::coro::co_withExecutor(
          folly::getKeepAliveToken(eventBase),
          sendMockFetchData(callback, req)
      ).start();
    });
  } else {
    // Fallback to CPU executor if no EventBase available
    folly::coro::co_withExecutor(
        folly::getGlobalCPUExecutor(),
        [callback = fetchCallback, req = fetchReq, sendMockFetchData]() mutable -> folly::coro::Task<void> {
          co_await sendMockFetchData(callback, req);
        }()
    ).start();
  }
  
  // Create and return a fetch handle
  auto handle = std::make_shared<MockFetchHandle>(fetchOk);
  co_return handle;
}

// ============================================================================
// MockSubscriber implementations
// ============================================================================
moxygen::Subscriber::PublishResult
MockSubscriber::publish(moxygen::PublishRequest pub,
                        std::shared_ptr<moxygen::SubscriptionHandle> handle) {
  std::cout << "MockSubscriber::publish called for track: "
            << pub.fullTrackName.trackName << " (namespace: "
            << pub.fullTrackName.trackNamespace << ")" << std::endl;

  // For now, return NOT_SUPPORTED as we don't expect to receive PUBLISH
  // In a real relay scenario, you might accept this and create a consumer
  // Can we use publish_namespace to test this?
  return folly::makeUnexpected(moxygen::PublishError{
      pub.requestID,
      moxygen::PublishErrorCode::NOT_SUPPORTED,
      "Mock subscriber does not accept PUBLISH requests"});
}

} // namespace interop_test