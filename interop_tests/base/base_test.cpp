#include "base_test.h"
#include "test_fixture.h"
#include <iostream>

namespace interop_test {

BaseTest::BaseTest(const TestContext& context)
    : context_(context)
    , fixture_(std::make_shared<TestFixture>(context)) {
}

TestResult BaseTest::run() {
    logAlways("Running test: " + getName());
    log("Description: " + getDescription());
    log("Category: " + testCategoryToString(getCategory()));

    try {
        // Setup phase
        log("Setting up test...");
        setUp();
        fixture_->setUp();

        // Execute phase
        log("Executing test...");
        TestResult result = execute();

        // Teardown phase (always run, even on failure)
        log("Tearing down test...");
        tearDown();
        fixture_->tearDown();

        if (result == TestResult::PASS) {
            logAlways("✓ Test PASSED");
        } else {
            logAlways("✗ Test FAILED: " + lastError_);
        }

        return result;

    } catch (const std::exception& ex) {
        // Ensure teardown runs even if exception thrown
        try {
            tearDown();
            fixture_->tearDown();
        } catch (const std::exception& teardownEx) {
            std::cerr << "Exception during teardown: " << teardownEx.what() << std::endl;
        }

        setError(std::string("Exception: ") + ex.what());
        logAlways("✗ Test ERROR: " + lastError_);
        return TestResult::ERROR;
    }
}

void BaseTest::setError(const std::string& error) {
    lastError_ = error;
}

void BaseTest::log(const std::string& message) const {
    if (context_.verbose) {
        std::cout << "  [" << getName() << "] " << message << std::endl;
    }
}

void BaseTest::logAlways(const std::string& message) const {
    std::cout << "[" << getName() << "] " << message << std::endl;
}

void BaseTest::assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        setError("Assertion failed: " + message);
        throw std::runtime_error(lastError_);
    }
}

void BaseTest::assertFalse(bool condition, const std::string& message) {
    if (condition) {
        setError("Assertion failed (expected false): " + message);
        throw std::runtime_error(lastError_);
    }
}

void BaseTest::assertNotNull(const void* ptr, const std::string& message) {
    if (ptr == nullptr) {
        setError("Assertion failed (null pointer): " + message);
        throw std::runtime_error(lastError_);
    }
}

} // namespace interop_test
