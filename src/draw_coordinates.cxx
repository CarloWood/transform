#include "sys.h"
#include "window_size.h"
#include "cairowindow/CoordinateSystem.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/plot/Point.h"
#include "cairowindow/plot/Rectangle.h"
#include "cairowindow/plot/Line.h"
#include "cairowindow/draw/Shape.h"
#include "cairowindow/draw/Line.h"
#include "cairowindow/draw/Text.h"
#include "cairowindow/draw/Point.h"
#include "cairowindow/draw/PlotArea.h"
#include "math/Line.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include <vector>
#include "debug.h"

using math::CS;
using math::Transform;

template<CS cs>
using CoordinateSystem = cairowindow::CoordinateSystem<cs>;

namespace csid {
  using namespace math::csid;

  DECLARE_CSID(centered);       // Coordinate system with origin in the middle of the window, where -1 corresponds with the bottom of the window and 1 with the top.
} // namespace csid

#if 0
  else if (id == csid::centered.id)
    return "cs::centered";
#endif

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  Dout(dc::notice, "Entering main()");

  try
  {
    using EventLoop = cairowindow::EventLoop;
    using Layer = cairowindow::Layer;
    using Line = cairowindow::draw::Line;
    using LineStyle = cairowindow::draw::LineStyle;
    using PointStyle = cairowindow::draw::PointStyle;
    using RectangleStyle = cairowindow::draw::RectangleStyle;
    using Shape = cairowindow::draw::Shape;
    using Window = cairowindow::Window;
    namespace color = cairowindow::color;
    namespace cs = cairowindow::cs;
    namespace plot = cairowindow::plot;

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

    // Transformation from centered to pixels.
    Transform<csid::centered, csid::pixels> const centered_transform_pixels =
      Transform<csid::centered, csid::pixels>{}.translate(half_window_size).scale(half_window_size.height());
    Dout(dc::notice, "centered_transform_pixels = " << centered_transform_pixels);

    cs::Size<csid::pixels> const ObjectSize_pixels{object_width, object_height};
    Dout(dc::notice, "ObjectSize_pixels = " << ObjectSize_pixels);

    cs::Size<csid::centered> const ObjectSize_centered = ObjectSize_pixels * centered_transform_pixels.inverse();
    Dout(dc::notice, "ObjectSize_centered = " << ObjectSize_centered);

    for (int step = 0; step < 24; ++step)
    {
      double angle = step * M_PI / 12.0;
      math::TranslationVector<csid::centered> size_translation{ObjectSize_centered};
      auto const painter_transform_centered = Transform<csid::painter, csid::centered>{}.translate(-0.5 * size_translation).rotate(angle);
      Dout(dc::notice, "painter_transform_centered = " << painter_transform_centered);

      cs::Point<csid::painter> PainterOrigin_painter;
      Dout(dc::notice, "PainterOrigin_painter = " << PainterOrigin_painter);

      cs::Point<csid::centered> PainterOrigin_centered = PainterOrigin_painter * painter_transform_centered;
      Dout(dc::notice, "PainterOrigin_centered = " << PainterOrigin_centered);

      auto const painter_transform_pixels = painter_transform_centered * centered_transform_pixels;
      Dout(dc::notice, "painter_transform_pixels = " << painter_transform_pixels);

      cs::Size<csid::painter> const ObjectSize_painter = ObjectSize_pixels * painter_transform_pixels.inverse();
      Dout(dc::notice, "ObjectSize_painter = " << ObjectSize_painter);

      // Display the centered-coordinate-system.
      CoordinateSystem<csid::centered> centered_coordinate_system(centered_transform_pixels, win.geometry());
      centered_coordinate_system.display(layer, LineStyle({.line_color = color::green, .line_width = 1.0}));

      // Display the painter-coordinate-system.
      CoordinateSystem<csid::painter> painter_coordinate_system(painter_transform_pixels, win.geometry());
      painter_coordinate_system.display(layer, LineStyle({.line_color = color::red, .line_width = 1.0}));

      // Draw a test point at (1, -0.5) in both coordinate systems.
      PointStyle const point_style({.color_index = 0, .filled_shape = 3});   // Filled ellipse.
      plot::cs::Point<csid::centered> const point_centered(1.0, -0.5);
      plot::cs::Point<csid::painter> const point_painter(1.0, -0.5);
      centered_coordinate_system.add_point(layer, point_style, point_centered);
      painter_coordinate_system.add_point(layer, point_style, point_painter);
      // Draw a rectangle in painter with one corner in the origin and the other at ObjectSize_painter.
      plot::cs::Rectangle<csid::painter> const rectangle_painter(PainterOrigin_painter, ObjectSize_painter);
      painter_coordinate_system.add_rectangle(layer, RectangleStyle({.line_color = color::black}), rectangle_painter);
      // Draw a line in painter through the point (1, -0.5) with a slope of 1.
      cs::Direction slope{cs::Point<csid::painter>{1, 1}};
      plot::cs::Line<csid::painter> const line_painter(point_painter, slope);
      painter_coordinate_system.add_line(layer, LineStyle({.line_color = color::blue, .line_width = 1.0}), line_painter);

      std::cin.get();
    }

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
