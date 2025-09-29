#pragma once

#include "CS.h"
#include "utils/has_print_on.h"
#include "utils/to_string.h"
#include <QPointF>      // For qreal

constexpr int window_width = 600;
constexpr int window_height = 450;

constexpr int object_width = 200;
constexpr int object_height = 100;

using utils::has_print_on::operator<<;

template<CS cs>
class Size
{
 private:
  qreal width_;
  qreal height_;

 public:
  constexpr Size(qreal width, qreal height) : width_(width), height_(height) { }

  qreal width() const { return width_; }
  qreal height() const { return height_; }

  void print_on(std::ostream& os) const
  {
    os << utils::to_string(cs) << ":(" << width_ << ", " << height_ << ")";
  }
};

static constexpr Size<CS::pixels> window_size(window_width, window_height);
static constexpr Size<CS::pixels> half_window_size(0.5 * window_width, 0.5 * window_height);
