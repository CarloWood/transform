#include "sys.h"
// This file is intentionally not wired into `src/CMakeLists.txt` yet.
//
// It is meant as a compile-time exercise of `math::Transform` when instantiated
// with the Qt backend `math::QTransformAffineTransform<QTransform>` instead of
// the default `math::AffineTransform2D`.
//
// Note: do not be surprised if this fails to compile at the moment; it is
// expected to help reveal template/CS API mismatches.

#include <QTransform>
#include "math/QTransformAffineTransform.h"
#include "math/Transform.h"
#include "math/TranslationVector.h"
#include "math/cs/CS.h"
#include "math/cs/Direction.h"
#include "math/cs/Point.h"
#include "math/cs/Size.h"
#include "math/cs/TransformOperators.h"
#include "math/cs/Vector.h"
#include <cmath>
#include <utility>

#include "debug.h"

namespace csid {
using namespace math::csid;

DECLARE_CSID(from);
DECLARE_CSID(mid);
DECLARE_CSID(to);
} // namespace csid

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  using QtBackend = math::QTransformAffineTransform<QTransform>;

  auto& From = csid::from;
  auto& Mid = csid::mid;
  auto& To = csid::to;

  using T_from_mid = math::Transform<From, Mid, false, QtBackend>;
  using T_mid_to = math::Transform<Mid, To, false, QtBackend>;
  using T_from_to = math::Transform<From, To, false, QtBackend>;

  // Construct (identity) transforms.
  T_from_mid f_m;
  T_mid_to m_t;
  T_from_to f_t;

  // translate
  f_m.translate(math::TranslationVector<Mid>{math::cs::Point<Mid>{10.0, -20.0}});

  // scale
  f_m.scale(2.0, 3.0);
  f_m.scale(0.5);

  // rotate (radians)
  f_m.rotate(0.25 * std::acos(-1.0));

  // map_point overloads
  [[maybe_unused]] auto mapped_xy = f_m.map_point(1.0, 2.0);
  math::cs::Point<From> const p_from{3.0, 4.0};
  [[maybe_unused]] math::cs::Point<Mid> const p_mid = f_m.map_point(p_from);
  [[maybe_unused]] math::cs::Point<Mid> const p_mid2 = p_from * f_m;

  // map_vector overloads
  [[maybe_unused]] auto mapped_vec_xy = f_m.map_vector(1.0, 0.0);
  math::cs::Vector<From> const v_from{5.0, -6.0};
  [[maybe_unused]] math::cs::Vector<Mid> const v_mid = f_m.map_vector(v_from);
  [[maybe_unused]] math::cs::Vector<Mid> const v_mid2 = v_from * f_m;

  // map_direction
  math::cs::Direction<From> const d_from{0.1};
  [[maybe_unused]] math::cs::Direction<Mid> const d_mid = f_m.map_direction(d_from);
  [[maybe_unused]] math::cs::Direction<Mid> const d_mid2 = d_from * f_m;

  // map_size
  math::cs::Size<From> const s_from{7.0, 8.0};
  [[maybe_unused]] math::cs::Size<Mid> const s_mid = f_m.map_size(s_from);
  [[maybe_unused]] math::cs::Size<Mid> const s_mid2 = s_from * f_m;

  // inverted (returns a non-inverted Transform in the opposite direction).
  [[maybe_unused]] auto const m_f = f_m.inverted();               // Mid -> From
  [[maybe_unused]] math::cs::Point<From> const p_from2 = p_mid * m_f;

  // translation / scale helpers
  [[maybe_unused]] auto const tv_mid = f_m.translation();
  [[maybe_unused]] auto const xs = f_m.x_scale();
  [[maybe_unused]] auto const ys = f_m.y_scale();
  [[maybe_unused]] auto const scale_factors = f_m.scale_factors();

  // Axis directions.
  [[maybe_unused]] auto const x_dir = f_m.x_axis_direction();
  [[maybe_unused]] auto const y_dir = f_m.y_axis_direction();

  // inverse() is a view with inverted_ = true (not directly mappable with map_point/map_vector).
  [[maybe_unused]] auto const& inverse_view = f_m.inverse();
  (void)inverse_view;

  // operator* cases (see math/Transform.h for the four combinations).
  //
  // Case 1: non-inverted * non-inverted
  auto composed_1 = f_m * m_t;                                     // From -> To
  (void)composed_1;

  // For the remaining cases, we create inverted views by calling inverse() on non-inverted transforms.
  math::Transform<Mid, From, false, QtBackend> m_f_noninv;
  math::Transform<To, Mid, false, QtBackend> t_m_noninv;
  auto const& f_m_inv = m_f_noninv.inverse();                      // From -> Mid, inverted
  auto const& m_t_inv = t_m_noninv.inverse();                      // Mid -> To, inverted

  // Case 2: inverted * inverted
  auto composed_2 = f_m_inv * m_t_inv;                             // From -> To, inverted
  (void)composed_2;

  // Case 3: inverted * non-inverted
  auto composed_3 = f_m_inv * m_t;                                 // From -> To, non-inverted
  (void)composed_3;

  // Case 4: non-inverted * inverted
  auto composed_4 = f_m * m_t_inv;                                 // From -> To, non-inverted
  (void)composed_4;
}
