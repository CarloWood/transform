#pragma once

#include "CS.h"
#include "cairowindow/Range.h"
#include "utils/to_string.h"
#include "utils/has_print_on.h"

using utils::has_print_on::operator<<;

template<CS cs>
class Range : public cairowindow::Range
{
 public:
  using cairowindow::Range::Range;

  void print_on(std::ostream& os) const
  {
    os << utils::to_string(cs) << ":";
    cairowindow::Range::print_on(os);
  }
};
