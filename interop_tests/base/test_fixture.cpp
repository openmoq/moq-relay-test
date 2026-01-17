#include "test_fixture.h"
#include "base_test.h"
#include <iostream>
#include <thread>
#include <folly/coro/BlockingWait.h>
#include <moxygen/MoQSession.h>

namespace interop_test {

TestFixture::TestFixture(const TestContext& context)
    : eventBase_(context.eventBase)
    , relayUrl_(context.relayUrl)
    , timeout_(context.defaultTimeout) {
}

void TestFixture::setUp() {
    // Currently no global setup needed
    // Resources are created lazily on demand
}

void TestFixture::tearDown() {
    std::cout << "  [Fixture] Starting cleanup..." << std::endl;

    // Clean up all additional interfaces
    for (auto& interface : additionalInterfaces_) {
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

std::shared_ptr<moq_interface::MoQInterface> TestFixture::getPublisher() {
    if (!publisher_) {
        std::cout << "  [Fixture] Creating publisher connection..." << std::endl;
        publisher_ = std::make_shared<moq_interface::MoQInterface>(eventBase_);

        try {
            bool connected = folly::coro::blockingWait(publisher_->connect(relayUrl_));
            if (!connected) {
                std::cerr << "  [Fixture] Failed to connect publisher" << std::endl;
                publisher_.reset();
                throw std::runtime_error("Failed to establish publisher connection");
            }
            std::cout << "  [Fixture] Publisher connected successfully" << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "  [Fixture] Exception connecting publisher: " << ex.what() << std::endl;
            publisher_.reset();
            throw;
        }
    }

    return publisher_;
}

std::shared_ptr<moq_interface::MoQInterface> TestFixture::getSubscriber() {
    if (!subscriber_) {
        std::cout << "  [Fixture] Creating subscriber connection..." << std::endl;
        subscriber_ = std::make_shared<moq_interface::MoQInterface>(eventBase_);

        try {
            bool connected = folly::coro::blockingWait(subscriber_->connect(relayUrl_));
            if (!connected) {
                std::cerr << "  [Fixture] Failed to connect subscriber" << std::endl;
                subscriber_.reset();
                throw std::runtime_error("Failed to establish subscriber connection");
            }
            std::cout << "  [Fixture] Subscriber connected successfully" << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "  [Fixture] Exception connecting subscriber: " << ex.what() << std::endl;
            subscriber_.reset();
            throw;
        }
    }

    return subscriber_;
}

std::shared_ptr<moq_interface::MoQInterface> TestFixture::createMoQInterface(bool autoConnect) {
    std::cout << "  [Fixture] Creating new MoQ interface..." << std::endl;
    
    auto interface = std::make_shared<moq_interface::MoQInterface>(eventBase_);
    
    if (autoConnect) {
        try {
            bool connected = folly::coro::blockingWait(interface->connect(relayUrl_));
            if (!connected) {
                std::cerr << "  [Fixture] Failed to connect new interface" << std::endl;
                throw std::runtime_error("Failed to establish connection");
            }
            std::cout << "  [Fixture] New interface connected successfully" << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "  [Fixture] Exception connecting interface: " << ex.what() << std::endl;
            throw;
        }
    }
    
    // Track additional interfaces for cleanup
    additionalInterfaces_.push_back(interface);
    
    return interface;
}

std::shared_ptr<TestSubscriptionHandle> TestFixture::createSubscriptionHandle() {
    return std::make_shared<TestSubscriptionHandle>();
}

void TestFixture::resetPublisher() {
    if (publisher_) {
        std::cout << "  [Fixture] Resetting publisher..." << std::endl;
        cleanupInterface(publisher_, "publisher");
    }
}

void TestFixture::resetSubscriber() {
    if (subscriber_) {
        std::cout << "  [Fixture] Resetting subscriber..." << std::endl;
        cleanupInterface(subscriber_, "subscriber");
    }
}

void TestFixture::cleanupInterface(std::shared_ptr<moq_interface::MoQInterface>& interface,
                                   const std::string& name) {
    if (!interface) {
        return;
    }

    try {
        auto client = interface->getClient();
        if (eventBase_ && client && client->moqSession_) {
            std::cout << "  [Fixture] Closing " << name << " session..." << std::endl;
            
            eventBase_->runImmediatelyOrRunInEventBaseThreadAndWait([client, &name]() {
                try {
                    client->moqSession_->close(moxygen::SessionCloseErrorCode::NO_ERROR);
                    std::cout << "  [Fixture] " << name << " session closed" << std::endl;
                } catch (const std::exception& ex) {
                    std::cerr << "  [Fixture] Exception closing " << name 
                             << " session: " << ex.what() << std::endl;
                }
            });

            // Give time for graceful shutdown
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& ex) {
        std::cerr << "  [Fixture] Exception during " << name 
                 << " cleanup: " << ex.what() << std::endl;
    }

    interface.reset();
}

} // namespace interop_test
