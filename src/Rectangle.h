#pragma once

#if 0 // old code
#include "Point.h"
#include "Size.h"
#include "Transform.h"
#include "cairowindow/Rectangle.h"
#include "debug.h"

struct RectangleToWindow
{
  static Transform<CS::centered, CS::pixels> const centered_transform_pixels;

  Point<CS::pixels> topleft_pixels_;
  Size<CS::pixels> size_pixels_;

  RectangleToWindow(Point<CS::centered> const& topleft_centered, Size<CS::centered> const& size_centered) :
    topleft_pixels_(topleft_centered * centered_transform_pixels),
    size_pixels_(size_centered * centered_transform_pixels)
  {
    DoutEntering(dc::notice, "RectangleToWindow(" << topleft_centered << ", " << size_centered << ")");
    Dout(dc::notice, "topleft_pixels_ = " << topleft_pixels_);
    Dout(dc::notice, "size_pixels_ = " << size_pixels_);

    Dout(dc::notice, "Where " << topleft_pixels_ << " = " << topleft_centered << " * " << centered_transform_pixels);
    Dout(dc::notice, "The translation should have been " << half_window_size);
  }
};

class Rectangle : public RectangleToWindow, public cairowindow::Rectangle
{
 public:
  Rectangle(Point<CS::centered> const& topleft_centered, Size<CS::centered> const& size_centered) :
    RectangleToWindow(topleft_centered, size_centered),
    cairowindow::Rectangle(topleft_pixels_.x(), topleft_pixels_.y(), size_pixels_.width(), size_pixels_.height())
  {
  }
};
#endif

#include "CS.h"
#include "cairowindow/Rectangle.h"

template<CS cs>
class Rectangle : public cairowindow::Rectangle
{
 public:
  using cairowindow::Rectangle::Rectangle;
};
