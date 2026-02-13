#include "sys.h"
#include "cairowindow/CoordinateSystem.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/plot/Point.h"
#include "cairowindow/plot/Rectangle.h"
#include "math/cs/Direction.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include "utils/ColorPool.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <thread>
#include "debug.h"

using math::Transform;
using math::CS;

template<CS cs>
using CoordinateSystem = cairowindow::CoordinateSystem<cs>;

namespace csid {
using namespace math::csid;

DECLARE_CSID(centered);         // Coordinate system with origin in the middle of the window, same scale as the window (1 per pixel).
DECLARE_CSID(p2frame);          // Coordinate system with origin at P2 and axes aligned with the two perpendicular lines.
} // namespace csid

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  try
  {
    using EventLoop = cairowindow::EventLoop;
    using Layer = cairowindow::Layer;
    using Window = cairowindow::Window;
    using Geometry = cairowindow::Geometry;
    namespace cs = cairowindow::cs;
    namespace color = cairowindow::color;
    namespace draw = cairowindow::draw;
    namespace plot = cairowindow::plot;

    Window window("Bounding box test", 1280, 720);

    [[maybe_unused]] auto background_layer = window.create_background_layer<Layer>(color::white COMMA_DEBUG_ONLY("background_layer"));
    auto layer = window.create_layer<Layer>({} COMMA_DEBUG_ONLY("layer"));

    // Put the origin of `centered` in the center of the window.
    Geometry const window_geometry = window.geometry();
    math::cs::Size<csid::pixels> const half_window_size(0.5 * window_geometry.width(), 0.5 * window_geometry.height());
    auto const centered_transform_pixels = Transform<csid::centered, csid::pixels>{}.translate(half_window_size);
    CoordinateSystem<csid::centered> centered_coordinate_system(centered_transform_pixels, window_geometry);
    using CenteredPoint = math::cs::Point<csid::centered>;
    using CenteredDirection = math::cs::Direction<csid::centered>;

    // Determine a circle radius in pixels.
    double const circle_radius_pixels = 0.15 * half_window_size.width();

    // Draw styles being used.
    utils::ColorPool<32> color_pool;
    draw::PointStyle const P0_style({.color_index = color_pool.get_and_use_color(), .filled_shape = 10});
    draw::PointStyle const P1_style({.color_index = color_pool.get_and_use_color(), .filled_shape = 10});
    draw::PointStyle const P2_style({.color_index = color_pool.get_and_use_color(), .filled_shape = 10});
    draw::PointStyle const Q2_style({.color_index = color_pool.get_and_use_color(), .filled_shape = 10});
    draw::RectangleStyle const rectangle_style({.line_color = color::black, .line_width = 2.0});
    draw::LineStyle const circle_style({.line_color = color::gainsboro, .line_width = 1.0});
    draw::LineStyle const line_style({.line_color = color::fuchsia, .line_width = 1.0});
    draw::PointStyle const ip_style({.color_index = color_pool.get_and_use_color(), .filled_shape = 5});
    draw::TextStyle label_style({.position = draw::centered_below, .font_size = 18.0, .offset = 10});

    // Construct an centered-Axes Aligned Bounding Box from two points.
    auto AABB_centered_from = [](auto const& p0, auto const& p1) -> cs::Rectangle<csid::centered>
    {
      double const left = std::min(p0.x(), p1.x());
      double const right = std::max(p0.x(), p1.x());
      double const top = std::min(p0.y(), p1.y());
      double const bottom = std::max(p0.y(), p1.y());
      return {left, top, right - left, bottom - top};
    };

    //-------------------------------------------------------------------------
    // Draggables
    //
    plot::cs::Point<csid::centered> plot_P0{-400.0, -200.0};    // Top-left corner of the rectangle.
    plot::cs::Point<csid::centered> plot_P1{400.0, 200.0};      // Bottom-right corner of the rectangle.
    plot::cs::Point<csid::centered> plot_P2{0.0, 0.0};          // A point inside the rectangle.

    // Initial direction of D2 and D3.
    CenteredDirection D2{CenteredDirection::right};             // The direction of the x-axis of the P2-coordinate-system.
    CenteredDirection D3{CenteredDirection::right};             // The direction perpendicular to one of the edges of (minimal) rectangle through all four intersection points.
    // Radius of control circle for D2.
    double const circle_radius = circle_radius_pixels / centered_transform_pixels.x_scale();
    double const circle_radius3 = 2.0 * circle_radius;

    // A draggable point on a circle around P2.
    //
    plot::cs::Point<csid::centered> plot_Q2{plot_P2 + circle_radius * D2};
    plot::cs::Point<csid::centered> plot_Q3{plot_P2 + circle_radius3 * D3};

    // Create a coordinate system with origin in P2 and axes aligned with the two perpendicular lines through P2.
    //
    // p2frame -> centered is a rotation by angle(D2) followed by a translation by P2.
    // Since centered -> pixels is just a translation (origin in the center of the window), we can compose both transforms.
    auto make_p2frame_transform_pixels = [&]() -> Transform<csid::p2frame, csid::pixels>
    {
      double const theta = D2.as_angle();
      auto const p2frame_transform_centered = Transform<csid::p2frame, csid::centered>{}.translate(plot_P2).rotate(theta);
      return p2frame_transform_centered * centered_transform_pixels;
    };

    // Current bounding box. This is updated inside the main loop; the P2 restriction captures it by reference.
    cs::Rectangle<csid::centered> AABB_centered = AABB_centered_from(plot_P0, plot_P1);

    // Restrict a point to stay within the bounding box, away from all edges.
    //
    // We interpret "stay away from the edge" as a margin on all sides. The requested margin is 1/1000th
    // of the width of the rectangle. For the y-direction we cap the margin so that the point remains
    // placeable even for very flat rectangles.
    auto restrict_to_AABB_inner_area = [](CenteredPoint const& position, cs::Rectangle<csid::centered> const& aabb) -> CenteredPoint
    {
      double const left = aabb.offset_x();
      double const right = aabb.offset_x() + aabb.width();
      double const top = aabb.offset_y();
      double const bottom = aabb.offset_y() + aabb.height();

      double const margin_x = aabb.width() / 1000.0;
      double const margin_y = std::min(margin_x, 0.5 * aabb.height());

      double const restricted_x = std::clamp(position.x(), left + margin_x, right - margin_x);
      double const restricted_y = std::clamp(position.y(), top + margin_y, bottom - margin_y);

      return {restricted_x, restricted_y};
    };

    centered_coordinate_system.add_point(layer, P0_style, plot_P0);
    centered_coordinate_system.add_point(layer, P1_style, plot_P1);
    centered_coordinate_system.add_point(layer, P2_style, plot_P2);
    centered_coordinate_system.add_point(layer, Q2_style, plot_Q2);
    centered_coordinate_system.add_point(layer, Q2_style, plot_Q3);

    window.register_draggable(centered_coordinate_system, &plot_P0);
    window.register_draggable(centered_coordinate_system, &plot_P1);

    using Restriction = std::function<CenteredPoint(CenteredPoint const&)>;
    window.register_draggable(centered_coordinate_system, &plot_P2,
        Restriction{[&](CenteredPoint const& new_position) -> CenteredPoint
        {
          CenteredPoint restricted_position = restrict_to_AABB_inner_area(new_position, AABB_centered);
          // Keep the directions D2 and D3 invariant while moving P2 (see cairowindow/tests/scale_test.cxx).
          plot_Q2.move_to(plot_P2 + circle_radius * D2);
          plot_Q3.move_to(plot_P2 + circle_radius3 * D3);
          return restricted_position;
        }});

    // Keep Q2 on the circle around P2.
    window.register_draggable(centered_coordinate_system, &plot_Q2,
        Restriction{[&](CenteredPoint const& new_position) -> CenteredPoint
        {
          D2 = CenteredDirection{plot_P2, new_position};
          return plot_P2 + circle_radius * D2;
        }});

    window.register_draggable(centered_coordinate_system, &plot_Q3,
        Restriction{[&](CenteredPoint const& new_position) -> CenteredPoint
        {
          D3 = CenteredDirection{plot_P2, new_position};
          return plot_P2 + circle_radius3 * D3;
        }});

    //
    //-------------------------------------------------------------------------

    // Open window, handle event loop in a separate thread. This must be constructed after the draw stuff, so that it is destructed first!
    std::thread event_loop([&](){
      Debug(NAMESPACE_DEBUG::init_thread("event_loop"));
      {
        EventLoop event_loop(window.run());
        event_loop.set_cleanly_terminated();
      }
      Dout(dc::cairowindow, "Leaving event_loop thread!");
    });

    // Main loop.
    while (true)
    {
      // Suppress immediate updating of the window for each created item, in order to avoid flickering.
      window.set_send_expose_events(false);

      // Update the bounding box for this redraw.
      AABB_centered = AABB_centered_from(plot_P0, plot_P1);

      // If AABB_centered changed because P0 or P1 was dragged, then P2 might now be is outside
      // this updated bounding box (or too close to an edge), clamp it back inside.
      CenteredPoint const restricted_P2 = restrict_to_AABB_inner_area(plot_P2, AABB_centered);
      if (plot_P2 != restricted_P2)
        plot_P2.move_to(restricted_P2);

      // Create the p2frame coordinate system that follows P2 and D2.
      CoordinateSystem<csid::p2frame> p2frame_coordinate_system(make_p2frame_transform_pixels(), window_geometry);
      p2frame_coordinate_system.display(layer, draw::LineStyle({.line_color = color::red, .line_width = 1.0}));

      // Draw a rectangle between P0 and P1.
      auto plot_rectangle_centered = centered_coordinate_system.create_rectangle(layer, rectangle_style, AABB_centered);

      // Draw a circle around P2.
      auto plot_circle_centered = centered_coordinate_system.create_circle(layer, circle_style, plot_P2, circle_radius);
      auto plot_circle3_centered = centered_coordinate_system.create_circle(layer, circle_style, plot_P2, circle_radius3);

      // Draw lines through P2 and Q2, and perpendicular to that.
      std::array<math::cs::Line<csid::centered>, 2> lines;
      for (int l = 0; l <= 1; ++l)
        lines[l] = centered_coordinate_system.create_line(layer, line_style, plot_P2, l == 0 ? D2 : D2.rotated_90_degrees());

      // The plot- intersection points.
      std::array<std::vector<plot::cs::Point<csid::centered>>, 2> plot_ips;
      //std::array<std::vector<plot::Text>, 2> plot_labels;
      {
        // Hyperblock and Hyperplane are not CS-aware. Use raw types (still in csid::centered!)
        math::Vector<2> const P0{plot_P0.raw()};
        math::Vector<2> const P1{plot_P1.raw()};
        math::Vector<2> const P2{plot_P2.raw()};
        math::Hyperblock<2> const rectangle{P0, P1};
        std::array<math::Vector<2>, 2> normal;
        std::array<math::Hyperblock<2>::IntersectionPoints, 2> ips;
        std::vector<math::Hyperplane<2>> hyperplanes;
        for (int l = 0; l <= 1; ++l)
        {
          normal[l] = lines[l].raw().direction().rotated_90_degrees();
          hyperplanes.emplace_back(normal[l], -normal[l].dot(P2));
          // Calculate intersection points of line0 and line1 with the rectangle.
          ips[l] = rectangle.intersection_points(hyperplanes[l]);
          // Run over all intersection points of line l.
          for (auto const& ip : ips[l])
          {
            // We know that the coordinates of these intersection points are in 'centered' coordinates.
            math::cs::Vector<csid::centered> ip_cs{ip};
            plot_ips[l].push_back(centered_coordinate_system.create_point(layer, ip_style, ip_cs.as_point()));
            //plot_labels[l].push_back(centered_coordinate_system.create_text(layer, label_style, plot_ips[l].back(), std::to_string(l) + "." + std::to_string(plot_labels[l].size())));
          }
        }
      }

      // Store intersection points counter-clockwise in an array.
      constexpr int x_axis = 0;
      constexpr int y_axis = 1;
      constexpr int negative_side = 0;
      constexpr int positive_side = 1;
      auto centered_transform_p2frame = centered_transform_pixels * p2frame_coordinate_system.cs_transform_pixels().inverse();
      std::array<math::cs::Point<csid::p2frame>, 4> const ips = {
        plot_ips[x_axis][positive_side] * centered_transform_p2frame,
        plot_ips[y_axis][positive_side] * centered_transform_p2frame,
        plot_ips[x_axis][negative_side] * centered_transform_p2frame,
        plot_ips[y_axis][negative_side] * centered_transform_p2frame,
      };

      // Select and draw one line.
      //
      // For each intersection point, consider the line through that point with direction perpendicular to D3.
      // Let T be the intersection of that line with the line through P2 and Q3.
      // We choose the line that maximizes dot(D3, vector(P2, T)).
      math::cs::Point<csid::p2frame> const origin_p2frame{0.0, 0.0};                                    // P2 is the origin of p2frame by definition.
      math::cs::Direction<csid::p2frame> const D3_p2frame = D3 * centered_transform_p2frame;
      math::cs::Direction<csid::p2frame> const D3_perp_p2frame = D3_p2frame.rotated_90_degrees();
      math::cs::Line<csid::p2frame> const line_P2_Q3_p2frame{origin_p2frame, D3_p2frame};               // Line through P2 and Q3.

      int best_ip = -1;
      double best_dot = std::numeric_limits<double>::lowest();
      for (int i = 0; i < ips.size(); ++i)
      {
        math::cs::Line<csid::p2frame> const candidate_line{ips[i], D3_perp_p2frame};
        math::cs::Point<csid::p2frame> const T = candidate_line.intersection_with(line_P2_Q3_p2frame);

        math::cs::Vector<csid::p2frame> const vector_to_T{T};   // Vector from origin (P2) to T.
        double const dot = vector_to_T.dot(D3_p2frame);
        if (dot > best_dot)
        {
          best_dot = dot;
          best_ip = i;
        }
      }
      auto selected_line = p2frame_coordinate_system.create_line(layer, line_style({.line_color = color::blue}), ips[best_ip], D3_perp_p2frame);

      // Flush all expose events related to the drawing done above.
      window.set_send_expose_events(true);

      // Block until a redraw is necessary (for example because the user moved a draggable object,
      // or wants to print the current drawing) then go to the top of loop for a redraw.
      if (!window.handle_input_events())
        break;          // Program must be terminated.
    }

    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }

  Dout(dc::notice, "Leaving main()");
}
