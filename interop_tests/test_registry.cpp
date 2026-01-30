#include "test_registry.h"
#include <stdexcept>

namespace interop_test {

TestRegistry &TestRegistry::instance() {
  static TestRegistry registry;
  return registry;
}

void TestRegistry::registerTest(const std::string &name, const TestInfo &info) {
  tests_[name] = info;
}

const std::map<std::string, TestInfo> &TestRegistry::getAllTests() const {
  return tests_;
}

std::vector<TestInfo>
TestRegistry::getTestsByCategory(TestCategory categoryFilter) const {
  std::vector<TestInfo> result;

  for (const auto &[name, info] : tests_) {
    // If categoryFilter is ALL, include all tests
    // Otherwise, check if test has any of the requested categories
    if (categoryFilter == TestCategory::ALL || 
        hasCategory(info.categories, categoryFilter)) {
      result.push_back(info);
    }
  }

  return result;
}

std::unique_ptr<BaseTest>
TestRegistry::createTest(const std::string &name,
                         const TestContext &context) const {

  auto it = tests_.find(name);
  if (it == tests_.end()) {
    throw std::runtime_error("Test not found: " + name);
  }

  return it->second.factory(context);
}

} // namespace interop_test
