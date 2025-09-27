#include "sys.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/draw/Shape.h"
#include "cairowindow/draw/Line.h"
#include "cairowindow/draw/Text.h"
#include "cairowindow/intersection_points.h"
#include "math/Vector.h"
#include "math/Line.h"
#include "utils/AIAlert.h"
#include "utils/has_print_on.h"
#include "utils/to_string.h"
#include "utils/debug_ostream_operators.h"
#include <QTransform>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iomanip>
#include "debug.h"

constexpr int window_width = 600;
constexpr int window_height = 450;

constexpr int object_width = 200;
constexpr int object_height = 100;

using utils::has_print_on::operator<<;

enum class CS
{
  painter,              // The space defined by the painter’s Current Transformation Matrix (CTM) at the instant you issue a painter->draw*() call.
  centered,             // Coordinate system with origin in the middle of the window, where -1 corresponds with the bottom of the window and 1 with the top.
  pixels                // The final coordinate system of the window in pixels.
};

template<CS cs>
class Point;

template<CS cs>
class Size;

template<CS cs>
class TranslationVector;

template<CS from_cs, CS to_cs, bool inverted = false>
class Transform
{
 private:
  template<CS from_cs2, CS to_cs2, bool inverted2>
  friend class Transform;

  QTransform m_;

 private:
  Transform(QTransform const& m) : m_(m) { }

 public:
  Transform() = default;

  Transform& translate(TranslationVector<to_cs> const& tv);
  Transform& scale(qreal s);

  // The inverse converts from `to_cs` to `from_cs`!
  Transform<to_cs, from_cs, !inverted> const& inverse() const
  {
    return reinterpret_cast<Transform<to_cs, from_cs, !inverted> const&>(*this);
  }

  Point<to_cs> multiply_from_the_right_with(Point<from_cs> const& point) const;
  Size<to_cs> multiply_from_the_right_with(Size<from_cs> const& size) const;

  // Let A_M1_B be non-inverted and convert from A to B.
  // Let B_M2_C be non-inverted and convert from B to C.
  //
  // Multiplication between two non-inverted Transforms.
  // 1. A_M12_C = A_M1_B * B_M2_C
  //
  // Let A_M1_B^-1 be an inverted matrix that converts from B to A, and therefore denote it as B_M1inv_A
  // Let B_M2_C^-1 be an inverted matrix that converts from C to B, and therefore denote it as C_M2inv_B
  //
  // Multiplication between two inverted Transforms.
  // 2. A_M34inv_C = A_M4inv_B * B_M3inv_C
  //
  // Note that C_M34_A = C_M3_B * B_M4_A
  //
  // Let A_M5inv_B = B_M5_A^-1 be an inverted matrix that converts from A to B.
  // Let B_M56_C = B_M5_A * A_M6_C.
  //
  // Multiplication between an inverted Transform and a non-inverted Transform.
  // 3. A_M6_C = A_M5inv_B * B_M56_C
  //
  // Let A_M78_B = A_M7_C * C_M8_B.
  //
  // Multiplication between a non-inverted Transform and an inverted Transform.
  // 4. A_M7_C = A_M78_B * B_M8inv_C
  //
  // Then using specializations, where from_cs = A, to_cs = B and result_cs = C we'd have:
  //
  // Specialization for 1.
  // std::enable_if_t<!inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, false> const& rhs) const;
  //
  // Specialization for 2.
  // std::enable_if_t<inverted, Transform<from_cs, result_cs, true>> operator*(Transform<to_cs, result_cs, true> const& rhs) const;
  //
  // Specialization for 3.
  // std::enable_if_t<inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, false> const& rhs) const;
  //
  // Specialization for 4.
  //std::enable_if_t<!inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, true> const& rhs) const;
  //
  template<CS result_cs, bool rhs_inverted>
  Transform<from_cs, result_cs, inverted && rhs_inverted> operator*(Transform<to_cs, result_cs, rhs_inverted> const& rhs) const
  {
    // 1. Multiplication between two non-inverted Transforms.
    if constexpr (!inverted && !rhs_inverted)
    {
      return {m_ * rhs.m_};
    }
    // 2. Multiplication between two inverted Transforms.
    else if constexpr (inverted && rhs_inverted)
    {
      // A^-1 * B^-1 = (B * A)^-1
      return {rhs.m_ * m_};
    }
    // 3. Multiplication between an inverted Transform and a non-inverted Transform.
    else if constexpr (inverted && !rhs_inverted)
    {
      return {m_.inverted() * rhs.m_};
    }
    // 4. Multiplication between a non-inverted Transform and an inverted Transform.
    else if constexpr (!inverted && rhs_inverted)
    {
      return {m_ * rhs.m_.inverted()};
    }
  }

  void print_on(std::ostream& os) const
  {
    std::ostringstream prefix;
    prefix << utils::to_string(from_cs) << "_transform_" << utils::to_string(to_cs) << ":";
    int const prefix_len = std::max((int)prefix.str().length(), 24);

    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎛" << std::setw(9) << m_.m11() << " " << std::setw(9) << m_.m12() << " " << m_.m13() << "⎞" << std::right;
    os << '\n' << std::right << std::setw(prefix_len) << prefix.str() <<
      std::left << "⎜" << std::setw(9) << m_.m21() << " " << std::setw(9) << m_.m22() << " " << m_.m23() << "⎟" << std::right;
    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎝" << std::setw(9) << m_.m31() << " " << std::setw(9) << m_.m32() << " " << m_.m33() << "⎠" << std::right;
  }
};

template<CS cs>
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
    os << utils::to_string(cs) << ":(" << x_ << ", " << y_ << ")";
  }
};

template<CS cs>
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
    os << utils::to_string(cs) << ":(" << width_ << ", " << height_ << ")";
  }
};

template<CS cs>
class TranslationVector
{
 private:
  math::Vector translation_;

 private:
  TranslationVector(math::Vector const& translation) : translation_(translation) { }

 public:
  TranslationVector(Size<cs> const& size) : translation_(size.width(), size.height()) { }
  TranslationVector(Point<cs> const& point) : translation_(point.x(), point.y()) { }

  double x() const { return translation_.x(); }
  double y() const { return translation_.y(); }

  friend TranslationVector operator*(double scale, TranslationVector const& tv)
  {
    return {scale * tv.translation_};
  }
};

template<CS from_cs, CS to_cs, bool inverted>
Transform<from_cs, to_cs, inverted>& Transform<from_cs, to_cs, inverted>::translate(TranslationVector<to_cs> const& tv)
{
  m_.translate(tv.x(), tv.y());
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted>
Transform<from_cs, to_cs, inverted>& Transform<from_cs, to_cs, inverted>::scale(qreal s)
{
  m_.scale(s, s);
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted>
Point<to_cs> Transform<from_cs, to_cs, inverted>::multiply_from_the_right_with(Point<from_cs> const& point) const
{
  QPointF p{point.x(), point.y()};
  QPointF result;
  if constexpr (!inverted)
    result = m_.map(p);
  else
    result = m_.inverted().map(p);
  return {result.x(), result.y()};
}

template<CS from_cs, CS to_cs, bool inverted>
Point<to_cs> operator*(Point<from_cs> const& point, Transform<from_cs, to_cs, inverted> const& transform)
{
  return transform.multiply_from_the_right_with(point);
}

template<CS from_cs, CS to_cs, bool inverted>
Size<to_cs> Transform<from_cs, to_cs, inverted>::multiply_from_the_right_with(Size<from_cs> const& size) const
{
  // Just scale.
  if constexpr (!inverted)
    return {size.width() * m_.m11(), size.height() * m_.m22()};
  else
    return {size.width() / m_.m11(), size.height() / m_.m22()};
}

template<CS from_cs, CS to_cs, bool inverted>
Size<to_cs> operator*(Size<from_cs> const& size, Transform<from_cs, to_cs, inverted> const& transform)
{
  return transform.multiply_from_the_right_with(size);
}

Size<CS::pixels> half_window_size(0.5 * window_width, 0.5 * window_height);
Transform<CS::centered, CS::pixels> const centered_transform_pixels = Transform<CS::centered, CS::pixels>{}.translate(half_window_size).scale(half_window_size.height());

struct RectangleToWindow
{
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

std::shared_ptr<cairowindow::draw::Line> display_line(
    boost::intrusive_ptr<cairowindow::Layer> const& layer,
    cairowindow::draw::LineStyle const& line_style,
    math::Line const& line)
{
  DoutEntering(dc::notice, "display_line(layer, line_style, " << line << ")");

  math::Direction const& direction = line.direction();
  math::Point const& point = line.point();

  double normal_x = -direction.y();
  double normal_y = direction.x();
  intersections::HyperPlane<double, 2> line_({normal_x, normal_y}, -point.x() * normal_x - point.y() * normal_y);
  intersections::HyperBlock<double, 2> rectangle_({0, 0}, {window_width, window_height});
  auto intersections = rectangle_.intersection_points(line_);

  // Is the line outside the plot area?
  if (intersections.empty())
    return {};

  double x1 = intersections[0][0];
  double y1 = intersections[0][1];
  double x2 = intersections[1][0];
  double y2 = intersections[1][1];

  Dout(dc::notice, "Calling draw(" << x1 << ", " << y1 << ", " << x2 << ", " << y2 << ", ...)");
  std::shared_ptr<cairowindow::draw::Line> result = std::make_shared<cairowindow::draw::Line>(x1, y1, x2, y2, line_style);
  layer->draw(result);
  return result;
}

template<CS cs>
class CoordinateSystem
{
 private:
  std::vector<std::shared_ptr<cairowindow::draw::Line>> lines_;         // draw::Line objects that are part of the CoordinateSystem drawing.
  std::vector<std::shared_ptr<cairowindow::draw::Text>> texts_;         // draw::Text objects that are part of the CoordinateSystem drawing.
  Transform<cs, CS::pixels> reference_transform_;                       // The Transform defining this CoordinateSystem.

 public:
  // Construct a CoordinateSystem from a Transform. Call `display` to draw it.
  CoordinateSystem(Transform<cs, CS::pixels> const& reference_transform) : reference_transform_(reference_transform)
  {
    DoutEntering(dc::notice, "CoordinateSystem<" << utils::to_string(cs) << ">::CoordinateSystem(" << reference_transform << ") [" << this << "]");
  }

  void display(boost::intrusive_ptr<cairowindow::Layer> const& layer);
};

template<CS cs>
void CoordinateSystem<cs>::display(boost::intrusive_ptr<cairowindow::Layer> const& layer)
{
  DoutEntering(dc::notice, "CoordinateSystem<" << utils::to_string(cs) << ">::display(layer) [" << this << "]");

  // Call display only once.
  ASSERT(lines_.empty());

  using Line = cairowindow::draw::Line;
  using LineStyle = cairowindow::draw::LineStyle;
  namespace color = cairowindow::color;

  Point<CS::pixels> const csOrigin_pixels = Point<cs>{0, 0} * reference_transform_;
  Point<CS::pixels> const csP10_pixels    = Point<cs>{1, 0} * reference_transform_;
  Point<CS::pixels> const csP01_pixels    = Point<cs>{0, 1} * reference_transform_;

  math::Point origin(csOrigin_pixels.x(), csOrigin_pixels.y());         // The (0, 0) (cs coordinates) point, in pixels coordinates.
  math::Point P10(csP10_pixels.x(), csP10_pixels.y());                  // The (1, 0) (cs coordinates) point, in pixels coordinates.
  math::Point P01(csP01_pixels.x(), csP01_pixels.y());                  // The (0, 1) (cs coordinates) point, in pixels coordinates.
  math::Direction x_direction(origin, P10);
  math::Direction y_direction(origin, P01);

  // Draw the x-axis.
  lines_.emplace_back(display_line(layer, LineStyle({.line_color = color::red, .line_width = 1.0}), {origin, x_direction}));
  // Draw the y-axis.
  lines_.emplace_back(display_line(layer, LineStyle({.line_color = color::red, .line_width = 1.0}), {origin, y_direction}));
}

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

    Size<CS::pixels> const ObjectSize_pixels{object_width, object_height};
    Size<CS::centered> const ObjectSize_centered = ObjectSize_pixels * centered_transform_pixels.inverse();
    Dout(dc::notice, "ObjectSize_centered = " << ObjectSize_centered);

    Point<CS::painter> PainterOrigin_painter;
    auto painter_transform_centered = Transform<CS::painter, CS::centered>{}.translate(-0.5 * TranslationVector{ObjectSize_centered});
    Dout(dc::notice, "painter_transform_centered = " << painter_transform_centered);

    Point<CS::centered> PainterOrigin_centered = PainterOrigin_painter * painter_transform_centered;
    Dout(dc::notice, "PainterOrigin_centered = " << PainterOrigin_centered);

    auto painter_transform_pixels = painter_transform_centered * centered_transform_pixels;

    // Display the painter-coordinate-system.
    CoordinateSystem<CS::painter> painter_coordinate_system(painter_transform_pixels);
    painter_coordinate_system.display(layer);

    // Display the object of ObjectSize_centered (centered-coordinate-system) with the top-left in the origin of the painter-coordinate-system (PainterOrigin).
    auto object1 = std::make_shared<Shape>(Rectangle{PainterOrigin_centered, ObjectSize_centered}, ShapeStyleParams{.line_color = color::black, .shape = cwdraw::rectangle});
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
