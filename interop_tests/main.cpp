#include "TestSuite.h"
#include <iostream>
#include <folly/init/Init.h>
#include <folly/io/async/EventBaseThread.h>
#include <gflags/gflags.h>

DEFINE_string(relay, "https://localhost:4433/moq", "MoQ relay server URL");

using namespace interop_test;

int main(int argc, char* argv[]) {
    folly::Init init(&argc, &argv);

    // Create EventBaseThread
    auto eventBaseThread = std::make_unique<folly::EventBaseThread>();

    // Create test suite
    auto testSuite = std::make_unique<TestSuite>(eventBaseThread->getEventBase());

    // Configure test suite
    TestSuiteConfig config;
    config.relayUrl = FLAGS_relay;

    // Run all tests
    bool success = testSuite->runAll(config);

    return success ? 0 : 1;
}