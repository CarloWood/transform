#pragma once

#include "CS.h"
#include "Size.h"
#include "cairowindow/Point.h"

template<CS cs>
class Point : public cairowindow::Point
{
 public:
  // Construct the origin.
  Point() : cairowindow::Point(0.0, 0.0) { }
  Point(double x, double y) : cairowindow::Point(x, y) { }

  // Allow returning a cairowindow::Point by value from a function that returns a Point<cs>.
  // This constructor should never be called manually (i.e. using std::move).
  Point(cairowindow::Point&& base) : cairowindow::Point(base) { }

  Point operator+(Size<cs> const& size) const
  {
    return {x_ + size.width(), y_ + size.height()};
  }
};
