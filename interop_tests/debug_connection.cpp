#include <iostream>
#include <folly/init/Init.h>
#include <folly/logging/Init.h>
#include <folly/io/async/EventBaseThread.h>
#include <folly/coro/BlockingWait.h>
#include <chrono>
#include <memory>
#include <thread>
#include "moxygen_adapter/moxygen_interface.h"

std::unique_ptr<folly::EventBaseThread> globalEventBaseThread;
// TODO: DELETE THIS
// Just testing things out
folly::coro::Task<void> testConnection() {
    std::shared_ptr<interop_test::MoxygenInterface> moqInterface = nullptr;
    try {
        std::cout << "Creating event base..." << std::endl;

        globalEventBaseThread = std::make_unique<folly::EventBaseThread>();

        std::cout << "Setting up MoQ session..." << std::endl;

        moqInterface = std::make_shared<interop_test::MoxygenInterface>(
            globalEventBaseThread->getEventBase());
        
        bool connected = co_await moqInterface->connect("https://localhost:4433/moq");
        
        if (!connected) {
            std::cout << "Connection failed!" << std::endl;
            co_return;
        }

        std::cout << "Connection successful!" << std::endl;

        std::cout << "Keeping connection alive for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        std::cout << "Starting graceful shutdown..." << std::endl;

    } catch (const std::exception& ex) {
        std::cout << "Connection failed: " << ex.what() << std::endl;
    }

    if (moqInterface) {
        std::cout << "Cleaning up MoQ interface..." << std::endl;
        moqInterface.reset();
        std::cout << "MoQ interface cleaned up" << std::endl;

        // Give time for async cleanup to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Cleaning up event base..." << std::endl;
    globalEventBaseThread.reset();
    std::cout << "Event base cleaned up" << std::endl;
}

int main(int argc, char* argv[]) {
    folly::Init init(&argc, &argv);

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