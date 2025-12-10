#include "moq_interface.h"
#include <iostream>

namespace moq_interface {

MoQInterface::MoQInterface(folly::EventBase* eventBase)
    : eventBase_(eventBase) {
    if (!eventBase_) {
        throw std::invalid_argument("EventBase cannot be null");
    }
}

folly::coro::Task<bool> MoQInterface::connect(
    const std::string& url,
    std::chrono::milliseconds connectTimeout,
    std::chrono::milliseconds transactionTimeout,
    bool useInsecureVerifier) {

    try {
        // Create executor from the provided event base
        auto executor = std::make_shared<moxygen::MoQFollyExecutorImpl>(eventBase_);

        proxygen::URL parsedUrl(url);
        if (!parsedUrl.isValid()) {
            std::cerr << "Invalid URL: " << url << std::endl;
            co_return false;
        }

        std::shared_ptr<fizz::CertificateVerifier> verifier = nullptr;
        if (useInsecureVerifier) {
            verifier = std::make_shared<fizz::InsecureAcceptAnyCertificate>();
        }

        // Create MoQ client
        client_ = std::make_shared<moxygen::MoQClient>(
            executor,
            std::move(parsedUrl),
            verifier);

        // Set up MoQ session
        co_await client_->setupMoQSession(
            connectTimeout,
            transactionTimeout,
            nullptr,
            nullptr,
            {}
        );
        
        std::cout << "MoQ session established successfully" << std::endl;
        co_return true;

    } catch (const std::exception& ex) {
        std::cerr << "Failed to establish MoQ session: " << ex.what() << std::endl;
        client_.reset();
        co_return false;
    }
}

folly::coro::Task<bool> MoQInterface::publish(
    const std::string& trackNamespace,
    const std::string& trackName,
    std::shared_ptr<moxygen::SubscriptionHandle> subscriptionHandle) {
    
    try {
        if (!isConnected()) {
            std::cerr << "No MoQ session available" << std::endl;
            co_return false;
        }

        auto session = client_->moqSession_;

        // Create publish request
        moxygen::PublishRequest publishReq;
        publishReq.fullTrackName = moxygen::FullTrackName{
            .trackNamespace = moxygen::TrackNamespace(std::vector<std::string>{trackNamespace}),
            .trackName = trackName
        };
        publishReq.requestID = moxygen::RequestID{1};
        publishReq.trackAlias = moxygen::TrackAlias{1};
        publishReq.groupOrder = moxygen::GroupOrder::Default;
        publishReq.forward = false;

        std::cout << "Sending publish request for track: "
                  << trackNamespace << "/" << trackName << std::endl;

        // Send publish request via session's publish method
        auto publishResult = session->publish(publishReq, subscriptionHandle);

        if (publishResult.hasValue()) {
            auto publishConsumerAndReply = std::move(publishResult.value());

            // Handle the PUBLISH_OK or PUBLISH_ERROR
            auto replyTask = std::move(publishConsumerAndReply.reply);
            auto replyResult = co_await std::move(replyTask);

            if (replyResult.hasValue()) {
                auto publishOk = replyResult.value();
                std::cout << "Publish OK received. Request ID: "
                          << publishOk.requestID.value << std::endl;
                co_return true;
            } else {
                auto error = replyResult.error();
                std::cerr << "Publish failed: " << error.reasonPhrase << std::endl;
                co_return false;
            }
        } else {
            auto error = publishResult.error();
            std::cerr << "Publish request failed: " << error.reasonPhrase << std::endl;
            co_return false;
        }
    } catch (const std::exception& ex) {
        std::cerr << "Exception in publish: " << ex.what() << std::endl;
        co_return false;
    }
}

} // namespace moq_interface