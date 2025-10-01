#pragma once

#include "Transform.h"
#include "Point.h"
#include "Line.h"
#include "Rectangle.h"
#include "Range.h"
#include "Vector.h"
#include "NiceDelta.h"
#include "cairowindow/draw/Point.h"
#include "cairowindow/draw/PlotArea.h"          // number_of_axis, calculate_range_ticks
#include "cairowindow/draw/Line.h"
#include "cairowindow/draw/Rectangle.h"
#include "cairowindow/draw/Text.h"
#include "cairowindow/draw/Arc.h"
#include "cairowindow/draw/Circle.h"
#include "cairowindow/draw/Connector.h"
#include "cairowindow/Layer.h"
#include "cairowindow/Range.h"
#include "cairowindow/intersection_points.h"
#include "math/Point.h"
#include "math/Line.h"
#include "math/Direction.h"
#include <boost/intrusive_ptr.hpp>
#include <vector>

#if 0
//FIXME: move to .cxx
#include "cairowindow/intersection_points.h"
inline std::shared_ptr<cairowindow::draw::Line> display_line(
    boost::intrusive_ptr<Layer> const& layer,
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
#endif

#if 0
template<CS cs>
class CoordinateSystem : public cairowindow::draw::PlotArea
{
 private:
  std::vector<std::shared_ptr<cairowindow::draw::Line>> lines_;         // draw::Line objects that are part of the CoordinateSystem drawing.
  std::vector<std::shared_ptr<cairowindow::draw::Text>> texts_;         // draw::Text objects that are part of the CoordinateSystem drawing.
  Transform<cs, CS::pixels> reference_transform_;                       // The Transform defining this CoordinateSystem.

 public:
  // Construct a CoordinateSystem from a Transform. Call `display` to draw it.
  CoordinateSystem(Transform<cs, CS::pixels> const& reference_transform, cairowindow::draw::PlotAreaStyle const& plot_area_style) :
    cairowindow::draw::PlotArea({0, 0, window_width, window_height}, plot_area_style),
    reference_transform_(reference_transform)
  {
    DoutEntering(dc::notice, "CoordinateSystem<" << utils::to_string(cs) << ">::CoordinateSystem(" <<
        reference_transform << ", " << plot_area_style << ") [" << this << "]");
  }

  void display(boost::intrusive_ptr<Layer> const& layer);
};

template<CS cs>
void CoordinateSystem<cs>::display(boost::intrusive_ptr<Layer> const& layer)
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
#endif

namespace draw {

// Things from cairowindow::draw that we can use as-is.
using PlotArea = cairowindow::draw::PlotArea;
//using PlotAreaStyle = cairowindow::draw::PlotAreaStyle;
using LineStyle = cairowindow::draw::LineStyle;
//using PointStyle = cairowindow::draw::PointStyle;
//using ConnectorStyle = cairowindow::draw::ConnectorStyle;
//using RectangleStyle = cairowindow::draw::RectangleStyle;
//using CircleStyle = cairowindow::draw::CircleStyle;
//using ArcStyle = cairowindow::draw::ArcStyle;
//using TextStyle = cairowindow::draw::TextStyle;

// Things from cairowindow::plot that we can use as-is.
using cairowindow::plot::x_axis;
using cairowindow::plot::y_axis;

template<CS cs>
class CoordinateSystem
{
  static constexpr int number_of_axes = PlotArea::number_of_axes;
  using Direction = cairowindow::Direction;

 private:
  Transform<cs, CS::pixels> cs_transform_pixels_;                       // The Transform defining this CoordinateSystem.
  LineStyle axis_style_;                                                // The linestyle to use for the axes and tickmarks.
  Point<CS::pixels> csOrigin_pixels_;                                   // The origin in pixels.
  std::array<Direction, number_of_axes> csAxisDirection_;               // The direction of the x-axis and y-axis (in CS::pixels).
  std::vector<std::shared_ptr<cairowindow::draw::Line>> lines_;         // To keep drawn lines alive.
  std::array<math::LinePiece, number_of_axes> line_piece_;              // The visible part of the axes (in CS::pixels).

 private:
  using LayerPtr = boost::intrusive_ptr<cairowindow::Layer>;
  using PointPtr = std::shared_ptr<Point<cs>>;
  using LinePtr = std::shared_ptr<Line<cs>>;
  using RectanglePtr = std::shared_ptr<Rectangle<cs>>;
//  using LinePiecePtr = std::shared_ptr<LinePiece<cs>>;
//  using CirclePtr = std::shared_ptr<Circle<cs>>;
//  using ArcPtr = std::shared_ptr<Arc<cs>>;
//  using TextPtr = std::shared_ptr<Text<cs>>;

/*  std::shared_ptr<Text> xlabel_;
  std::shared_ptr<Text> ylabel_;*/
  std::array<Range<cs>, number_of_axes> range_{{{0.0, 0.0}, {0.0, 0.0}}};       // Zero means: not visible.
  std::array<NiceDelta<cs>, number_of_axes> range_ticks_;                       // The number of tick marks on the visible segment of the respective axis.
                                                                                // Invalid (default constructed) means: don't draw ticks.
/*  std::array<std::vector<std::shared_ptr<Text>>, number_of_axes> labels_;*/

 public:
  CoordinateSystem(Transform<cs, CS::pixels> const reference_transform, LineStyle axis_style);

#if 0
  CoordinateSystem(PlotAreaStyle plot_area_style,
      std::string xlabel, XLabelStyle xlabel_style, std::string ylabel, YLabelStyle ylabel_style) :
    plot_area_(axes_geometry(plot_area_style.axes_line_width()), plot_area_style),
    xlabel_(std::make_shared<Text>(xlabel, plot_area_.geometry().offset_x() + 0.5 * plot_area_.geometry().width(),
        plot_area_.geometry().offset_y() + plot_area_.geometry().height() + XLabelStyleDefaults::offset, xlabel_style)),
    ylabel_(std::make_shared<Text>(ylabel, plot_area_.geometry().offset_x() - YLabelStyleDefaults::offset,
        plot_area_.geometry().offset_y() + 0.5 * plot_area_.geometry().height(), ylabel_style)) { }
#endif

  void set_range(int axis, Range<cs> range)
  {
    DoutEntering(dc::notice, "CoordinateSystem::set_range(" << axis << ", " << range << ") [" << this << "]");
    range_[axis] = range;
    range_ticks_[axis] = NiceDelta<cs>{range};
    Dout(dc::notice, "range_[" << axis << "] = " << range_[axis] << "; range_ticks_[" << axis << "] = " << range_ticks_[axis]);
  }

  cairowindow::Point clamp_to_plot_area(cairowindow::Point const& point) const
  {
    return {std::clamp(point.x(), range_[x_axis].min(), range_[x_axis].max()),
            std::clamp(point.y(), range_[y_axis].min(), range_[y_axis].max())};
  }

  cairowindow::Range const& xrange() const { return range_[x_axis]; }
  cairowindow::Range const& yrange() const { return range_[y_axis]; }

  cairowindow::Rectangle viewport() const
  {
    return {range_[x_axis].min(), range_[y_axis].min(), range_[x_axis].size(), range_[y_axis].size()};
  }

#if 0
  double convert_x(double x) const;
  double convert_y(double y) const;
  Pixel convert_to_pixel(cairowindow::Point const& point) const;

  double convert_from_pixel_x(double pixel_x) const;
  double convert_from_pixel_y(double pixel_y) const;
  cairowindow::Point convert_from_pixel(Pixel const& pixel) const;

  double convert_horizontal_offset_from_pixel(double pixel_offset_x) const;
  double convert_vertical_offset_from_pixel(double pixel_offset_y) const;

  void convert_to_pixels(cairowindow::Point const* data_in, Pixel* data_out, std::size_t size);

  template<std::size_t size>
  [[gnu::always_inline]] void convert_to_pixels(std::array<cairowindow::Point, size> const& in, std::array<Pixel, size>& out)
  {
    convert_to_pixels(in.data(), out.data(), size);
  }
#endif

#if 0
  //--------------------------------------------------------------------------
  // Point

  // Add and draw cs_point on layer using point_style.
  void add_point(LayerPtr const& layer, PointStyle const& point_style, PointPtr const& cs_point);

 public:
  // Create and draw a point on layer at x,y using point_style.
  [[nodiscard]] PointPtr create_point(LayerPtr const& layer, PointStyle const& point_style, Point<cs> const& point)
  {
    PointPtr cs_point = std::make_shared<Point<cs>>(point);
    add_point(layer, point_style, cs_point);
    return cs_point;
  }
#endif

#if 0
  //--------------------------------------------------------------------------
  // LinePiece

  // Add and draw cs_line_piece on layer using line_style and line_extend.
  void add_line(LayerPtr const& layer, LineStyle const& line_style, LineExtend line_extend, LinePiecePtr const& cs_line_piece);

 private:
  void add_line(LayerPtr const& layer, LineStyle const& line_style, LineExtend line_extend, LinePiecePtr&& cs_line_piece);

 public:
  // Create and draw a line piece between points from and to using line_style and line_extend.
  template<typename... Args>
  [[nodiscard]] LinePiecePtr create_line(LayerPtr const& layer, LineStyle const& line_style, LineExtend line_extend, Args&&... args)
    requires requires(Args&&... args) { LinePiece<cs>{std::forward<Args>(args)...}; }
  {
    LinePiecePtr cs_line_piece = std::make_shared<LinePiece<cs>>(std::forward<Args>(args)...);
    add_line(layer, line_style, line_extend, cs_line_piece);
    return cs_line_piece;
  }

  // Same, but without a line_extend.
  template<typename... Args>
  [[nodiscard]] LinePiecePtr create_line(LayerPtr const& layer, LineStyle const& line_style, Args&&... args)
    requires requires(Args&&... args) { LinePiece<cs>{std::forward<Args>(args)...}; }
  {
    LinePiecePtr cs_line_piece = std::make_shared<LinePiece<cs>>(std::forward<Args>(args)...);
    add_line(layer, line_style, LineExtend::none, cs_line_piece);
    return cs_line_piece;
  }
#endif

#if 0
  //--------------------------------------------------------------------------
  // Connector

  // Add and draw cs_connector on layer, using line_style, fill_color, arrow_head_shape_from and arrow_head_shape_to.
  void add_connector(LayerPtr const& layer, ConnectorStyle const& style, ConnectorPtr const& cs_connector);

 private:
  void add_connector(LayerPtr const& layer, ConnectorStyle const& style, ConnectorPtr&& cs_connector);

 private:
  template<typename... Args>
  [[nodiscard]] ConnectorPtr create_connector_helper(
      Connector::ArrowHeadShape& default_arrow_head_shape_from,
      Connector::ArrowHeadShape& default_arrow_head_shape_to,
      LayerPtr const& layer,
      ConnectorStyle const& connector_style, std::tuple<Args...>&& args)
  {
    // Strip Connector::ArrowHeadShape arguments from args... and apply them to the defaults.
    if constexpr (std::is_same_v<std::tuple_element_t<0, std::tuple<Args...>>, typename Connector::ArrowHeadShape>)
    {
      if constexpr (std::is_same_v<std::tuple_element_t<1, std::tuple<Args...>>, typename Connector::ArrowHeadShape>)
        default_arrow_head_shape_from = std::get<0>(args);
      else
        default_arrow_head_shape_to = std::get<0>(args);
      return create_connector_helper(default_arrow_head_shape_from, default_arrow_head_shape_to,
          layer, connector_style, tuple_tail(std::move(args)));
    }
    else
    {
      // The defaults are now set. Construct a Connector from the remaining arguments and the default arrow head shapes.
      ConnectorPtr cs_connector = std::apply([&](auto&&... unpacked_args) -> ConnectorPtr {
        return std::make_shared<Connector<cs>>(std::forward<decltype(unpacked_args)>(unpacked_args)..., default_arrow_head_shape_from, default_arrow_head_shape_to);
      }, std::move(args));

      add_connector(layer, connector_style, cs_connector);
      return cs_connector;
    }
  }

 public:
  // Create and draw a connector on layer, using args... and line_style.
  // Args can optionally start with zero, one or two ArrowHeadShape arguments.
  template<typename... Args>
  [[nodiscard]] ConnectorPtr create_connector(LayerPtr const& layer,
      ConnectorStyle const& connector_style, Args&&... args)
  {
    Connector::ArrowHeadShape arrow_head_shape_from = Connector::no_arrow;
    Connector::ArrowHeadShape arrow_head_shape_to = Connector::open_arrow;
    return create_connector_helper(arrow_head_shape_from, arrow_head_shape_to, layer,
        connector_style, std::make_tuple(std::forward<Args>(args)...));
  }
#endif

#if 0
  //--------------------------------------------------------------------------
  // Line

  // Add and draw plot_line using line_style.
  void add_line(LayerPtr const& layer, LineStyle const& line_style, LinePtr const& cs_line);

 private:
  void add_line(LayerPtr const& layer, LineStyle const& line_style, LinePtr&& cs_line);

 public:
  // Create and draw a line through point in direction using line_style.
  template<typename... Args>
  [[nodiscard]] LinePtr create_line(LayerPtr const& layer, LineStyle const& line_style, Args&&... args)
  {
    LinePtr cs_line = std::make_shared<Line<cs>>(std::forward<Args>(args)...);
    add_line(layer, line_style, cs_line);
    return cs_line;
  }

  //--------------------------------------------------------------------------
  // Rectangle

  // Add and draw plot_rectangle on layer, using rectangle_style.
  void add_rectangle(LayerPtr const& layer, RectangleStyle const& rectangle_style, RectanglePtr const& cs_rectangle);

 private:
  void add_rectangle(LayerPtr const& layer, RectangleStyle const& rectangle_style, RectanglePtr&& cs_rectangle);

 public:
  // Create and draw a rectangle on layer, using args... and rectangle_style.
  template<typename... Args>
  [[nodiscard]] RectanglePtr create_rectangle(LayerPtr const& layer,
      RectangleStyle const& rectangle_style, Args&&... args)
  {
    RectanglePtr cs_rectangle = std::make_shared<Rectangle<cs>>(std::forward<Args>(args)...);
    add_rectangle(layer, rectangle_style, cs_rectangle);
    return cs_rectangle;
  }
#endif

#if 0
  //--------------------------------------------------------------------------
  // Circle

  // Add and draw cs_circle on layer with center and radius using circle_style.
  void add_circle(LayerPtr const& layer, CircleStyle const& circle_style, CirclePtr const& cs_circle);

 private:
  void add_circle(LayerPtr const& layer, CircleStyle const& circle_style, CirclePtr&& cs_circle);

 public:
  // Create and draw a circle on layer with center and radius using circle_style.
  template<typename... Args>
  [[nodiscard]] CirclePtr create_circle(LayerPtr const& layer, CircleStyle const& circle_style, Args&&... args)
  {
    CirclePtr cs_circle = std::make_shared<Circle<cs>>(std::forward<Args>(args)...);
    add_circle(layer, circle_style, cs_circle);
    return cs_circle;
  }

  // Same as above but use line_style (no fill_color).
  template<typename... Args>
  [[nodiscard]] CirclePtr create_circle(LayerPtr const& layer, LineStyle const& line_style, Args&&... args)
  {
    return create_circle(layer, CircleStyle({.line_color = line_style.line_color(), .line_width = line_style.line_width()}), std::forward<Args>(args)...);
  }
#endif

#if 0
  //--------------------------------------------------------------------------
  // Arc

  // Add and draw cs_arc on layer using arc_style.
  void add_arc(LayerPtr const& layer, ArcStyle const& arc_style, ArcPtr const& cs_arc);

 private:
  void add_arc(LayerPtr const& layer, ArcStyle const& arc_style, ArcPtr&& cs_arc);

 public:
  // Create and draw an arc on layer width center, radius and start- and end_angle, using arc_style.
  template<typename... Args>
  [[nodiscard]] ArcPtr create_arc(LayerPtr const& layer, ArcStyle const& arc_style, Args&&... args)
  {
    ArcPtr cs_arc = std::make_shared<Arc<cs>>(std::forward<Args>(args)...);
    add_arc(layer, arc_style, cs_arc);
    return cs_arc;
  }
#endif

#if 0
  //--------------------------------------------------------------------------
  // Text

  // Add and draw cs_text on layer using text_style.
  void add_text(LayerPtr const& layer, TextStyle const& text_style, TextPtr const& cs_text);

 private:
  void add_text(LayerPtr const& layer, TextStyle const& text_style, TextPtr&& cs_text);

 public:
  // Create and draw text on layer at position using text_style.
  [[nodiscard]] TextPtr create_text(LayerPtr const& layer, TextStyle const& text_style, Point<cs> const& position, std::string const& text)
  {
    TextPtr cs_text = std::make_shared<Text<cs>>(convert_to_pixel(position), text);
    add_text(layer, text_style, cs_text);
    return cs_text;
  }

  // Same, but using pixel coordinates.
  [[nodiscard]] TextPtr create_text(LayerPtr const& layer, TextStyle const& text_style, Point<pixel> position, std::string const& text)
  {
    TextPtr cs_text = std::make_shared<Text<cs>>(position, text);
    add_text(layer, text_style, cs_text);
    return cs_text;
  }
#endif

  void display(LayerPtr const& layer);

 private:
//  void apply_line_extend(double& x1, double& y1, double& x2, double& y2, LineExtend line_extend);
};

template<CS cs>
CoordinateSystem<cs>::CoordinateSystem(Transform<cs, CS::pixels> const cs_transform_pixels, LineStyle axis_style) :
  cs_transform_pixels_(cs_transform_pixels), axis_style_{axis_style}
{
  // Calculate where the cs-axis intersect with the window geometry.

  // Construct three points on the CS axis.
  Point<cs> csOrigin_cs;
  Point<cs> csXAxisUnit_cs(1, 0);
  Point<cs> csYAxisUnit_cs(0, 1);
  // Convert them to pixel coordinates.
  csOrigin_pixels_ = csOrigin_cs * cs_transform_pixels;
  Point<CS::pixels> csXAxisUnit_pixels = csXAxisUnit_cs * cs_transform_pixels;
  Point<CS::pixels> csYAxisUnit_pixels = csYAxisUnit_cs * cs_transform_pixels;
  // Store the directions (in pixels coordinates).
  csAxisDirection_[x_axis] = Direction(csOrigin_pixels_, csXAxisUnit_pixels);
  csAxisDirection_[y_axis] = Direction(csOrigin_pixels_, csYAxisUnit_pixels);

  // The inverse transform.
  auto const& pixels_transform_cs = cs_transform_pixels.inverse();

  // Calcuate the length that is visible of each CS axis, in pixels.
  for (int axis = x_axis; axis <= y_axis; ++axis)
  {
    double normal_x = -csAxisDirection_[axis].y();
    double normal_y = csAxisDirection_[axis].x();
    intersections::HyperPlane<double, 2> line({normal_x, normal_y}, -csOrigin_pixels_.x() * normal_x - csOrigin_pixels_.y() * normal_y);
    intersections::HyperBlock<double, 2> rectangle({0, 0}, {window_width, window_height});
    auto intersections = rectangle.intersection_points(line);

    // Is the line outside the window?
    if (intersections.empty())
    {
      math::Point const origin(0, 0);
      line_piece_[axis] = math::LinePiece{origin, origin};      // Use twice the same point to encode that this axis is not visible within the window.
      continue;
    }

    // Get the two points where this cs-axis intersect with the window rectangle (in pixels).
    math::Point const p1{intersections[0][0], intersections[0][1]};
    math::Point const p2{intersections[1][0], intersections[1][1]};
    line_piece_[axis] = math::LinePiece(p1, p2);
    Dout(dc::notice, "line_piece_[" << axis << "] = " << line_piece_[axis]);

    // Convert the intersection points back to cs.
    Point<cs> const p1_cs = Point<CS::pixels>(p1.x(), p1.y()) * pixels_transform_cs;
    Point<cs> const p2_cs = Point<CS::pixels>(p2.x(), p2.y()) * pixels_transform_cs;
    Dout(dc::notice, "p1_cs = " << p1_cs);
    Dout(dc::notice, "p2_cs = " << p2_cs);
    // Extract the minimum and maximum values of the visible range.
    double min = (axis == x_axis) ? p1_cs.x() : p1_cs.y();
    double max = (axis == x_axis) ? p2_cs.x() : p2_cs.y();
    // We don't really know if p1 is on the left or right (above or below) p2.
    if (min > max)
      std::swap(min, max);
    // Set the range on each (visible) axis.
    set_range(axis, {min, max});
  }
}

template<CS cs>
void CoordinateSystem<cs>::display(LayerPtr const& layer)
{
  namespace color = cairowindow::color;

  for (int axis = x_axis; axis <= y_axis; ++axis)
  {
    // If the range is empty then min = max = 0 and size() will return zero exactly.
    if (range_[axis].size() == 0.0)     // Not visible?
      continue;
    // Draw the piece of the axis that is visible.
    lines_.emplace_back(std::make_shared<cairowindow::draw::Line>(
          line_piece_[axis].from().x(), line_piece_[axis].from().y(),
          line_piece_[axis].to().x(), line_piece_[axis].to().y(),
          axis_style_));
    layer->draw(lines_.back());

    if (range_ticks_[axis].is_invalid())
      continue;

    // Draw the tick marks.
    double const delta_cs = range_ticks_[axis].value();
    int k_min = std::ceil(range_[axis].min() / delta_cs);
    int k_max = std::floor(range_[axis].max() / delta_cs);
    for (int k = k_min; k <= k_max; ++k)
    {
      if (k == 0)
        continue;
      double value_cs = k * delta_cs;
      Point<cs> tick_cs{axis == x_axis ? value_cs : 0.0, axis == y_axis ? value_cs : 0.0};
      Point<CS::pixels> tick_pixels = tick_cs * cs_transform_pixels_;
      // Unit vector pointing into the positive direction of axis.
      Direction axis_pixels{csOrigin_pixels_, tick_pixels};
      Direction axis_tickmark_pixels = (axis == x_axis) == (k < 0) ? axis_pixels.normal() : axis_pixels.normal_inverse();
      Point<CS::pixels> tick_end_pixels = tick_pixels + Vector<CS::pixels>{axis_tickmark_pixels, 5.0};
      lines_.emplace_back(std::make_shared<cairowindow::draw::Line>(
            tick_pixels.x(), tick_pixels.y(),
            tick_end_pixels.x(), tick_end_pixels.y(),
            axis_style_));
      layer->draw(lines_.back());
    }
  }
}

//FIXME: add_* doesn't work like this: need to pass an object (eg plot::Point<cs>) derived from Point<cs> that also stores a std::shared_ptr<Point<pixels>.
#if 0

//--------------------------------------------------------------------------
// Point

template<CS cs>
void CoordinateSystem<cs>::add_point(LayerPtr const& layer, PointStyle const& point_style, PointPtr const& cs_point)
{
  double x = plot_point.x();
  double y = plot_point.y();

  plot_point.draw_object_ = std::make_shared<draw::Point>(convert_x(x), convert_y(y), point_style);
  draw_layer_region_on(layer, plot_point.draw_object_);
}

//--------------------------------------------------------------------------
// Line

template<CS cs>
void CoordinateSystem<cs>::add_line(LayerPtr const& layer, LineStyle const& line_style, LinePtr const& cs_line)
{
  cairowindow::Direction const& direction = plot_line.direction();
  cairowindow::Point const& point = plot_line.point();

  double normal_x = -direction.y();
  double normal_y = direction.x();
  intersections::HyperPlane<double, 2> line({normal_x, normal_y}, -point.x() * normal_x - point.y() * normal_y);
  intersections::HyperBlock<double, 2> rectangle({range_[x_axis].min(), range_[y_axis].min()}, {range_[x_axis].max(), range_[y_axis].max()});
  auto intersections = rectangle.intersection_points(line);

  // Is the line outside the plot area?
  if (intersections.empty())
    return;

  double x1 = intersections[0][0];
  double y1 = intersections[0][1];
  double x2 = intersections[1][0];
  double y2 = intersections[1][1];

  plot_line.draw_object_ = std::make_shared<draw::Line>(
      convert_x(x1), convert_y(y1), convert_x(x2), convert_y(y2),
      line_style);
  draw_layer_region_on(layer, plot_line.draw_object_);
}

//--------------------------------------------------------------------------
// Rectangle

template<CS cs>
void CoordinateSystem<cs>::add_rectangle(LayerPtr const& layer, RectangleStyle const& rectangle_style, RectanglePtr const& cs_rectangle)
{
  double offset_x = plot_rectangle.offset_x();
  double offset_y = plot_rectangle.offset_y();
  double width = plot_rectangle.width();
  double height = plot_rectangle.height();

  plot_rectangle.draw_object_ = std::make_shared<draw::Rectangle>(
      convert_x(offset_x), convert_y(offset_y), convert_x(offset_x + width), convert_y(offset_y + height),
      rectangle_style);
  draw_layer_region_on(layer, plot_rectangle.draw_object_);
}

//--------------------------------------------------------------------------
// Text

template<CS cs>
void CoordinateSystem<cs>::add_text(LayerPtr const& layer, TextStyle const& text_style, TextPtr const& cs_text)
{
  cairowindow::Pixel position = plot_text.position();
  std::string const& text = plot_text.text();

  plot_text.draw_object_ = std::make_shared<draw::Text>(text, position.x(), position.y(), text_style);
  draw_layer_region_on(layer, plot_text.draw_object_);
}
#endif

} // namespace draw
