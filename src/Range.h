#pragma once

#include "cairowindow/cs/CS.h"
#include "cairowindow/Range.h"
#include "utils/to_string.h"
#ifdef CWDEBUG
#include "utils/has_print_on.h"
#endif

namespace cairowindow::cs {
#ifdef CWDEBUG
using utils::has_print_on::operator<<;
#endif

template<CS cs>
class Range : public cairowindow::Range
{
 public:
  using cairowindow::Range::Range;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const
  {
    os << utils::to_string(cs) << ":";
    cairowindow::Range::print_on(os);
  }
#endif
};

} // namespace cairowindow::cs
