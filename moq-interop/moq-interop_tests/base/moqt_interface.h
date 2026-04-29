#pragma once

#include <chrono>
#include <string>
#include "moqt_types.h"

namespace interop_test {
/**
 * MoQT Interface - Abstract base class for MoQ Transport implementations
 *
 * This interface defines the common operations for MoQ Transport protocol,
 * allowing different implementations (moxygen, etc.) to be used
 * interchangeably. All methods are pure virtual; concrete implementations
 * must override every method.
 */
class MoqtInterface {
public:
  virtual ~MoqtInterface() = default;

  virtual bool
  connect(const std::string &url,
          std::chrono::milliseconds connectTimeout =
              std::chrono::milliseconds(30000),
          std::chrono::milliseconds transactionTimeout =
              std::chrono::milliseconds(30000),
          bool useInsecureVerifier = true) = 0;

  virtual bool publish(const std::string &trackNamespace,
                       const std::string &trackName, bool forward = false) = 0;

  virtual bool
  subscribe(const std::string &trackNamespace, const std::string &trackName,
            uint8_t priority = 128,
            GroupOrder groupOrder = GroupOrder::OldestFirst) = 0;

  virtual bool
  subscribeUpdate(const std::string &trackNamespace,
                  const std::string &trackName, uint8_t priority = 128,
                  GroupOrder groupOrder = GroupOrder::OldestFirst,
                  AbsoluteLocation start = AbsoluteLocation{0, 0},
                  uint8_t endGroup = 0) = 0;

  virtual bool fetch(const std::string &trackNamespace,
                     const std::string &trackName) = 0;

  virtual bool publish_namespace(const std::string &trackNamespace) = 0;

  virtual bool publish_namespace_done(const std::string &trackNamespace) = 0;

  virtual bool subscribe_namespace(const std::string &trackNamespace) = 0;

  virtual bool trackStatus(const std::string &trackNamespace,
                           const std::string &trackName) = 0;

  virtual bool goaway() = 0;

  virtual bool goaway_sequence() = 0;

  virtual bool setMaxConcurrentRequests(uint32_t maxConcurrentRequests) = 0;

  virtual bool isConnected() const = 0;
};

} // namespace interop_test
