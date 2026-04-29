#include "moxygen_fixture.h"
#include <glog/logging.h>
#include "base/base_test.h"
#include <folly/coro/BlockingWait.h>
#include <iostream>
#include <moxygen/MoQSession.h>
#include <thread>

namespace interop_test {

MoxygenTestFixture::MoxygenTestFixture(const TestContext &context)
    : eventBase_(context.eventBase), relayUrl_(context.relayUrl),
      timeout_(context.defaultTimeout) {}

void MoxygenTestFixture::setUp() {
  // Currently no global setup needed
  // Resources are created lazily on demand
}

void MoxygenTestFixture::tearDown() {
  LOG(INFO) << "  [Fixture] Starting cleanup...";

  // Clean up all additional interfaces
  for (auto &interface : additionalInterfaces_) {
    cleanupInterface(interface, "additional");
  }
  additionalInterfaces_.clear();

  // Clean up subscriber
  if (subscriber_) {
    cleanupInterface(subscriber_, "subscriber");
  }

  // Clean up publisher
  if (publisher_) {
    cleanupInterface(publisher_, "publisher");
  }

  LOG(INFO) << "  [Fixture] Cleanup completed";
}

std::shared_ptr<MoqtInterface> MoxygenTestFixture::getPublisher() {
  if (!publisher_) {
    LOG(INFO) << "  [Fixture] Creating publisher connection...";
    publisher_ = std::make_shared<MoxygenInterface>(eventBase_);

    try {
      bool connected = publisher_->connect(relayUrl_);
      if (!connected) {
        LOG(ERROR) << "  [Fixture] Failed to connect publisher";
        publisher_.reset();
        throw std::runtime_error("Failed to establish publisher connection");
      }
      LOG(INFO) << "  [Fixture] Publisher connected successfully";
    } catch (const std::exception &ex) {
      LOG(ERROR) << "  [Fixture] Exception connecting publisher: " << ex.what();
      publisher_.reset();
      throw;
    }
  }

  return publisher_;
}

std::shared_ptr<MoqtInterface> MoxygenTestFixture::getSubscriber() {
  if (!subscriber_) {
    LOG(INFO) << "  [Fixture] Creating subscriber connection...";
    subscriber_ = std::make_shared<MoxygenInterface>(eventBase_);

    try {
      bool connected = subscriber_->connect(relayUrl_);
      if (!connected) {
        LOG(ERROR) << "  [Fixture] Failed to connect subscriber";
        subscriber_.reset();
        throw std::runtime_error("Failed to establish subscriber connection");
      }
      LOG(INFO) << "  [Fixture] Subscriber connected successfully";
    } catch (const std::exception &ex) {
      LOG(ERROR) << "  [Fixture] Exception connecting subscriber: " << ex.what();
      subscriber_.reset();
      throw;
    }
  }

  return subscriber_;
}

std::shared_ptr<MoqtInterface>
MoxygenTestFixture::createMoQInterface(bool autoConnect) {
  LOG(INFO) << "\n[INTERFACE] Creating new MoQ interface...";

  auto interface = std::make_shared<MoxygenInterface>(eventBase_);

  if (autoConnect) {
    try {
      bool connected = interface->connect(relayUrl_);
      if (!connected) {
        LOG(ERROR) << "  [Fixture] Failed to connect new interface";
        throw std::runtime_error("Failed to establish connection");
      }
      LOG(INFO) << "  [Fixture] New interface connected successfully";
    } catch (const std::exception &ex) {
      LOG(ERROR) << "  [Fixture] Exception connecting interface: " << ex.what();
      throw;
    }
  }

  // Track additional interfaces for cleanup
  additionalInterfaces_.push_back(interface);

  return interface;
}

void MoxygenTestFixture::resetPublisher() {
  if (publisher_) {
    LOG(INFO) << "  [Fixture] Resetting publisher...";
    cleanupInterface(publisher_, "publisher");
  }
}

void MoxygenTestFixture::resetSubscriber() {
  if (subscriber_) {
    LOG(INFO) << "  [Fixture] Resetting subscriber...";
    cleanupInterface(subscriber_, "subscriber");
  }
}

void MoxygenTestFixture::cleanupInterface(std::shared_ptr<MoxygenInterface> &interface,
                                   const std::string &name) {
  if (!interface) {
    return;
  }

  try {
    auto client = interface->getClient();
    if (eventBase_ && client && client->moqSession_) {
      LOG(INFO) << "  [Fixture] Closing " << name << " session...";
      eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait([client,
                                                               &name]() {
        try {
          // Check again if session is still valid before closing
          // to avoid use-after-free if goaway already closed it
          if (client && client->moqSession_) {
            client->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
            LOG(INFO) << "  [Fixture] " << name << " session closed";
          } else {
            LOG(INFO) << "  [Fixture] " << name << " session already closed";
          }
        } catch (const std::exception &ex) {
          LOG(ERROR) << "  [Fixture] Exception closing " << name
                    << " session: " << ex.what();
        }
      });
      // Give time for graceful shutdown
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } catch (const std::exception &ex) {
    LOG(ERROR) << "  [Fixture] Exception during " << name
              << " cleanup: " << ex.what();
  }

  interface.reset();
  LOG(INFO) << "  [Fixture] " << name << " interface reset";
}

} // namespace interop_test
