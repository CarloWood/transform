#include "sys.h"
#include "CoordinateSystem.h"
#include "Transform.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/LayerRegion.h"
#include "cairowindow/plot/Point.h"
#include "cairowindow/plot/Rectangle.h"
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

// Draw a rectangle using painter->drawRect.
auto draw_rectangle(boost::intrusive_ptr<cairowindow::Layer> const& layer, Transform<CS::painter, CS::pixels> const& painter_transform_pixels,
    cairowindow::cs::Point<CS::painter> const& topleft_painter, cairowindow::cs::Size<CS::painter> const& size_painter, cairowindow::draw::RectangleStyle const& rectangle_style)
{
  DoutEntering(dc::notice, "draw_rectangle(layer, " << painter_transform_pixels << ", " << topleft_painter << ", " << size_painter << ", rectangle_style)");

  cairowindow::cs::Rectangle<CS::painter> const rectangle_painter(topleft_painter.x(), topleft_painter.y(), size_painter.width(), size_painter.height());

  // Convert painter_transform_pixels to a cairo_matrix_t by mapping the origin and the standard basis vectors.
  cairowindow::cs::Point<CS::pixels> const origin_pixels = cairowindow::cs::Point<CS::painter>{0.0, 0.0} * painter_transform_pixels;
  cairowindow::cs::Point<CS::pixels> const ex_pixels = cairowindow::cs::Point<CS::painter>{1.0, 0.0} * painter_transform_pixels;
  cairowindow::cs::Point<CS::pixels> const ey_pixels = cairowindow::cs::Point<CS::painter>{0.0, 1.0} * painter_transform_pixels;
  cairo_matrix_t painter_matrix;
  cairo_matrix_init(&painter_matrix,
      ex_pixels.x() - origin_pixels.x(), ex_pixels.y() - origin_pixels.y(),
      ey_pixels.x() - origin_pixels.x(), ey_pixels.y() - origin_pixels.y(),
      origin_pixels.x(), origin_pixels.y());

  auto object = std::make_shared<cairowindow::LayerRegion>([rectangle_painter, rectangle_style, painter_matrix](cairo_t* cr) -> cairowindow::StrokeExtents
  {
#ifdef CWDEBUG
    using namespace debugcairo;
#endif
    // Construct the rectangle path in painter coordinates.
    cairo_save(cr);
    cairo_transform(cr, &painter_matrix);
    cairo_rectangle(cr, rectangle_painter.offset_x(), rectangle_painter.offset_y(), rectangle_painter.width(), rectangle_painter.height());
    cairo_restore(cr);

    cairo_set_source_rgba(cr, rectangle_style.line_color().red(), rectangle_style.line_color().green(),
        rectangle_style.line_color().blue(), rectangle_style.line_color().alpha());
    cairo_set_line_width(cr, rectangle_style.line_width());
    if (!rectangle_style.dashes().empty())
      cairo_set_dash(cr, rectangle_style.dashes().data(), rectangle_style.dashes().size(), rectangle_style.dashes_offset());

    double x1, y1, x2, y2;
    cairo_stroke_extents(cr, &x1, &y1, &x2, &y2);
    cairo_stroke(cr);
    return {x1, y1, x2, y2};
  });

  layer->draw(object);
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

    cairowindow::cs::Size<CS::pixels> const ObjectSize_pixels{object_width, object_height};
    Dout(dc::notice, "ObjectSize_pixels = " << ObjectSize_pixels);

    cairowindow::cs::Size<CS::centered> const ObjectSize_centered = ObjectSize_pixels * centered_transform_pixels.inverse();
    Dout(dc::notice, "ObjectSize_centered = " << ObjectSize_centered);

    for (double a = 0.0; a < 360.0; a += 15.0)
    {
      auto const painter_transform_centered = Transform<CS::painter, CS::centered>{}.translate(-0.5 * TranslationVector{ObjectSize_centered}).rotate(a);
      Dout(dc::notice, "painter_transform_centered = " << painter_transform_centered);

      cairowindow::cs::Point<CS::painter> PainterOrigin_painter;
      Dout(dc::notice, "PainterOrigin_painter = " << PainterOrigin_painter);

      cairowindow::cs::Point<CS::centered> PainterOrigin_centered = PainterOrigin_painter * painter_transform_centered;
      Dout(dc::notice, "PainterOrigin_centered = " << PainterOrigin_centered);

      auto const painter_transform_pixels = painter_transform_centered * centered_transform_pixels;
      Dout(dc::notice, "painter_transform_pixels = " << painter_transform_pixels);

      cairowindow::cs::Size<CS::painter> const ObjectSize_painter = ObjectSize_pixels * painter_transform_pixels.inverse();
      Dout(dc::notice, "ObjectSize_painter = " << ObjectSize_painter);

      // Display the centered-coordinate-system.
      cairowindow::CoordinateSystem<CS::centered> centered_coordinate_system(centered_transform_pixels);
      centered_coordinate_system.display(layer, LineStyle({.line_color = color::green, .line_width = 1.0}));

      // Display the painter-coordinate-system.
      cairowindow::CoordinateSystem<CS::painter> painter_coordinate_system(painter_transform_pixels);
      painter_coordinate_system.display(layer, LineStyle({.line_color = color::red, .line_width = 1.0}));

      // Draw a test point at (1, -0.5) in both coordinate systems.
      cairowindow::draw::PointStyle const point_style({.color_index = 0, .filled_shape = 3});   // Filled ellipse.
      cairowindow::plot::cs::Point<CS::centered> const centered_point(1.0, -0.5);
      cairowindow::plot::cs::Point<CS::painter> const painter_point(1.0, -0.5);
      centered_coordinate_system.add_point(layer, point_style, centered_point);
      painter_coordinate_system.add_point(layer, point_style, painter_point);
      // Draw a rectangle in painter with one corner in the origin and the other at ObjectSize_painter.
      cairowindow::plot::cs::Rectangle<CS::painter> const rectangle_painter(PainterOrigin_painter, ObjectSize_painter);
      painter_coordinate_system.add_rectangle(layer, RectangleStyle({.line_color = color::black}), rectangle_painter);

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
