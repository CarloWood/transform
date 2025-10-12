#pragma once

#include "Point.h"
#include "Size.h"
#include "math/Vector.h"

template<CS cs>
class TranslationVector
{
 private:
  math::Vector<2> translation_;

 private:
  TranslationVector(math::Vector<2> const& translation) : translation_(translation) { }

 public:
  TranslationVector(Point<cs> const& point) : translation_(point.x(), point.y()) { }
  TranslationVector(Size<cs> const& size) : translation_(size.width(), size.height()) { }

  double x() const { return translation_.x(); }
  double y() const { return translation_.y(); }

  friend TranslationVector operator*(double scale, TranslationVector const& tv)
  {
    return {scale * tv.translation_};
  }
};
