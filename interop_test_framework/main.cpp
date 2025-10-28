#include "PublishTest.h"
#include <iostream>
#include <memory>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <folly/logging/Init.h>
#include <folly/logging/LoggerDB.h>


using namespace interop_test;


int main (int argc, char* argv[]) {
    // Initialize Folly and logging
    folly::Init init(&argc, &argv);

    // Simple logging configuration that should work
    // This will enable DBG1 level logging and output to stderr by default
    folly::initLogging(".=DBG1");

    // Also set specific categories to be more verbose
    auto& db = folly::LoggerDB::get();
    db.getCategory("")->setLevel(folly::LogLevel::DBG1, true);

    std::cout << "Logging initialized - XLOG messages should be visible" << std::endl;    // Create PublishTest as a shared pointer since it uses shared_from_this()
    auto test = std::make_shared<PublishTest>();    PublishTestConfig config;
    config.trackNamespace = "test";
    config.trackName = "interop-track";
    config.serverUrl = "https://localhost:4433/moq";

    std::cout << "Running Interop Publish Test...\n";

    auto result = test->runTest(config);

    switch (result) {
        case TestResult::PASS:
            std::cout << "Interop Publish Test PASSED\n";
            break;
        case TestResult::FAIL:
            std::cout << "Interop Publish Test FAILED: " << test->getLastError() << "\n";
            break;
        case TestResult::TIMEOUT:
            std::cout << "Interop Publish Test TIMED OUT\n";
            break;
        case TestResult::ERROR:
            std::cout << "Interop Publish Test ERROR: " << test->getLastError() << "\n";
            break;
    }

    return 0;
}