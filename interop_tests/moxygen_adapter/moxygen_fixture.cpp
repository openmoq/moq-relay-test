#include "moxygen_fixture.h"
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
  std::cout << "  [Fixture] Starting cleanup..." << std::endl;

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

  std::cout << "  [Fixture] Cleanup completed" << std::endl;
}

std::shared_ptr<MoqtInterface> MoxygenTestFixture::getPublisher() {
  if (!publisher_) {
    std::cout << "  [Fixture] Creating publisher connection..." << std::endl;
    publisher_ = std::make_shared<MoxygenInterface>(eventBase_);

    try {
      bool connected =
          folly::coro::blockingWait(publisher_->connect(relayUrl_));
      if (!connected) {
        std::cerr << "  [Fixture] Failed to connect publisher" << std::endl;
        publisher_.reset();
        throw std::runtime_error("Failed to establish publisher connection");
      }
      std::cout << "  [Fixture] Publisher connected successfully" << std::endl;
    } catch (const std::exception &ex) {
      std::cerr << "  [Fixture] Exception connecting publisher: " << ex.what()
                << std::endl;
      publisher_.reset();
      throw;
    }
  }

  return publisher_;
}

std::shared_ptr<MoqtInterface> MoxygenTestFixture::getSubscriber() {
  if (!subscriber_) {
    std::cout << "  [Fixture] Creating subscriber connection..." << std::endl;
    subscriber_ = std::make_shared<MoxygenInterface>(eventBase_);

    try {
      bool connected =
          folly::coro::blockingWait(subscriber_->connect(relayUrl_));
      if (!connected) {
        std::cerr << "  [Fixture] Failed to connect subscriber" << std::endl;
        subscriber_.reset();
        throw std::runtime_error("Failed to establish subscriber connection");
      }
      std::cout << "  [Fixture] Subscriber connected successfully" << std::endl;
    } catch (const std::exception &ex) {
      std::cerr << "  [Fixture] Exception connecting subscriber: " << ex.what()
                << std::endl;
      subscriber_.reset();
      throw;
    }
  }

  return subscriber_;
}

std::shared_ptr<MoqtInterface>
MoxygenTestFixture::createMoQInterface(bool autoConnect) {
  std::cout << "\n[INTERFACE] Creating new MoQ interface..." << std::endl;

  auto interface = std::make_shared<MoxygenInterface>(eventBase_);

  if (autoConnect) {
    try {
      bool connected = folly::coro::blockingWait(interface->connect(relayUrl_));
      if (!connected) {
        std::cerr << "  [Fixture] Failed to connect new interface" << std::endl;
        throw std::runtime_error("Failed to establish connection");
      }
      std::cout << "  [Fixture] New interface connected successfully"
                << std::endl;
    } catch (const std::exception &ex) {
      std::cerr << "  [Fixture] Exception connecting interface: " << ex.what()
                << std::endl;
      throw;
    }
  }

  // Track additional interfaces for cleanup
  additionalInterfaces_.push_back(interface);

  return interface;
}

void MoxygenTestFixture::resetPublisher() {
  if (publisher_) {
    std::cout << "  [Fixture] Resetting publisher..." << std::endl;
    cleanupInterface(publisher_, "publisher");
  }
}

void MoxygenTestFixture::resetSubscriber() {
  if (subscriber_) {
    std::cout << "  [Fixture] Resetting subscriber..." << std::endl;
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
      std::cout << "  [Fixture] Closing " << name << " session..." << std::endl;

      eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait([client,
                                                               &name]() {
        try {
          client->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
          std::cout << "  [Fixture] " << name << " session closed" << std::endl;
        } catch (const std::exception &ex) {
          std::cerr << "  [Fixture] Exception closing " << name
                    << " session: " << ex.what() << std::endl;
        }
      });

      // Give time for graceful shutdown
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } catch (const std::exception &ex) {
    std::cerr << "  [Fixture] Exception during " << name
              << " cleanup: " << ex.what() << std::endl;
  }

  interface.reset();
}

} // namespace interop_test
