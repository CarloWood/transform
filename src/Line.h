#pragma once

#include "CS.h"
#include "utils/to_string.h"
#include "cairowindow/Line.h"

template<CS cs>
class Line : public cairowindow::Line
{
 public:
  using cairowindow::Line::Line;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const
  {
    os << utils::to_string(cs) << ":";
    cairowindow::Line::print_on(os);
  }
#endif
};
