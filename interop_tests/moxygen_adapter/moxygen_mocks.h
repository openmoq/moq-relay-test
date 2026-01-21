#pragma once

#include <folly/coro/Task.h>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <moxygen/Publisher.h>

namespace interop_test {

// Simple test subscription handle for publish/subscribe tests
class TestSubscriptionHandle : public moxygen::SubscriptionHandle {
public:
  TestSubscriptionHandle() = default;
  explicit TestSubscriptionHandle(moxygen::SubscribeOk ok);
  
  void unsubscribe() override;
  bool wasUnsubscribed() const { return unsubscribe_called_; }
  folly::coro::Task<SubscribeUpdateResult>
  subscribeUpdate(moxygen::SubscribeUpdate subUpdate) override;

private:
  bool unsubscribe_called_{false};
};

// Simple TrackConsumer implementation for testing
class TestTrackConsumer : public moxygen::TrackConsumer {
public:
  TestTrackConsumer() = default;
  ~TestTrackConsumer() override = default;

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

} // namespace interop_test
