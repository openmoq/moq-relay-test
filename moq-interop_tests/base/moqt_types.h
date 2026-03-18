#pragma once

#include <cstdint>
#include <string>

namespace interop_test {

/**
 * Group ordering preference for MoQ Transport subscriptions
 * Specifies how groups should be delivered when multiple are available
 */
enum class GroupOrder : uint8_t {
  Default = 0x0,      // Use default ordering
  OldestFirst = 0x1,  // Deliver oldest groups first
  NewestFirst = 0x2   // Deliver newest groups first
};

/**
 * Absolute location within a MoQ track
 * Identifies a specific object by group and object ID
 */
struct AbsoluteLocation {
  uint64_t group{0};
  uint64_t object{0};

  AbsoluteLocation() = default;
  constexpr AbsoluteLocation(uint64_t g, uint64_t o) : group(g), object(o) {}

  bool operator==(const AbsoluteLocation& other) const {
    return group == other.group && object == other.object;
  }

  bool operator!=(const AbsoluteLocation& other) const {
    return !(*this == other);
  }

  bool operator<(const AbsoluteLocation& other) const {
    if (group < other.group) {
      return true;
    } else if (group == other.group) {
      return object < other.object;
    }
    return false;
  }

  bool operator<=(const AbsoluteLocation& other) const {
    return *this < other || *this == other;
  }

  bool operator>(const AbsoluteLocation& other) const {
    return !(*this <= other);
  }

  bool operator>=(const AbsoluteLocation& other) const {
    return !(*this < other);
  }

  std::string describe() const {
    return "{" + std::to_string(group) + "," + std::to_string(object) + "}";
  }
};

} // namespace interop_test
