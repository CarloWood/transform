#pragma once

#include "CS.h"
#include "cairowindow/Line.h"

template<CS cs>
class Line : public cairowindow::Line
{
 public:
  using cairowindow::Line::Line;
};
