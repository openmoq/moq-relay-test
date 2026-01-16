#pragma once

#include <string>
#include <memory>
#include "base/BaseTest.h"
#include "TestCommon.h"

namespace interop_test {

/**
 * Basic publish test - verifies that a client can successfully publish a track to the relay
 */
class PublishTest : public BaseTest {
public:
    explicit PublishTest(const TestContext& context);
    ~PublishTest() override = default;

    // BaseTest interface
    std::string getName() const override { return "PublishTest"; }
    std::string getDescription() const override {
        return "Verifies that a client can successfully publish a track to the relay";
    }
    TestCategory getCategory() const override { return TestCategory::ALL; }

protected:
    TestResult execute() override;

private:
    std::string trackNamespace_{"test"};
    std::string trackName_{"interop-track"};
};

} // namespace interop_test