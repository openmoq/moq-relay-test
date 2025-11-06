#include "moq_utils.h"
#include <iostream>

namespace moq_utils {

folly::coro::Task<std::shared_ptr<moxygen::MoQClient>> createMoQSession(
    folly::EventBase* eventBase,
    const MoQSessionConfig& config) {

    try {
        // Create executor from the provided event base
        auto executor = std::make_shared<moxygen::MoQFollyExecutorImpl>(eventBase);

        // Parse the URL
        proxygen::URL url(config.url);
        if (!url.isValid()) {
            throw std::invalid_argument("Invalid URL: " + config.url);
        }

        // Create certificate verifier
        std::shared_ptr<fizz::CertificateVerifier> verifier = nullptr;
        if (config.useInsecureVerifier) {
            verifier = std::make_shared<fizz::InsecureAcceptAnyCertificate>();
        }

        // Create MoQ client
        auto client = std::make_shared<moxygen::MoQClient>(
            executor,
            std::move(url),
            verifier);

        // Determine handlers to use
        auto publishHandler = config.publishHandler;
        auto subscribeHandler = config.subscribeHandler;

        // If no handlers provided, use default stub handlers
        if (!publishHandler) {
            publishHandler = std::make_shared<StubPublisher>();
        }
        if (!subscribeHandler) {
            subscribeHandler = std::make_shared<StubSubscriber>();
        }

        // Set up MoQ session
        co_await client->setupMoQSession(
            config.connectTimeout,
            config.transactionTimeout,
            nullptr,
            nullptr,
            // publishHandler,
            // subscribeHandler,
            {}
        );

        co_return client;

    } catch (const std::exception& ex) {
        std::cerr << "Failed to create MoQ session: " << ex.what() << std::endl;
        throw;
    }
}

folly::coro::Task<std::shared_ptr<moxygen::MoQClient>> createMoQSessionWithStubHandlers(
    folly::EventBase* eventBase,
    const std::string& url) {

    MoQSessionConfig config;
    config.url = url;
    config.publishHandler = std::make_shared<StubPublisher>();
    config.subscribeHandler = std::make_shared<StubSubscriber>();

    co_return co_await createMoQSession(eventBase, config);
}

} // namespace moq_utils