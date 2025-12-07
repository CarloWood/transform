#include "sys.h"
#include "math/Hyperblock.h"
#include "math/Universe.h"
#include "cairowindow/intersection_points.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/Plot.h"
#include "utils/print_using.h"
#include "utils/AIAlert.h"
#include "utils/Array.h"
#include <cmath>
#include <thread>
#include "debug.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

using namespace cairowindow;

//--------------------------------------------------------------------------
// Text

[[nodiscard]] std::shared_ptr<draw::Text> create_text(boost::intrusive_ptr<Layer> const& layer,
    draw::TextStyle const& text_style, Pixel position, std::string const& text)
{
  auto draw_object = std::make_shared<draw::Text>(text, position.x(), position.y(), text_style);
  layer->draw(draw_object);
  return draw_object;
}

//--------------------------------------------------------------------------
// Slider

#if 0
std::shared_ptr<draw::Slider> create_slider(plot::Plot& fake_plot, boost::intrusive_ptr<Layer> const& layer,
    Rectangle const& geometry, double start_value, double min_value, double max_value)
{
//  Slider plot_slider(geometry, min_value, max_value);
  auto draw_slider = std::make_shared<draw::Slider>(geometry.offset_x(), geometry.offset_y(), geometry.width(), geometry.height(), start_value, min_value, max_value);
  static_cast<draw::MultiRegion&>(*draw_slider).draw_regions_on(layer.get());
  Window* window = layer->window();
  window->register_draggable(fake_plot, draw_slider.get(), [draw_slider](Point const& point) -> Point { return point; });
  return draw_slider;
}
#endif

// Where to put the center of the tesseract.
constexpr int window_center_x = 450;
constexpr int window_center_y = 450;

struct Sliders
{
  static constexpr double sliders_pos_x_start = 978;
  static constexpr double sliders_pos_y_start = 50;
  static constexpr double sliders_height = 375;
  static constexpr double sliders_width = 7;
  static constexpr double sliders_offset_x = 50;
  static constexpr double sliders_offset_y = sliders_height + 70;
  static constexpr int number_of_tesseract_sliders = 4;
  static constexpr double sliders_center_x = sliders_pos_x_start + 0.5 * (number_of_tesseract_sliders - 1) * sliders_offset_x;

  draw::TextStyle const title_style_;
  draw::TextStyle const slider_style_;

  plot::Plot fake_plot_;
  boost::intrusive_ptr<Layer> layer_;
  double sliders_pos_x_;
  double sliders_pos_y_;
  std::vector<std::shared_ptr<draw::Text>> texts_;
  std::vector<plot::Slider> sliders_;

  Sliders(boost::intrusive_ptr<Layer> const& layer, Rectangle const& geometry) :
    title_style_({.position = draw::centered_above, .font_size = 12.0, .offset = 30}),
    slider_style_({.position = draw::centered_below_no_bearing, .font_size = 12.0, .offset = 10}),
    fake_plot_(geometry, {}), layer_(layer), sliders_pos_x_(sliders_pos_x_start), sliders_pos_y_(sliders_pos_y_start) { }

  void first_row(std::string title);
  void next_row(std::string title);
  auto add(std::string label, double start_value, double min_value, double max_value);
};

void Sliders::first_row(std::string title)
{
  texts_.push_back(create_text(layer_, title_style_, Pixel{sliders_center_x, sliders_pos_y_}, title));
}

void Sliders::next_row(std::string title)
{
  sliders_pos_x_ = sliders_pos_x_start;
  sliders_pos_y_ = sliders_pos_y_start + sliders_offset_y;
  texts_.push_back(create_text(layer_, title_style_, Pixel{sliders_center_x, sliders_pos_y_}, title));
}

auto Sliders::add(std::string label, double start_value, double min_value, double max_value)
{
  int index = sliders_.size();
  sliders_.emplace_back(fake_plot_.create_slider(layer_, {sliders_pos_x_, sliders_pos_y_, sliders_width, sliders_height}, start_value, min_value, max_value));
  texts_.push_back(create_text(layer_, slider_style_, Pixel{sliders_pos_x_, sliders_pos_y_ + sliders_height}, label));
  sliders_pos_x_ += sliders_offset_x;
  return [this, index](){ return sliders_[index].value(); };
}

enum Id {
  universe_id
};

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  using namespace intersections;

  try
  {
    using namespace cairowindow;
    using Window = cairowindow::Window;

    // Create a window.
    Window window("hypercube", 1200, 900);

    // The center of the free space (where there aren't sliders).
    math::Vector<2> const window_center{window_center_x, window_center_y};

    // Create a new layer with a white background.
    auto background_layer = window.create_background_layer<Layer>(color::white COMMA_DEBUG_ONLY("background_layer"));

    // Create another layer.
    auto second_layer = window.create_layer<Layer>({} COMMA_DEBUG_ONLY("second_layer"));

    // Open the window and start drawing.
    std::thread event_loop([&](){
      // Open window, handle event loop. This must be constructed after the draw stuff, so that it is destructed first!
      // Upon destruction it blocks until the event loop thread finished (aka, the window was closed).
      EventLoop event_loop = window.run();
      event_loop.set_cleanly_terminated();
    });

    draw::LineStyle tesseract_line_style({.line_width = 1.0});
    draw::LineStyle corner_line_style({.line_color = color::turquoise, .line_width = 1.0});
    draw::PointStyle point_style({.color_index = 31, .filled_shape = 6 /*diamond*/});
    draw::LineStyle line_style({.line_color = color::blue, .line_width = 1.0});

    Sliders sliders(second_layer, window.geometry());

    sliders.first_row("Tesseract pos. rel. to hyperplane");
    auto alpha  = sliders.add("alpha", 1.0, 0.0, M_PI);
    auto beta   = sliders.add("beta", 1.0, 0.0, M_PI);
    auto gamma  = sliders.add("gamma", 1.0, 0.0, 2.0 * M_PI);
    auto offset_value = sliders.add("offset", 0.0, -1.0, 1.0);

    sliders.next_row("Hyperplane pos. rel. to screen");
    auto theta  = sliders.add("theta", 1.0, 0.0, M_PI);          // Angle relative to positive z-axis.
    auto phi    = sliders.add("phi", 1.0, 0.0, 2.0 * M_PI);      // Angle relative to positive x-axis.

    math::Vector<4> const center(0.5, 0.5, 0.5, 0.5);
    math::Hyperblock<4> const tesseract({0, 0, 0, 0}, {1, 1, 1, 1});

    for (;;)
    {
      // Suppress immediate updating of the window for each created item, in order to avoid flickering.
      window.set_send_expose_events(false);

      // Coordinate systems:
      // _uc : 4D Universe Coordinates.
      // _hc : 3D Hyperplane Coordinates.
      // _wc : 2D Windowplane Coordinates.

      math::Vector<4> hyperplane_normal_uc{
        std::cos(alpha()),
        std::sin(alpha()) * std::cos(beta()),
        std::sin(alpha()) * std::sin(beta()) * std::cos(gamma()),
        std::sin(alpha()) * std::sin(beta()) * std::sin(gamma())
      };
      math::Vector<4> offset_uc = offset_value() * hyperplane_normal_uc;
      math::Vector<4> P_uc = center + offset_uc;
      math::Hyperplane hyperplane_uc(hyperplane_normal_uc, -(P_uc.dot(hyperplane_normal_uc)));

      // Define the universe: the tesseract lives in 4D space.
      using U = math::Universe<Id, 4>;

      // Construct a 3D orthonormal basis for the hyperplane.
      math::SubSpace<U, 1> hyperplane_orthogonal_subspace(hyperplane_normal_uc);
      math::Basis<U, 3> hyperplane_basis(hyperplane_orthogonal_subspace);
      auto const hc_to_uc = hyperplane_basis.to_universe_coordinates();
      auto const uc_to_hc = hyperplane_basis.to_basis_coordinates();

      //Dout(dc::notice, "hyperplane_basis = " << hyperplane_basis);

      math::Vector<3> windowplane_normal_hc{
        std::sin(theta()) * std::cos(phi()),
        std::sin(theta()) * std::sin(phi()),
        std::cos(theta())
      };
      math::Hyperplane windowplane(windowplane_normal_hc, 0);

      // Construct a 2D orthonormal basis for the windowplane.
      math::SubSpace<U, 2> windowplane_orthogonal_subspace(hyperplane_normal_uc, windowplane_normal_hc * hc_to_uc);
      math::Basis<U, 2> windowplane_basis(windowplane_orthogonal_subspace, 0.0025);
      auto const wc_to_uc = windowplane_basis.to_universe_coordinates();
      auto const uc_to_wc = windowplane_basis.to_basis_coordinates();

      //Dout(dc::notice, "windowplane_basis = " << windowplane_basis);

      utils::Array<math::Vector<2>, 1 << 4, CornerIndex> projected_corners;
      auto const hc_to_wc = hc_to_uc * uc_to_wc;
      for (CornerIndex ci = tesseract.ibegin(); ci != tesseract.iend(); ++ci)
      {
        math::Vector<4> const& corner_uc = tesseract[ci] - center;
        math::Vector<3> hyperplane_projection_hc = hyperplane_uc.project(corner_uc) * uc_to_hc;
        math::Vector<2> windowplane_projection_wc = windowplane.project(hyperplane_projection_hc) * hc_to_wc;
        projected_corners[ci] = windowplane_projection_wc;
      }

      std::vector<std::shared_ptr<draw::Point>> draw_corners;
      std::vector<std::shared_ptr<draw::Line>> draw_edges;
      for (CornerIndex ci = tesseract.ibegin(); ci != tesseract.iend(); ++ci)
      {
        cairowindow::Point const corner{projected_corners[ci].as_point() + window_center};
        draw_corners.push_back(std::make_shared<draw::Point>(corner, point_style));
        //second_layer->draw(draw_corners.back());

        static std::array<cairowindow::Color, 4> const axis_color = { color::red, color::green, color::blue, color::cyan };
        for (int d = 0; d < 4; ++d)
        {
          size_t const bit = math::detail::to_mask(d);
          CornerIndex const adjacent_ci{ci.get_value() ^ bit};

          cairowindow::Point const adjacent_corner{projected_corners[adjacent_ci].as_point() + window_center};
          draw_edges.push_back(std::make_shared<draw::Line>(corner, adjacent_corner, line_style({.line_color = color::white, .line_width = 3.0})));
          second_layer->draw(draw_edges.back());
          draw_edges.push_back(std::make_shared<draw::Line>(corner, adjacent_corner, line_style({.line_color = axis_color[d]})));
          second_layer->draw(draw_edges.back());
        }
      }

      // Flush all expose events related to the drawing done above.
      window.set_send_expose_events(true);

      // Block until a key is pressed.
      if (!window.handle_input_events())
        break;          // Program must be terminated.
    }

    window.close();
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }
}
