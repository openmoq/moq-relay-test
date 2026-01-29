#include "moxygen_mocks.h"

#include <iostream>

namespace interop_test {

// MockSubscriptionHandle implementations
MockSubscriptionHandle::MockSubscriptionHandle(moxygen::SubscribeOk ok)
    : moxygen::SubscriptionHandle(std::move(ok)) {}

void MockSubscriptionHandle::unsubscribe() {
  std::cout << "MockSubscriptionHandle::unsubscribe() called" << std::endl;
  unsubscribe_called_ = true;
}

folly::coro::Task<MockSubscriptionHandle::SubscribeUpdateResult>
MockSubscriptionHandle::subscribeUpdate(moxygen::SubscribeUpdate subUpdate) {
  std::cout
      << "MockSubscriptionHandle::subscribeUpdate() called with request ID: "
      << subUpdate.requestID << std::endl;

  // Return a successful result
  co_return moxygen::SubscribeUpdateOk{subUpdate.requestID};
}

// MockTrackConsumer implementations
folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::setTrackAlias(moxygen::TrackAlias alias) {
  std::cout << "MockTrackConsumer::setTrackAlias: " << alias.value << std::endl;
  return folly::unit;
}

folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>,
                moxygen::MoQPublishError>
MockTrackConsumer::beginSubgroup(uint64_t groupID, uint64_t subgroupID,
                                 moxygen::Priority priority) {
  std::cout << "MockTrackConsumer::beginSubgroup - Group: " << groupID
            << ", Subgroup: " << subgroupID << std::endl;
  return folly::makeUnexpected(moxygen::MoQPublishError(
      moxygen::MoQPublishError::API_ERROR, "not implemented"));
}

folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
MockTrackConsumer::awaitStreamCredit() {
  return folly::makeSemiFuture(folly::unit);
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::objectStream(const moxygen::ObjectHeader &header,
                                moxygen::Payload payload) {
  std::cout << "MockTrackConsumer::objectStream - Group: " << header.group
            << ", Object: " << header.id << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::datagram(const moxygen::ObjectHeader &header,
                            moxygen::Payload payload) {
  std::cout << "MockTrackConsumer::datagram - Group: " << header.group
            << ", Object: " << header.id << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::groupNotExists(uint64_t groupID, uint64_t subgroup,
                                  moxygen::Priority pri) {
  std::cout << "MockTrackConsumer::groupNotExists - Group: " << groupID
            << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
MockTrackConsumer::subscribeDone(moxygen::SubscribeDone subDone) {
  std::cout << "MockTrackConsumer::subscribeDone" << std::endl;
  return folly::unit;
}

} // namespace interop_test