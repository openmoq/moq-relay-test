#pragma once

#include "base/BaseTest.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace interop_test {

/**
 * Factory function type for creating test instances
 */
using TestFactory = std::function<std::unique_ptr<BaseTest>(const TestContext&)>;

/**
 * Metadata about a registered test
 */
struct TestInfo {
    std::string name;
    std::string description;
    TestCategory category;
    TestFactory factory;
};

/**
 * Global registry for test cases
 * 
 * Tests register themselves using the REGISTER_TEST macro.
 * The TestSuite queries the registry to discover and run tests.
 */
class TestRegistry {
public:
    // Get the singleton instance
    static TestRegistry& instance();

    // Register a test
    void registerTest(const std::string& name, const TestInfo& info);

    // Get all registered tests
    const std::map<std::string, TestInfo>& getAllTests() const;

    // Get tests by category
    std::vector<TestInfo> getTestsByCategory(TestCategory category) const;

    // Create a test instance by name
    std::unique_ptr<BaseTest> createTest(const std::string& name, const TestContext& context) const;

private:
    TestRegistry() = default;
    std::map<std::string, TestInfo> tests_;
};

/**
 * Helper class for automatic test registration
 * 
 * Usage: static TestRegistrar<MyTest> myTestRegistrar("MyTest");
 */
template<typename T>
class TestRegistrar {
public:
    explicit TestRegistrar(const std::string& name) {
        // Create a dummy context to get metadata
        TestContext dummyContext;
        auto testInstance = std::make_unique<T>(dummyContext);
        
        TestInfo info;
        info.name = testInstance->getName();
        info.description = testInstance->getDescription();
        info.category = testInstance->getCategory();
        info.factory = [](const TestContext& ctx) -> std::unique_ptr<BaseTest> {
            return std::make_unique<T>(ctx);
        };
        
        TestRegistry::instance().registerTest(name, info);
    }
};

/**
 * Macro for registering tests
 * 
 * REGISTER_TEST(MyTestClass)
 */
#define REGISTER_TEST(TestClass) \
    static ::interop_test::TestRegistrar<TestClass> g_##TestClass##_registrar(#TestClass)

} // namespace interop_test
