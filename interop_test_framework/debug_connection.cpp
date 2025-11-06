#include <iostream>
#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/io/async/EventBaseThread.h>
#include <folly/coro/BlockingWait.h>
#include <chrono>
#include <memory>
#include <thread>
#include "moq_utils.h"

std::unique_ptr<folly::EventBaseThread> globalEventBaseThread;

folly::coro::Task<void> testConnection() {
    std::shared_ptr<moxygen::MoQClient> client = nullptr;
    try {
        std::cout << "Creating event base..." << std::endl;

        // Create a dedicated event base thread for proper cleanup
        globalEventBaseThread = std::make_unique<folly::EventBaseThread>();

        std::cout << "Setting up MoQ session..." << std::endl;

        // Use the utility function to create the MoQ session
        client = co_await moq_utils::createMoQSessionWithStubHandlers(
            globalEventBaseThread->getEventBase(),
            "https://localhost:4433/moq");

        std::cout << "Connection successful!" << std::endl;

        // Let the connection stay alive a bit to test the cleanup properly
        std::cout << "Keeping connection alive for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        std::cout << "Starting graceful shutdown..." << std::endl;

    } catch (const std::exception& ex) {
        std::cout << "Connection failed: " << ex.what() << std::endl;
    }

    // Explicit cleanup to avoid race conditions
    if (client) {
        std::cout << "Cleaning up client..." << std::endl;
        client.reset();
        std::cout << "Client cleaned up" << std::endl;

        // Give time for async cleanup to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Cleaning up event base..." << std::endl;
    globalEventBaseThread.reset();
    std::cout << "Event base cleaned up" << std::endl;
}

int main(int argc, char* argv[]) {
    folly::Init init(&argc, &argv);
    folly::initLogging(".=DBG3");

    std::cout << "Starting debug connection test..." << std::endl;

    try {
        folly::coro::blockingWait(testConnection());
        std::cout << "Test completed successfully" << std::endl;
    } catch (const std::exception& ex) {
        std::cout << "Test failed: " << ex.what() << std::endl;

        // Clean up global resources
        if (globalEventBaseThread) {
            globalEventBaseThread.reset();
        }
        return 1;
    }

    std::cout << "Main function exiting cleanly" << std::endl;
    return 0;
}