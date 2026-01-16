#pragma once

#include <folly/coro/Task.h>
#include <iostream>
#include <memory>
#include <moxygen/MoQConsumers.h>
#include <moxygen/Publisher.h>

namespace interop_test {

enum class TestResult { PASS, FAIL, TIMEOUT, ERROR };

// Simple test subscription handle for publish/subscribe tests
class TestSubscriptionHandle : public moxygen::SubscriptionHandle {
public:
  TestSubscriptionHandle() = default;

  TestSubscriptionHandle(moxygen::SubscribeOk ok)
      : moxygen::SubscriptionHandle(std::move(ok)) {}

  void unsubscribe() override {
    std::cout << "TestSubscriptionHandle::unsubscribe() called" << std::endl;
  }

  folly::coro::Task<SubscribeUpdateResult>
  subscribeUpdate(moxygen::SubscribeUpdate subUpdate) override {
    std::cout
        << "TestSubscriptionHandle::subscribeUpdate() called with request ID: "
        << subUpdate.requestID << std::endl;

    // Return a successful result
    co_return moxygen::SubscribeUpdateOk{subUpdate.requestID};
  }
};

// Simple TrackConsumer implementation for testing
class TestTrackConsumer : public moxygen::TrackConsumer {
public:
  TestTrackConsumer() = default;
  ~TestTrackConsumer() override = default;

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  setTrackAlias(moxygen::TrackAlias alias) override {
    std::cout << "TestTrackConsumer::setTrackAlias: " << alias.value
              << std::endl;
    return folly::unit;
  }

  folly::Expected<std::shared_ptr<moxygen::SubgroupConsumer>,
                  moxygen::MoQPublishError>
  beginSubgroup(uint64_t groupID, uint64_t subgroupID,
                moxygen::Priority priority) override {
    std::cout << "TestTrackConsumer::beginSubgroup - Group: " << groupID
              << ", Subgroup: " << subgroupID << std::endl;
    return folly::makeUnexpected(moxygen::MoQPublishError(
        moxygen::MoQPublishError::API_ERROR, "not implemented"));
  }

  folly::Expected<folly::SemiFuture<folly::Unit>, moxygen::MoQPublishError>
  awaitStreamCredit() override {
    return folly::makeSemiFuture(folly::unit);
  }

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  objectStream(const moxygen::ObjectHeader &header,
               moxygen::Payload payload) override {
    std::cout << "TestTrackConsumer::objectStream - Group: " << header.group
              << ", Object: " << header.id << std::endl;
    return folly::unit;
  }

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  datagram(const moxygen::ObjectHeader &header,
           moxygen::Payload payload) override {
    std::cout << "TestTrackConsumer::datagram - Group: " << header.group
              << ", Object: " << header.id << std::endl;
    return folly::unit;
  }

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  groupNotExists(uint64_t groupID, uint64_t subgroup,
                 moxygen::Priority pri) override {
    std::cout << "TestTrackConsumer::groupNotExists - Group: " << groupID
              << std::endl;
    return folly::unit;
  }

  folly::Expected<folly::Unit, moxygen::MoQPublishError>
  subscribeDone(moxygen::SubscribeDone subDone) override {
    std::cout << "TestTrackConsumer::subscribeDone" << std::endl;
    return folly::unit;
  }
};

} // namespace interop_test
