#pragma once

#include "base/moqt_types.h"
#include <moxygen/MoQFramer.h>

namespace interop_test {

/**
 * Utility functions to convert between interop_test types and moxygen types
 */

inline moxygen::GroupOrder toMoxygenGroupOrder(GroupOrder order) {
  switch (order) {
    case GroupOrder::Default:
      return moxygen::GroupOrder::Default;
    case GroupOrder::OldestFirst:
      return moxygen::GroupOrder::OldestFirst;
    case GroupOrder::NewestFirst:
      return moxygen::GroupOrder::NewestFirst;
    default:
      return moxygen::GroupOrder::Default;
  }
}

inline GroupOrder fromMoxygenGroupOrder(moxygen::GroupOrder order) {
  switch (order) {
    case moxygen::GroupOrder::Default:
      return GroupOrder::Default;
    case moxygen::GroupOrder::OldestFirst:
      return GroupOrder::OldestFirst;
    case moxygen::GroupOrder::NewestFirst:
      return GroupOrder::NewestFirst;
    default:
      return GroupOrder::Default;
  }
}

inline moxygen::AbsoluteLocation toMoxygenAbsoluteLocation(const AbsoluteLocation& loc) {
  return moxygen::AbsoluteLocation{loc.group, loc.object};
}

inline AbsoluteLocation fromMoxygenAbsoluteLocation(const moxygen::AbsoluteLocation& loc) {
  return AbsoluteLocation{loc.group, loc.object};
}

} // namespace interop_test
