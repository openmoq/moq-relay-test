#pragma once

#include <chrono>
#include <folly/coro/Task.h>
#include <memory>
#include <string>
#include "moqt_types.h"

namespace interop_test {
/**
 * MoQT Interface - Abstract base class for MoQ Transport implementations
 * 
 * This interface defines the common operations for MoQ Transport protocol,
 * allowing different implementations (moxygen, etc.) to be used interchangeably.
 */
class MoqtInterface {
public:
  virtual ~MoqtInterface() = default;

  /**
   * Establishes a MoQ session with the given URL
   * @param url The MoQ relay server URL
   * @param connectTimeout Connection timeout
   * @param transactionTimeout Transaction timeout
   * @param useInsecureVerifier Whether to use insecure certificate verification
   * @return Task that resolves to true on success
   */
  virtual bool
  connect(const std::string &url,
          std::chrono::milliseconds connectTimeout =
              std::chrono::milliseconds(30000),
          std::chrono::milliseconds transactionTimeout =
              std::chrono::milliseconds(30000),
          bool useInsecureVerifier = true);

  /**
   * Sends a publish request to the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @return true on success
   */
  virtual bool
  publish(const std::string &trackNamespace, const std::string &trackName);

  /**
   * Subscribes to a track on the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @param priority Subscribe priority (default: 128)
   * @param groupOrder Group ordering preference (default: OldestFirst)
   * @return true on success
   */
  virtual bool
  subscribe(const std::string &trackNamespace, const std::string &trackName,
            uint8_t priority = 128,
            GroupOrder groupOrder = GroupOrder::OldestFirst);

  /**
   * Updates an existing subscription
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @param priority Subscribe priority (default: 128)
   * @param groupOrder Group ordering preference (default: OldestFirst)
   * @param start Starting location for the update
   * @param endGroup End group for the update
   * @return true on success
   */
  virtual bool
  subscribeUpdate(const std::string &trackNamespace,
                  const std::string &trackName,
                  uint8_t priority = 128,
                  GroupOrder groupOrder = GroupOrder::OldestFirst,
                  AbsoluteLocation start = AbsoluteLocation{0, 0},
                  uint8_t endGroup = 0);

  /**
   * Announces a namespace to the MoQ relay
   * @param trackNamespace The namespace to announce (e.g., "video/conference")
   * @return true on success
   */
  virtual bool
  publish_namespace(const std::string &trackNamespace);

  /**
   * Signals publish done for a namespace to the MoQ relay
   * @param trackNamespace The namespace to unannounce
   * @return true on success
   */
  virtual bool
  publish_namespace_done(const std::string &trackNamespace);

  /**
   * Subscribes to announces for a given namespace prefix
   * @param trackNamespace The namespace prefix to subscribe to
   * @return true on success
   */
  virtual bool
  subscribe_namespace(const std::string &trackNamespace);

  /**
   * Sends a track status request to the MoQ relay
   * @param trackNamespace The track namespace
   * @param trackName The track name
   * @return true on success
   */
  virtual bool
  trackStatus(const std::string &trackNamespace,
              const std::string &trackName);

  /**
   * Sends a goaway signal to the MoQ relay
   * @return true on success
   */
  virtual bool goaway();

  /**
   * Sends a goaway signal after publishing a dummy track
   * @return true on success
   */
  virtual bool goaway_sequence();

  /**
   * Sets the maximum number of concurrent requests
   * @param maxConcurrentRequests Maximum concurrent requests limit
   * @return true on success
   */
  virtual bool
  setMaxConcurrentRequests(uint32_t maxConcurrentRequests);

  /**
   * Checks if the interface is connected to a MoQ session
   * @return true if connected, false otherwise
   */
  virtual bool isConnected() const;
};

} // namespace interop_test
