#include "sys.h"
#include "math/Hyperblock.h"
#include "math/Universe.h"
#include "math/Line.h"
#include "cairowindow/intersection_points.h"
#include "cairowindow/Window.h"
#include "cairowindow/Layer.h"
#include "cairowindow/Plot.h"
#include "utils/print_using.h"
#include "utils/AIAlert.h"
#include "utils/Array.h"
#include <string>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>
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
  window->register_draggable(fake_plot, draw_slider.get(), [draw_slider](plot::Point const& point) -> plot::Point { return point; });
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

// A (2D) windowplane point / depth pair.
struct PointDepth
{
  math::Point<2> point_wc;        // Some projection of a point in the hyperplane onto the windowplane.
  double depth;                   // The signed distance of the hyperplane point to the windowplane; this is used for depth ordering.
};

struct LineSegment
{
  using EdgeIndex = math::EdgeIndex<4>;
  using FaceIndex = math::kFaceIndex<4, 2>;

  std::array<PointDepth, 2> pd;   // A line segment, with pd[0].depth <= pd[1].depth.
  bool is_edge;                   // True if this is a tesseract edge, false if this is 2-face segment.
  union Info
  {
    EdgeIndex ei;
    FaceIndex fi;

    Info(EdgeIndex ei0) : ei(ei0) { }
    Info(FaceIndex fi0) : fi(fi0) { }
  } idx;

  template<typename Index>
  requires (std::same_as<Index, EdgeIndex> || std::same_as<Index, FaceIndex>)
  LineSegment(Index index, PointDepth const& pd0, PointDepth pd1) : pd({pd0, pd1}), is_edge(std::same_as<Index, EdgeIndex>), idx(index)
  {
    if (pd[1].depth < pd[0].depth)
      std::swap(pd[0], pd[1]);
  }
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
    draw::TextStyle label_style({.position = draw::centered_right_of, .font_size = 15.0, .color = color::lightgray, .offset = 3});
    draw::TextStyle depth_style({.position = draw::centered_above, .font_size = 12.0, .offset = 1});
    draw::PointStyle segment_point_style({.color_index = 2, .filled_shape = 3});
    draw::LineStyle segment_line_style({.line_color = color::darkmagenta, .line_width = 2.0});

    Sliders sliders(second_layer, window.geometry());

    // A given orientation (copied from debug output).
    double alpha = 2.2583, beta = 0.619212, gamma = 0.400667, offset = -0.24058, theta = 1.51161, phi = 0.327818;
    // Latest assert:
//    double alpha = 2.03065, beta = 0.619212, gamma = 1.51161, offset = -0.37971, theta = 1.41144, phi = 2.53149;

    sliders.first_row("Tesseract pos. rel. to hyperplane");
    auto alpha_slider  = sliders.add("alpha", alpha, 0.0, M_PI);
    auto beta_slider   = sliders.add("beta", beta, 0.0, M_PI);
    auto gamma_slider  = sliders.add("gamma", gamma, 0.0, 2.0 * M_PI);
    auto offset_value_slider = sliders.add("offset", offset, -1.0, 1.0);

    sliders.next_row("Hyperplane pos. rel. to screen");
    auto theta_slider  = sliders.add("theta", theta, 0.0, M_PI);        // Angle relative to positive z-axis.
    auto phi_slider    = sliders.add("phi", phi, 0.0, 2.0 * M_PI);      // Angle relative to positive x-axis.

    using tesseract_type = math::Hyperblock<4>;
    tesseract_type const tesseract({0, 0, 0, 0}, {1, 1, 1, 1});
    math::Vector<4> const center(0.5, 0.5, 0.5, 0.5);                   // The center of the tesseract.

    for (;;)
    {
      double const alpha = alpha_slider();
      double const beta = beta_slider();
      double const gamma = gamma_slider();
      double const offset_value = offset_value_slider();
      double const theta = theta_slider();
      double const phi = phi_slider();
      // Print current slider values, so we can reproduce a problem if one occurs.
      Dout(dc::notice, "alpha = " << alpha << ", beta = " << beta << ", gamma = " << gamma << ", offset = " << offset_value << ", theta = " << theta << ", phi = " << phi);

#if 0
      constexpr double step = 0.5;
      for (double alpha = 0.5; alpha < M_PI; alpha += step)
      {
        Dout(dc::notice, "alpha = " << alpha);
        for (double beta = 0.5; beta < M_PI; beta += step)
        {
          Dout(dc::notice, "beta = " << beta);
          for (double gamma = 1.5; gamma < 2.0 * M_PI; gamma += step)
            for (double theta = 1.0; theta < M_PI; theta += step)
              for (double phi = 0.5; phi < 2.0 * M_PI; phi += step)
              {
#endif

      // Suppress immediate updating of the window for each created item, in order to avoid flickering.
      window.set_send_expose_events(false);

      // Coordinate systems:
      // _uc : 4D Universe Coordinates.
      // _hc : 3D Hyperplane Coordinates.
      // _wc : 2D Windowplane Coordinates.

      // Convert slider values to `hyperplane_normal_uc` and `offset_uc`.
      math::Vector<4> hyperplane_normal_uc{
        std::cos(alpha),
        std::sin(alpha) * std::cos(beta),
        std::sin(alpha) * std::sin(beta) * std::cos(gamma),
        std::sin(alpha) * std::sin(beta) * std::sin(gamma)
      };
      Dout(dc::notice, "hyperplane_normal_uc = " << hyperplane_normal_uc);
      math::Vector<4> offset_uc = offset_value * hyperplane_normal_uc;
      // Create the `hyperplane_uc` itself.
      math::Hyperplane hyperplane_uc(hyperplane_normal_uc, -((center + offset_uc).dot(hyperplane_normal_uc)));  // center + offset_uc is a point in the hyperplane.

      // Define the universe: the tesseract lives in 4D space.
      using U = math::Universe<Id, 4>;

      // Construct a (3D) orthonormal basis for the hyperplane.
      math::SubSpace<U, 1> hyperplane_orthogonal_subspace(hyperplane_normal_uc);
      math::Basis<U, 3> hyperplane_basis(hyperplane_orthogonal_subspace);
      Dout(dc::notice, "hyperplane_basis = " << hyperplane_basis);
      // Conversion matrices.
      auto const hc_to_uc = hyperplane_basis.to_universe_coordinates();
      auto const uc_to_hc = hyperplane_basis.to_basis_coordinates();

      // Convert slider values to `windowplane_normal_hc`.
      math::Vector<3> windowplane_normal_hc{
        std::sin(theta) * std::cos(phi),
        std::sin(theta) * std::sin(phi),
        std::cos(theta)
      };
      Dout(dc::notice, "windowplane_normal_hc = " << windowplane_normal_hc);
      // Create the `windowplane` itself.
      math::Hyperplane windowplane(windowplane_normal_hc, 0);

      // Define a path delta direction that works at the very least near the pole θ=π.
      math::Vector<3> path_delta_hc(-std::cos(phi), -std::sin(phi), 0);      // Use ∂/∂θ (windowplane_normal_hc) at θ=π.
      // Construct a (2D) orthonormal basis for the windowplane.
      math::SubSpace<U, 2> windowplane_orthogonal_subspace(windowplane_normal_hc * hc_to_uc, hyperplane_normal_uc);
      math::Basis<U, 2> windowplane_basis(windowplane_orthogonal_subspace, 0.0025, {path_delta_hc * hc_to_uc});
      Dout(dc::notice, "windowplane_basis = " << windowplane_basis);
      // Conversion matrices.
      auto const wc_to_uc = windowplane_basis.to_universe_coordinates();
      auto const uc_to_wc = windowplane_basis.to_basis_coordinates();

      using CornerIndex = math::CornerIndex<4>;         // ci ; there are CornerIndex::size = (n choose 0) * 2^n     = 1 * 16 = 16 corners.
      using EdgeIndex = math::EdgeIndex<4>;             // ei ; there are EdgeIndex::size   = (n choose 1) * 2^(n-1) = 4 *  8 = 32 edges.
      using FaceIndex = math::kFaceIndex<4, 2>;         // fi ; there are FaceIndex::size   = (n choose 2) * 2^(n-2) = 6 *  4 = 24 2-faces.

      auto const hc_to_wc = hc_to_uc * uc_to_wc;

      // Lambda to project a point in Universe Coordinates to Window Coordinates and extract
      // the signed distance between the projection on the hyperplane and the windowplane.
      auto project_uc_to_wc = [&](math::Vector<4> const& point_uc) -> PointDepth
      {
        math::Vector<3> point_hc = hyperplane_uc.project(point_uc - center) * uc_to_hc;
        return {(windowplane.project(point_hc) * hc_to_wc).as_point(), windowplane.signed_distance(point_hc)};
      };

      // Create a mapping from corner index (ci) to the projected corner in windowplane coordinates (wc) and the depth of that corner.
      utils::Array<PointDepth, CornerIndex::size, CornerIndex> corners_pd;
      for (CornerIndex ci = tesseract.ibegin(); ci != tesseract.iend(); ++ci)
        corners_pd[ci] = project_uc_to_wc(tesseract[ci]);

      // Get all intersection points, between tesseract edges and the hyperplane, in Universe Coordinates (uc).
      using intersection_points_type = math::Hyperblock<4>::IntersectionPoints; // A Vector of IntersectionPoint<n, T> point/index pairs.
      intersection_points_type const hyperplane_intersections_uc = tesseract.intersection_points(hyperplane_uc);
      size_t const number_of_intersection_points = hyperplane_intersections_uc.size();
      // The index type used for intersection points.
      using IntersectionPointIndex = intersection_points_type::index_type;      // ii ; there are between 4 and 12 intersection points depending on the orientation of the hyperplane.
      Dout(dc::notice, "There are " << number_of_intersection_points << " intersection points.");

      // Calculate the depth and projections of the intersection points onto the windowplane.
      utils::Vector<PointDepth, IntersectionPointIndex> intersections(number_of_intersection_points);
      for (IntersectionPointIndex ii = intersections.ibegin(); ii != intersections.iend(); ++ii)
        intersections[ii] = project_uc_to_wc(hyperplane_intersections_uc[ii]);

      auto edge_axis = [](EdgeIndex edge) -> int {
        math::kFace<4, 1> kedge = edge.as_kface();
        auto axis_bit = *kedge.k_axes.begin();
        return axis_bit();
      };

      Dout(dc::notice|continued_cf, "projected corners = {");
      char const* sep = "";
      for (CornerIndex ci = tesseract.ibegin(); ci != tesseract.iend(); ++ci)
      {
        Dout(dc::continued, sep << corners_pd[ci].point_wc << "@" << corners_pd[ci].depth);
        sep = ", ";
      }
      Dout(dc::finish, "}");

#if 0
      // Commented out because it doesn't make the drawing more clear.

      // Draw all tesseract corners.
      std::vector<std::shared_ptr<draw::Point>> draw_corners;
      std::vector<std::shared_ptr<draw::Text>> draw_corner_depths;
      for (CornerIndex ci = tesseract.ibegin(); ci != tesseract.iend(); ++ci)
      {
        cairowindow::Point const corner{corners_pd[ci].point_wc.as_point() + window_center};
        draw_corners.push_back(std::make_shared<draw::Point>(corner, point_style));
        second_layer->draw(draw_corners.back());
        // Show their depth.
        std::string depth_text = std::to_string(corners_pd[ci].depth);
        draw_corner_depths.push_back(std::make_shared<draw::Text>(depth_text, corner.x(), corner.y(), depth_style));
        second_layer->draw(draw_corner_depths.back());
      }
#endif

      // Map edges to intersection points, if they have any.
      utils::Array<IntersectionPointIndex, EdgeIndex::size, EdgeIndex> ei_to_ii;
      for (EdgeIndex ei = ei_to_ii.ibegin(); ei != ei_to_ii.iend(); ++ei)
        for (IntersectionPointIndex ii = hyperplane_intersections_uc.ibegin(); ii != hyperplane_intersections_uc.iend(); ++ii)
          if (hyperplane_intersections_uc[ii].edge_index() == ei)
          {
            ei_to_ii[ei] = ii;
            break;
          }

      using LineSegmentIndex = utils::VectorIndex<LineSegment>;
      utils::Vector<LineSegment, LineSegmentIndex> line_segments_pd;

      // For each edge of the tesseract, find the two corners add store the edge as a LineSegment.
      for (EdgeIndex ei = ei_to_ii.ibegin(); ei != ei_to_ii.iend(); ++ei)
      {
        std::array<CornerIndex, 2> edge_ci;
        int count = 0;
        for (CornerIndex ci : ei.facet_indexes())
          edge_ci[count++] = ci;
        line_segments_pd.emplace_back(ei, corners_pd[edge_ci[0]], corners_pd[edge_ci[1]]);
      }

      // For each 2-face of the tesseract, find the two intersection points (if any) and store the segment as a LineSegment.
      for (FaceIndex fi = FaceIndex::ibegin(); fi != FaceIndex::iend(); ++fi)
      {
        std::array<IntersectionPointIndex, 2> face_ii;
        int count = 0;
        for (EdgeIndex ei : fi.facet_indexes())
        {
          IntersectionPointIndex ii = ei_to_ii[ei];
          if (!ii.undefined())
            face_ii[count++] = ii;
        }
        ASSERT(count == 0 || count == 2);
        if (count == 2)
          line_segments_pd.emplace_back(fi, intersections[face_ii[0]], intersections[face_ii[1]]);
      }

      // Build a depth-ordering graph over all line segments (edges and 2-face segments).
      // This graph is supposed to be a Directed A-cyclic Grap (DAG) with LineSegment's as nodes and edges
      // that represent back->front relationships (back having a larger depth).
      utils::Vector<std::vector<LineSegmentIndex>, LineSegmentIndex> graph(line_segments_pd.size());    // The edges that point out of node `LineSegmentIndex`.

      // Keep a record of the number of incoming edges for each node.
      utils::Vector<int, LineSegmentIndex> indegree(line_segments_pd.size(), 0);                        // The number of edges that point into node `LineSegmentIndex`.
      auto add_relation = [&](LineSegmentIndex back, LineSegmentIndex front)
      {
        graph[back].push_back(front);
        ++indegree[front];
      };

      constexpr double parallel_epsilon = 1e-12;
      constexpr double intersection_epsilon = 1e-9;
      for (LineSegmentIndex i0 = line_segments_pd.ibegin(); i0 != line_segments_pd.iend(); ++i0)
      {
        LineSegment const& segment0 = line_segments_pd[i0];
        math::Point<2> const& p00 = segment0.pd[0].point_wc;    // The point of segment0 (in window coordinates) with the smallest depth,
        math::Point<2> const& p01 = segment0.pd[1].point_wc;    // and the largest depth.
        math::Vector<2> const p00_p01(p00, p01);                // The difference p01 - p00.
        math::Line<2> const line0(p00, p00_p01.direction());    // Line equation for the line through the segment.

        for (LineSegmentIndex i1 = i0 + 1; i1 != line_segments_pd.iend(); ++i1)
        {
          LineSegment const& segment1 = line_segments_pd[i1];
          math::Point<2> const& p10 = segment1.pd[0].point_wc;  // The point of segment1 (in window coordinates) with the smallest depth,
          math::Point<2> const& p11 = segment1.pd[1].point_wc;  // and the largest depth.
          math::Vector<2> const p10_p11(p10, p11);              // The difference p11 - p10.
          math::Line<2> const line1(p10, p10_p11.direction());  // Line equation for the line through the segment.

          // Skip parallel segments (in windowplane).
          if (std::abs(line0.direction().dot(line1.direction().normal())) < parallel_epsilon)
            continue;

          // The point where the two line segments cross each other in the window plane.
          math::Point<2> const pc = line0.intersection_with(line1);

          math::Vector<2> const p00_pc(p00, pc);                // The difference pc - p00.
          math::Vector<2> const p10_pc(p10, pc);                // The difference pc - p10.

          double const lambda0 = p00_pc.dot(p00_p01) / p00_p01.norm_squared();
          double const lambda1 = p10_pc.dot(p10_p11) / p10_p11.norm_squared();

          if (lambda0 <= intersection_epsilon || lambda0 >= 1.0 - intersection_epsilon ||
              lambda1 <= intersection_epsilon || lambda1 >= 1.0 - intersection_epsilon)
            continue;   // Intersection lies outside the line segment (or too close to an endpoint).

          // Approximate depth at the intersection point by linear interpolation of endpoint depths.
          double const depth0 = (1.0 - lambda0) * segment0.pd[0].depth + lambda0 * segment0.pd[1].depth;
          double const depth1 = (1.0 - lambda1) * segment1.pd[0].depth + lambda1 * segment1.pd[1].depth;

          if (std::abs(depth0 - depth1) < parallel_epsilon)
          {
            // Effectively the same depth; we could use the signed distance of the project
            // from 4D to the hyperplane, but this never happens anyway in reality.
            continue;
          }

          if (depth0 > depth1)
            add_relation(i0, i1);     // segment0 is behind segment1.
          else
            add_relation(i1, i0);     // segment1 is behind segment0.
        }
      }

      // Topologically sort all line segments (edges and 2-face segments) according to the
      // back->front relations stored in `graph`. The result is a draw order in which
      // every segment is drawn before all segments that are in front of it at some crossing.
      std::vector<LineSegmentIndex> draw_order;
      draw_order.reserve(line_segments_pd.size());

      // Start with all segments that have no segments behind them (indegree 0).
      std::queue<LineSegmentIndex> ready;
      for (LineSegmentIndex li = indegree.ibegin(); li != indegree.iend(); ++li)
        if (indegree[li] == 0)
          ready.push(li);

      // Kahn's algorithm for topological sorting.
      while (!ready.empty())
      {
        LineSegmentIndex const back = ready.front();
        ready.pop();
        draw_order.push_back(back);

        for (LineSegmentIndex const front : graph[back])
        {
          ASSERT(indegree[front] > 0);
          if (--indegree[front] == 0)
            ready.push(front);
        }
      }

      // If this fails then there is a cycle in the depth-ordering graph.
      ASSERT(draw_order.size() == line_segments_pd.size());

      std::vector<std::shared_ptr<draw::Line>> draw_edges;
      draw_edges.reserve(2 * EdgeIndex::size);
      std::vector<std::shared_ptr<draw::Text>> draw_edge_labels;
      draw_edge_labels.reserve(EdgeIndex::size);
      std::vector<std::shared_ptr<draw::Line>> draw_segments;
      draw_segments.reserve(2 * (line_segments_pd.size() - EdgeIndex::size));
      std::vector<std::shared_ptr<draw::Point>> draw_intersection_points;
      draw_intersection_points.reserve(number_of_intersection_points);

      // Draw all line segments in the topologically sorted order.
      for (LineSegmentIndex li : draw_order)
      {
        // Calculate cairowindow points in pixels for both endpoints.
        std::array<cairowindow::cs::Point<CS::pixels>, 2> endpoint_px;
        for (int i = 0; i < 2; ++i)
          endpoint_px[i] = cairowindow::cs::Point<CS::pixels>{line_segments_pd[li].pd[i].point_wc + window_center};

        if (line_segments_pd[li].is_edge)
        {
          static std::array<cairowindow::Color, 4> const axis_color = { color::red, color::green, color::blue, color::cyan };

          // Get the axis that this edge is parallel with.
          EdgeIndex const ei = line_segments_pd[li].idx.ei;
          int const axis_number = ei.as_kface().k_axes.lssbi()();

          // Draw edge.
          draw_edges.push_back(std::make_shared<draw::Line>(endpoint_px[0], endpoint_px[1], line_style({.line_color = color::white, .line_width = 3.0})));
          second_layer->draw(draw_edges.back());
          draw_edges.push_back(std::make_shared<draw::Line>(endpoint_px[0], endpoint_px[1], line_style({.line_color = axis_color[axis_number]})));
          second_layer->draw(draw_edges.back());

#if 0
          // Draw edge index as a label.
          double const mid_x_px = 0.5 * (endpoint_px[0].x() + endpoint_px[1].x());
          double const mid_y_px = 0.5 * (endpoint_px[0].y() + endpoint_px[1].y());
          std::string text = std::to_string(ei.get_value());
          draw_edge_labels.push_back(std::make_shared<draw::Text>(text, mid_x_px, mid_y_px, label_style));
          second_layer->draw(draw_edge_labels.back());
#endif
        }
        else
        {
          // Draw 2-face line segment.
          draw_segments.push_back(std::make_shared<draw::Line>(endpoint_px[0], endpoint_px[1], segment_line_style({.line_color = color::white, .line_width = 4.0})));
          second_layer->draw(draw_segments.back());
          draw_segments.push_back(std::make_shared<draw::Line>(endpoint_px[0], endpoint_px[1], segment_line_style));
          second_layer->draw(draw_segments.back());

          // Draw intersection points.
          for (int i = 0; i <= 1; ++i)
          {
            draw_intersection_points.push_back(std::make_shared<draw::Point>(endpoint_px[i], segment_point_style));
            second_layer->draw(draw_intersection_points.back());
          }
        }
      }

      // Flush all expose events related to the drawing done above.
      window.set_send_expose_events(true);

#if 0
              } // next angle
        }
      }
#endif

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
