#include "moqt_interface.h"
#include <iostream>

namespace interop_test {

folly::coro::Task<bool> MoqtInterface::connect(
    const std::string &url, std::chrono::milliseconds connectTimeout,
    std::chrono::milliseconds transactionTimeout, bool useInsecureVerifier) {
  std::cerr << "MoqtInterface::connect - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool> MoqtInterface::publish(
    const std::string &trackNamespace, const std::string &trackName) {
  std::cerr << "MoqtInterface::publish - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool> MoqtInterface::subscribe(
    const std::string &trackNamespace, const std::string &trackName,
    uint8_t priority, GroupOrder groupOrder) {
  std::cerr << "MoqtInterface::subscribe - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool> MoqtInterface::subscribeUpdate(
    const std::string &trackNamespace, const std::string &trackName,
    uint8_t priority, GroupOrder groupOrder, AbsoluteLocation start,
    uint8_t endGroup) {
  std::cerr << "MoqtInterface::subscribeUpdate - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool>
MoqtInterface::publish_namespace(const std::string &trackNamespace) {
  std::cerr << "MoqtInterface::publish_namespace - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool>
MoqtInterface::publish_namespace_done(const std::string &trackNamespace) {
  std::cerr << "MoqtInterface::publish_namespace_done - Not implemented"
            << std::endl;
  co_return false;
}

folly::coro::Task<bool>
MoqtInterface::subscribe_namespace(const std::string &trackNamespace) {
  std::cerr << "MoqtInterface::subscribe_namespace - Not implemented"
            << std::endl;
  co_return false;
}

folly::coro::Task<bool>
MoqtInterface::trackStatus(const std::string &trackNamespace,
                           const std::string &trackName) {
  std::cerr << "MoqtInterface::trackStatus - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool> MoqtInterface::goaway() {
  std::cerr << "MoqtInterface::goaway - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool> MoqtInterface::goaway_sequence() {
  std::cerr << "MoqtInterface::goaway_sequence - Not implemented" << std::endl;
  co_return false;
}

folly::coro::Task<bool>
MoqtInterface::setMaxConcurrentRequests(uint32_t maxConcurrentRequests) {
  std::cerr << "MoqtInterface::setMaxConcurrentRequests - Not implemented"
            << std::endl;
  co_return false;
}

bool MoqtInterface::isConnected() const {
  std::cerr << "MoqtInterface::isConnected - Not implemented" << std::endl;
  return false;
}

} // namespace interop_test
