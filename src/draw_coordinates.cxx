#include "sys.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/draw/Shape.h"
#include "cairowindow/draw/Line.h"
#include "math/Vector.h"
#include "utils/AIAlert.h"
#include "utils/has_print_on.h"
#include "utils/to_string.h"
#include "utils/debug_ostream_operators.h"
#include <QTransform>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include "debug.h"

constexpr int window_width = 600;
constexpr int window_height = 450;

constexpr int object_width = 200;
constexpr int object_height = 100;

using utils::has_print_on::operator<<;

enum CoordinateSystem
{
  draw,                                 // For coordinates used during a Qt draw call.
  centered,                             // Coordinate system with origin in the middle of the window, vertically running from -1 to 1.
  pixels                                // The final coordinate system of the window in pixels.
};

template<CoordinateSystem CS>
class Point;

template<CoordinateSystem CS>
class Size;

template<CoordinateSystem CS>
class TranslationVector;

template<CoordinateSystem from_CS, CoordinateSystem to_CS, bool inverted = false>
class Transform
{
  static constexpr CoordinateSystem convert_to = inverted ? from_CS : to_CS;
  static constexpr CoordinateSystem convert_from = inverted ? to_CS : from_CS;

 private:
  QTransform m_;

 private:
  Transform(QTransform const& m) : m_(m) { }

 public:
  Transform() = default;

  Transform& translate(TranslationVector<convert_to> const& tv);
  Transform& scale(qreal s);

  friend Point<convert_to> operator*(Point<convert_from> const& point, Transform const& transform);
  friend Size<convert_to> operator*(Size<convert_from> const& size, Transform const& transform);

  Transform<from_CS, to_CS, !inverted> const& inverse() const
  {
    return reinterpret_cast<Transform<from_CS, to_CS, !inverted> const&>(*this);
  }

  void print_on(std::ostream& os) const
  {
    std::ostringstream prefix;
    prefix << utils::to_string(from_CS) << "_transform_" << utils::to_string(to_CS) << ":";
    int const prefix_len = std::max((int)prefix.str().length(), 24);

    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎛" << std::setw(9) << m_.m11() << " " << std::setw(9) << m_.m12() << " " << m_.m13() << "⎞" << std::right;
    os << '\n' << std::right << std::setw(prefix_len) << prefix.str() <<
      std::left << "⎜" << std::setw(9) << m_.m21() << " " << std::setw(9) << m_.m22() << " " << m_.m23() << "⎟" << std::right;
    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎝" << std::setw(9) << m_.m31() << " " << std::setw(9) << m_.m32() << " " << m_.m33() << "⎠" << std::right;
  }
};

template<CoordinateSystem CS>
class Point
{
 private:
  qreal x_;
  qreal y_;

 public:
  Point() : x_(0), y_(0) { }
  Point(qreal x, qreal y) : x_(x), y_(y) { }

  qreal x() const { return x_; }
  qreal y() const { return y_; }

  void print_on(std::ostream& os) const
  {
    os << utils::to_string(CS) << ":(" << x_ << ", " << y_ << ")";
  }
};

template<CoordinateSystem CS>
class Size
{
 private:
  qreal width_;
  qreal height_;

 public:
  Size(qreal width, qreal height) : width_(width), height_(height) { }

  qreal width() const { return width_; }
  qreal height() const { return height_; }

  void print_on(std::ostream& os) const
  {
    os << utils::to_string(CS) << ":(" << width_ << ", " << height_ << ")";
  }
};

template<CoordinateSystem CS>
class TranslationVector
{
 private:
  math::Vector translation_;

 private:
  TranslationVector(math::Vector const& translation) : translation_(translation) { }

 public:
  TranslationVector(Size<CS> const& size) : translation_(size.width(), size.height()) { }
  TranslationVector(Point<CS> const& point) : translation_(point.x(), point.y()) { }

  double x() const { return translation_.x(); }
  double y() const { return translation_.y(); }

  friend TranslationVector operator*(double scale, TranslationVector const& tv)
  {
    return {scale * tv.translation_};
  }
};

template<CoordinateSystem from_CS, CoordinateSystem to_CS, bool inverted>
Transform<from_CS, to_CS, inverted>& Transform<from_CS, to_CS, inverted>::translate(TranslationVector<convert_to> const& tv)
{
  m_.translate(tv.x(), tv.y());
  return *this;
}

template<CoordinateSystem from_CS, CoordinateSystem to_CS, bool inverted>
Transform<from_CS, to_CS, inverted>& Transform<from_CS, to_CS, inverted>::scale(qreal s)
{
  m_.scale(s, s);
  return *this;
}

template<CoordinateSystem from_CS, CoordinateSystem to_CS, bool inverted>
Point<Transform<from_CS, to_CS, inverted>::convert_to> operator*(
    Point<Transform<from_CS, to_CS, inverted>::convert_from> const& point, Transform<from_CS, to_CS, inverted> const& transform)
{
  QPointF p{point.x(), point.y()};
  QPointF result;
  if constexpr (!inverted)
    result = transform.m_.map(p);
  else
    result = transform.m_.inverted().map(p);
  return {result.x(), result.y()};
}

template<CoordinateSystem from_CS, CoordinateSystem to_CS, bool inverted>
Size<Transform<from_CS, to_CS, inverted>::convert_to> operator*(
    Size<Transform<from_CS, to_CS, inverted>::convert_from> const& size, Transform<from_CS, to_CS, inverted> const& transform)
{
  // Just scale.
  if constexpr (!inverted)
    return {size.width() * transform.m_.m11(), size.height() * transform.m_.m22()};
  else
    return {size.width() / transform.m_.m11(), size.height() / transform.m_.m22()};
}

Size<pixels> half_window_size(0.5 * window_width, 0.5 * window_height);
Transform<centered, pixels> const centered_transform_pixels = Transform<centered, pixels>{}.translate(half_window_size).scale(half_window_size.height());

struct RectangleToWindow
{
  Point<pixels> topleft_centered_;
  Size<pixels> size_centered_;

  RectangleToWindow(Point<centered> const& topleft_centered, Size<centered> const& size_centered) :
    topleft_centered_(topleft_centered * centered_transform_pixels),
    size_centered_(size_centered * centered_transform_pixels)
  {
    DoutEntering(dc::notice, "RectangleToWindow(" << topleft_centered << ", " << size_centered << ")");
    Dout(dc::notice, "topleft_centered_ = " << topleft_centered_);
    Dout(dc::notice, "size_centered_ = " << size_centered_);

    Dout(dc::notice, "Where " << topleft_centered << " = " << topleft_centered << " * " << centered_transform_pixels);
    Dout(dc::notice, "The translation should have been " << half_window_size);
  }
};

class Rectangle : public RectangleToWindow, public cairowindow::Rectangle
{
 public:
  Rectangle(Point<centered> const& topleft_centered, Size<centered> const& size_centered) :
    RectangleToWindow(topleft_centered, size_centered),
    cairowindow::Rectangle(topleft_centered_.x(), topleft_centered_.y(), size_centered_.width(), size_centered_.height())
  {
  }
};

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  Dout(dc::notice, "Entering main()");

  try
  {
    using Window = cairowindow::Window;
    using Layer = cairowindow::Layer;
    using EventLoop = cairowindow::EventLoop;
    using Shape = cairowindow::draw::Shape;
    using Line = cairowindow::draw::Line;
    using LineStyle = cairowindow::draw::LineStyle;
    using ShapeStyleParams = cairowindow::draw::ShapeStyleParams;
    namespace color = cairowindow::color;
    namespace cwdraw = cairowindow::draw;

    // Create a window.
    Window win("My window", window_width, window_height);

    // Create a new layer with a white background.
    auto layer = win.create_background_layer<Layer>(color::white COMMA_DEBUG_ONLY("background_layer"));

    // Open the window and start drawing.
    std::thread event_loop([&](){
      Debug(NAMESPACE_DEBUG::init_thread("event_loop"));
      // Open window, handle event loop. This must be constructed after the draw stuff, so that it is destructed first!
      // Upon destruction it blocks until the event loop thread finished (aka, the window was closed).
      EventLoop event_loop = win.run();
      event_loop.set_cleanly_terminated();
    });

    //=========================================================================
    // Start of actual program.

    Size<pixels> const ObjectSize_pixels{object_width, object_height};
    Size<centered> const ObjectSize_centered = ObjectSize_pixels * centered_transform_pixels.inverse();
    Dout(dc::notice, "ObjectSize_centered = " << ObjectSize_centered);

    Point<draw> DrawOrigin_draw;
    auto draw_transform_centered = Transform<draw, centered>{}.translate(-0.5 * TranslationVector{ObjectSize_centered});
    Dout(dc::notice, "draw_transform_centered = " << draw_transform_centered);

    Point<centered> DrawOrigin_centered = DrawOrigin_draw * draw_transform_centered;
    Dout(dc::notice, "DrawOrigin_centered = " << DrawOrigin_centered);

    // Display the draw-coordinate-system.
//    display_coordinate_system(layer, draw_transform_pixels);

    // Draw a line.
//    auto blue_line = std::make_shared<Line>(350, 250, 100, 100, LineStyle({.line_color = color::blue, .line_width = 1.0}));
//    layer->draw(blue_line);

    // Display the object of ObjectSize_centered (centered-coordinate-system) with the top-left in the origin of the draw-coordinate-system (DrawOrigin).
    auto object1 = std::make_shared<Shape>(Rectangle{DrawOrigin_centered, ObjectSize_centered}, ShapeStyleParams{.line_color = color::black, .shape = cwdraw::rectangle});
    layer->draw(object1);

    // End
    //=========================================================================

    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }

  Dout(dc::notice, "Leaving main()");
}
