#pragma once

#include "base/base_test.h"
#include "moxygen_adapter/moxygen_fixture.h"
#include <memory>

namespace interop_test {

/**
 * Base class for tests using the Moxygen fixture
 * 
 * This convenience class automatically creates and sets up a MoxygenTestFixture
 * for tests that use the moxygen implementation.
 */
class MoxygenTest : public BaseTest {
public:
  explicit MoxygenTest(const TestContext &context) : BaseTest(context) {
    // Automatically create and set the moxygen fixture
    setFixture(std::make_shared<MoxygenTestFixture>(context));
  }
  
  virtual ~MoxygenTest() = default;
};

} // namespace interop_test
