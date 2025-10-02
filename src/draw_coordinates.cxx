#include "sys.h"
#include "CoordinateSystem.h"
#include "Transform.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/draw/Shape.h"
#include "cairowindow/draw/Line.h"
#include "cairowindow/draw/Text.h"
#include "cairowindow/draw/PlotArea.h"
#include "cairowindow/intersection_points.h"
#include "math/Line.h"
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#include <vector>
#include "debug.h"

// Draw a rectangle using painter->drawRect.
auto draw_rectangle(boost::intrusive_ptr<cairowindow::Layer> const& layer, Transform<CS::painter, CS::pixels> const& painter_transform_pixels,
    Point<CS::painter> const& topleft_painter, Size<CS::painter> const& size_painter, cairowindow::draw::RectangleStyle const& rectangle_style)
{
  DoutEntering(dc::notice, "draw_rectangle(layer, " << painter_transform_pixels << ", " << topleft_painter << ", " << size_painter << ", rectangle_style)");

  Point<CS::painter> bottomright_painter = topleft_painter + size_painter;
  Point<CS::painter> topright_painter(topleft_painter.x() + size_painter.width(), topleft_painter.y());
  Point<CS::painter> bottomleft_painter(topleft_painter.x(), topleft_painter.y() + size_painter.height());
  Point<CS::pixels> topleft     = topleft_painter     * painter_transform_pixels;
  Point<CS::pixels> bottomright = bottomright_painter * painter_transform_pixels;
  Point<CS::pixels> topright    = topright_painter    * painter_transform_pixels;
  Point<CS::pixels> bottomleft  = bottomleft_painter  * painter_transform_pixels;

  // Because the rectangle is potentially rotated, we have to draw four lines.
  std::array<std::shared_ptr<cairowindow::draw::Line>, 4> object = {
    std::make_shared<cairowindow::draw::Line>(topleft.x(), topleft.y(), topright.x(), topright.y(), rectangle_style),
    std::make_shared<cairowindow::draw::Line>(topright.x(), topright.y(), bottomright.x(), bottomright.y(), rectangle_style),
    std::make_shared<cairowindow::draw::Line>(bottomright.x(), bottomright.y(), bottomleft.x(), bottomleft.y(), rectangle_style),
    std::make_shared<cairowindow::draw::Line>(bottomleft.x(), bottomleft.y(), topleft.x(), topleft.y(), rectangle_style)
  };
  for (int i = 0; i < object.size(); ++i)
    layer->draw(object[i]);
  return object;
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
    using RectangleStyle = cairowindow::draw::RectangleStyle;
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

    // Transformation from centered to pixels.
    Transform<CS::centered, CS::pixels> const centered_transform_pixels =
      Transform<CS::centered, CS::pixels>{}.translate(half_window_size).scale(half_window_size.height());
    Dout(dc::notice, "centered_transform_pixels = " << centered_transform_pixels);

    Size<CS::pixels> const ObjectSize_pixels{object_width, object_height};
    Dout(dc::notice, "ObjectSize_pixels = " << ObjectSize_pixels);

    Size<CS::centered> const ObjectSize_centered = ObjectSize_pixels * centered_transform_pixels.inverse();
    Dout(dc::notice, "ObjectSize_centered = " << ObjectSize_centered);

    for (double a = 0.0; a < 360.0; a += 15.0)
    {
      auto const painter_transform_centered = Transform<CS::painter, CS::centered>{}.translate(-0.5 * TranslationVector{ObjectSize_centered}).rotate(a).scale(0.000001);
      Dout(dc::notice, "painter_transform_centered = " << painter_transform_centered);

      Point<CS::painter> PainterOrigin_painter;
      Dout(dc::notice, "PainterOrigin_painter = " << PainterOrigin_painter);

      Point<CS::centered> PainterOrigin_centered = PainterOrigin_painter * painter_transform_centered;
      Dout(dc::notice, "PainterOrigin_centered = " << PainterOrigin_centered);

      auto const painter_transform_pixels = painter_transform_centered * centered_transform_pixels;
      Dout(dc::notice, "painter_transform_pixels = " << painter_transform_pixels);

      Size<CS::painter> const ObjectSize_painter = ObjectSize_pixels * painter_transform_pixels.inverse();
      Dout(dc::notice, "ObjectSize_painter = " << ObjectSize_painter);

      // Display the painter-coordinate-system.
      draw::CoordinateSystem<CS::centered> centered_coordinate_system(centered_transform_pixels, LineStyle({.line_color = color::green, .line_width = 1.0}));
      centered_coordinate_system.display(layer);

      // Display the painter-coordinate-system.
      draw::CoordinateSystem<CS::painter> painter_coordinate_system(painter_transform_pixels, LineStyle({.line_color = color::red, .line_width = 1.0}));
      painter_coordinate_system.display(layer);

      // Display the rectangle of ObjectSize_centered (centered-coordinate-system) with the top-left in the origin of the painter-coordinate-system (PainterOrigin).
      auto object1 = draw_rectangle(layer, painter_transform_pixels, PainterOrigin_painter, ObjectSize_painter, RectangleStyle({.line_color = color::black}));

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
