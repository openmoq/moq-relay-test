#pragma once

#include <folly/coro/Task.h>
#include <folly/io/IOBuf.h>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <moxygen/Publisher.h>
#include <moxygen/Subscriber.h>

namespace interop_test {

// Utility function to create mock data buffers
inline std::unique_ptr<folly::IOBuf> makeMockData(uint32_t size) {
  auto buf = folly::IOBuf::create(size);
  buf->append(size);
  // Fill with pattern for debugging
  auto data = buf->writableData();
  for (uint32_t i = 0; i < size; ++i) {
    data[i] = static_cast<uint8_t>('A' + (i % 26));
  }
  return buf;
}

// Utility function to send mock data via TrackConsumer
// Sends 3 groups with 2 objects each using beginSubgroup pattern
folly::coro::Task<void> sendMockDataViaTrackConsumer(
    std::shared_ptr<moxygen::TrackConsumer> consumer,
    moxygen::RequestID requestID);

// Utility function to send mock data via TrackConsumer using objectStream
// Use this when beginSubgroup is not available (e.g., for active publishing)
folly::coro::Task<void> sendMockDataViaObjectStream(
    std::shared_ptr<moxygen::TrackConsumer> consumer,
    moxygen::RequestID requestID);

// Simple test subscription handle for publish/subscribe tests
class MockSubscriptionHandle : public moxygen::SubscriptionHandle {
public:
  MockSubscriptionHandle() = default;
  explicit MockSubscriptionHandle(moxygen::SubscribeOk ok);

  void unsubscribe() override;
  bool wasUnsubscribed() const { return unsubscribe_called_; }
  folly::coro::Task<SubscribeUpdateResult>
  subscribeUpdate(moxygen::SubscribeUpdate subUpdate) override;

private:
  bool unsubscribe_called_{false};
};

// Simple test fetch handle for fetch tests
class MockFetchHandle : public moxygen::Publisher::FetchHandle {
public:
  MockFetchHandle() = default;
  explicit MockFetchHandle(moxygen::FetchOk ok);

  void fetchCancel() override;
  bool wasCancelled() const { return cancel_called_; }

private:
  bool cancel_called_{false};
};

// Simple TrackConsumer implementation for testing
class MockTrackConsumer : public moxygen::TrackConsumer {
public:
  MockTrackConsumer() = default;
  ~MockTrackConsumer() override = default;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  setTrackAlias(moxygen::TrackAlias alias) override;

  folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>,
                  moxygen::MoQPublishError>
  beginSubgroup(uint64_t groupID, uint64_t subgroupID,
                moxygen::Priority priority) override;

  folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
  awaitStreamCredit() override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  objectStream(const moxygen::ObjectHeader &header,
               moxygen::Payload payload) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  datagram(const moxygen::ObjectHeader &header,
           moxygen::Payload payload) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  groupNotExists(uint64_t groupID, uint64_t subgroup,
                 moxygen::Priority pri) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  subscribeDone(moxygen::SubscribeDone subDone) override;
};

// Simple FetchConsumer implementation for testing
class MockFetchConsumer : public moxygen::FetchConsumer {
public:
  MockFetchConsumer() = default;
  ~MockFetchConsumer() override = default;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  object(uint64_t groupID, uint64_t subgroupID, uint64_t objectID,
         moxygen::Payload payload, moxygen::Extensions extensions,
         bool finFetch) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  objectNotExists(uint64_t groupID, uint64_t subgroupID, uint64_t objectID,
                  bool finFetch) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  groupNotExists(uint64_t groupID, uint64_t subgroupID, bool finFetch) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  beginObject(uint64_t groupID, uint64_t subgroupID, uint64_t objectID,
              uint64_t length, moxygen::Payload initialPayload,
              moxygen::Extensions extensions) override;

  folly::Expected<moxygen::ObjectPublishStatus, moxygen::MoQPublishError>
  objectPayload(moxygen::Payload payload, bool finSubgroup) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  endOfGroup(uint64_t groupID, uint64_t subgroupID, uint64_t objectID,
             bool finFetch) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  endOfTrackAndGroup(uint64_t groupID, uint64_t subgroupID,
                     uint64_t objectID) override;

  folly::Expected<folly::Unit, moxygen::MoQPublishError> endOfFetch() override;

  void reset(moxygen::ResetStreamErrorCode error) override;

  folly::Expected<folly::SemiFuture<uint64_t>, moxygen::MoQPublishError>
  awaitReadyToConsume() override;
};

// Mock Publisher implementation to handle incoming SUBSCRIBE requests
class MockPublisher : public moxygen::Publisher {
public:
  explicit MockPublisher(folly::EventBase* eventBase = nullptr) 
      : eventBase_(eventBase) {}
  ~MockPublisher() override = default;

  folly::coro::Task<SubscribeResult> subscribe(
      moxygen::SubscribeRequest sub,
      std::shared_ptr<moxygen::TrackConsumer> callback) override;

  folly::coro::Task<FetchResult> fetch(
      moxygen::Fetch fetchReq,
      std::shared_ptr<moxygen::FetchConsumer> fetchCallback) override;

private:
  folly::EventBase* eventBase_;
};

// Mock Subscriber implementation to handle incoming PUBLISH requests
class MockSubscriber : public moxygen::Subscriber {
public:
  MockSubscriber() = default;
  ~MockSubscriber() override = default;

  PublishResult publish(
      moxygen::PublishRequest pub,
      std::shared_ptr<moxygen::SubscriptionHandle> handle) override;
};

} // namespace interop_test
