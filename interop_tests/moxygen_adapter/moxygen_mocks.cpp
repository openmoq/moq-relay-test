#include "moxygen_mocks.h"

#include <iostream>

namespace interop_test {

// TestSubscriptionHandle implementations
TestSubscriptionHandle::TestSubscriptionHandle(moxygen::SubscribeOk ok)
    : moxygen::SubscriptionHandle(std::move(ok)) {}

void TestSubscriptionHandle::unsubscribe() {
  std::cout << "TestSubscriptionHandle::unsubscribe() called" << std::endl;
  unsubscribe_called_ = true;
}

folly::coro::Task<TestSubscriptionHandle::SubscribeUpdateResult>
TestSubscriptionHandle::subscribeUpdate(moxygen::SubscribeUpdate subUpdate) {
  std::cout
      << "TestSubscriptionHandle::subscribeUpdate() called with request ID: "
      << subUpdate.requestID << std::endl;

  // Return a successful result
  co_return moxygen::SubscribeUpdateOk{subUpdate.requestID};
}

// TestTrackConsumer implementations
folly::Expected<folly::Unit, moxygen::MoQPublishError>
TestTrackConsumer::setTrackAlias(moxygen::TrackAlias alias) {
  std::cout << "TestTrackConsumer::setTrackAlias: " << alias.value
            << std::endl;
  return folly::unit;
}

folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>,
                moxygen::MoQPublishError>
TestTrackConsumer::beginSubgroup(uint64_t groupID, uint64_t subgroupID,
                                 moxygen::Priority priority) {
  std::cout << "TestTrackConsumer::beginSubgroup - Group: " << groupID
            << ", Subgroup: " << subgroupID << std::endl;
  return folly::makeUnexpected(moxygen::MoQPublishError(
      moxygen::MoQPublishError::API_ERROR, "not implemented"));
}

folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
TestTrackConsumer::awaitStreamCredit() {
  return folly::makeSemiFuture(folly::unit);
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
TestTrackConsumer::objectStream(const moxygen::ObjectHeader &header,
                                moxygen::Payload payload) {
  std::cout << "TestTrackConsumer::objectStream - Group: " << header.group
            << ", Object: " << header.id << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
TestTrackConsumer::datagram(const moxygen::ObjectHeader &header,
                            moxygen::Payload payload) {
  std::cout << "TestTrackConsumer::datagram - Group: " << header.group
            << ", Object: " << header.id << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
TestTrackConsumer::groupNotExists(uint64_t groupID, uint64_t subgroup,
                                  moxygen::Priority pri) {
  std::cout << "TestTrackConsumer::groupNotExists - Group: " << groupID
            << std::endl;
  return folly::unit;
}

folly::Expected<folly::Unit, moxygen::MoQPublishError>
TestTrackConsumer::subscribeDone(moxygen::SubscribeDone subDone) {
  std::cout << "TestTrackConsumer::subscribeDone" << std::endl;
  return folly::unit;
}

} // namespace interop_test