#include "sys.h"
#include "CoordinateSystem.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/plot/Point.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include <iostream>
#include <thread>
#include "debug.h"

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
    namespace cs = cairowindow::cs;
    namespace color = cairowindow::color;
    namespace draw = cairowindow::draw;
    namespace plot = cairowindow::plot;

    Window window("Bounding box test", 1280, 720);

    auto background_layer = window.create_background_layer<Layer>(color::white COMMA_DEBUG_ONLY("background_layer"));
    auto layer = window.create_layer<Layer>({} COMMA_DEBUG_ONLY("layer"));
    (void)background_layer;

    // Put the origin in the center of the window.
    auto const geometry = window.geometry();
    cs::Size<CS::pixels> const half_window_size(0.5 * geometry.width(), 0.5 * geometry.height());
    auto const painter_transform_pixels = Transform<CS::painter, CS::pixels>{}.translate(half_window_size);
    CoordinateSystem<CS::painter> painter_coordinate_system(painter_transform_pixels, geometry);

    draw::PointStyle const point_style({.color_index = 0, .filled_shape = 3});   // Filled ellipse.
    plot::cs::Point<CS::painter> point_painter{0.0, 0.0};
    painter_coordinate_system.add_point(layer, point_style, point_painter);

    std::cout << "Hello World\n";

    // Open window, handle event loop in a separate thread. This must be constructed after the draw stuff, so that it is destructed first!
    std::thread event_loop([&](){
      Debug(NAMESPACE_DEBUG::init_thread("event_loop"));
      {
        EventLoop event_loop(window.run());
        event_loop.set_cleanly_terminated();
      }
      Dout(dc::cairowindow, "Leaving event_loop thread!");
    });

    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }

  Dout(dc::notice, "Leaving main()");
}
