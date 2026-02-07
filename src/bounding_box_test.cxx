#include "sys.h"
#include "cairowindow/CoordinateSystem.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/plot/Point.h"
#include "cairowindow/plot/Rectangle.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include "utils/ColorPool.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <thread>
#include "debug.h"

namespace csid = math::csid;
using math::Transform;
using math::CS;

template<CS cs>
using CoordinateSystem = cairowindow::CoordinateSystem<cs>;

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

    auto background_layer = window.create_background_layer<Layer>(color::white COMMA_DEBUG_ONLY("background_layer"));
    auto layer = window.create_layer<Layer>({} COMMA_DEBUG_ONLY("layer"));
    (void)background_layer;

    // Put the origin in the center of the window.
    Geometry const geometry = window.geometry();
    math::cs::Size<csid::pixels> const half_window_size(0.5 * geometry.width(), 0.5 * geometry.height());
    auto const painter_transform_pixels = Transform<csid::painter, csid::pixels>{}.translate(half_window_size);
    CoordinateSystem<csid::painter> painter_coordinate_system(painter_transform_pixels, geometry);

    utils::ColorPool<32> color_pool;
    draw::PointStyle const point0_style({.color_index = color_pool.get_and_use_color(), .filled_shape = 10});
    draw::PointStyle const point1_style({.color_index = color_pool.get_and_use_color(), .filled_shape = 10});
    draw::RectangleStyle const rectangle_style({.line_color = color::black, .line_width = 2.0});

    auto bounding_box_from = [](auto const& p0, auto const& p1) -> cs::Rectangle<csid::painter>
    {
      double const left = std::min(p0.x(), p1.x());
      double const right = std::max(p0.x(), p1.x());
      double const top = std::min(p0.y(), p1.y());
      double const bottom = std::max(p0.y(), p1.y());
      return {left, top, right - left, bottom - top};
    };

    plot::cs::Point<csid::painter> point0_painter{-100.0, -50.0};
    plot::cs::Point<csid::painter> point1_painter{100.0, 50.0};

    painter_coordinate_system.add_point(layer, point0_style, point0_painter);
    painter_coordinate_system.add_point(layer, point1_style, point1_painter);

    window.register_draggable(painter_coordinate_system, &point0_painter);
    window.register_draggable(painter_coordinate_system, &point1_painter);

    // Open window, handle event loop in a separate thread. This must be constructed after the draw stuff, so that it is destructed first!
    std::thread event_loop([&](){
      Debug(NAMESPACE_DEBUG::init_thread("event_loop"));
      {
        EventLoop event_loop(window.run());
        event_loop.set_cleanly_terminated();
      }
      Dout(dc::cairowindow, "Leaving event_loop thread!");
    });

    while (true)
    {
      // Suppress immediate updating of the window for each created item, in order to avoid flickering.
      window.set_send_expose_events(false);

      // Draw a rectangle between point0 and point1.
      auto plot_rectangle_painter = painter_coordinate_system.create_rectangle(layer, rectangle_style, bounding_box_from(point0_painter, point1_painter));

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
