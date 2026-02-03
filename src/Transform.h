#pragma once

#include "TranslationVector.h"
#include "cairowindow/cs/Rectangle.h"
#include <cmath>
#include <concepts>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <utility>
#ifdef CWDEBUG
#include "utils/has_print_on.h"
#endif

#ifdef CWDEBUG
// This class defines a print_on method.
using utils::has_print_on::operator<<;
#endif

//------------------------------------------------------------------------------
// Affine transforms
//
// Transform used to be implemented directly with Qt's QTransform. In order to
// keep cairowindow usable without Qt, Transform is now parameterized with an
// affine backend that supplies the required operations.
//
// The backend operates on doubles. Qt projects can provide an adapter type that
// wraps QTransform and satisfies AffineTransformConcept.
//
template<typename AffineTransform>
concept AffineTransformConcept =
  std::default_initializable<AffineTransform> &&
  std::copy_constructible<AffineTransform> &&
  requires(AffineTransform transform, AffineTransform const const_transform,
           double x, double y, double dx, double dy, double factor, double degrees)
  {
    { transform.m11() } -> std::same_as<double>;
    { transform.m21() } -> std::same_as<double>;
    { transform.m31() } -> std::same_as<double>;
    { transform.m12() } -> std::same_as<double>;
    { transform.m22() } -> std::same_as<double>;
    { transform.m32() } -> std::same_as<double>;
    { transform.translate(dx, dy) } -> std::same_as<AffineTransform&>;
    { transform.scale(factor) } -> std::same_as<AffineTransform&>;
    { transform.rotate(degrees) } -> std::same_as<AffineTransform&>;
    { const_transform.inverted() } -> std::same_as<AffineTransform>;
    { const_transform.map_point(x, y) } -> std::same_as<std::pair<double, double>>;
    { const_transform.map_vector(dx, dy) } -> std::same_as<std::pair<double, double>>;
    { const_transform * const_transform } -> std::same_as<AffineTransform>;
  };

// Standalone Affine backend.
//
// Matrix layout matches QTransform:
//
//   ⎡ m₁₁ m₁₂ 0 ⎤
//   ⎢ m₂₁ m₂₂ 0 ⎥
//   ⎣ m₃₁ m₃₂ 1 ⎦
//
// Mapping is done as a row-vector multiplication from the left:
//   [x y 1] * M
//
class AffineTransform2D
{
 private:
  // Identity.
  double m11_ = 1.0, m12_ = 0.0;
  double m21_ = 0.0, m22_ = 1.0;
  double m31_ = 0.0, m32_ = 0.0;

 private:
  static AffineTransform2D translation(double dx, double dy)
  {
    AffineTransform2D t;
    t.m31_ = dx;
    t.m32_ = dy;
    return t;
  }

  static AffineTransform2D scaling(double factor)
  {
    AffineTransform2D s;
    s.m11_ = factor;
    s.m22_ = factor;
    return s;
  }

  static AffineTransform2D rotation(double degrees)
  {
    AffineTransform2D r;
    static constexpr double pi = std::acos(-1.0);
    double const radians = degrees * pi / 180.0;
    double const c = std::cos(radians);
    double const s = std::sin(radians);
    r.m11_ = c;
    r.m12_ = s;
    r.m21_ = -s;
    r.m22_ = c;
    return r;
  }

 public:
  AffineTransform2D() = default;

  // Matrix element accessors.
  double m11() const { return m11_; }
  double m21() const { return m21_; }
  double m31() const { return m31_; }
  double m12() const { return m12_; }
  double m22() const { return m22_; }
  double m32() const { return m32_; }

  // Apply a translation *before* the current transform.
  //
  //   returns
  //
  //     ⎡  1  0  0 ⎤⎡ m₁₁ m₁₂ 0 ⎤   ⎡ m₁₁                  m₁₂                  0 ⎤
  //     ⎢  0  1  0 ⎥⎢ m₂₁ m₂₂ 0 ⎥ = ⎢ m₂₁                  m₂₂                  0 ⎥
  //     ⎣ dx dy  1 ⎦⎣ m₃₁ m₃₂ 1 ⎦   ⎣ m₁₁dx + m₂₁dy + m₃₁  m₁₂dx + m₂₂dy + m₃₂  1 ⎦
  //
  AffineTransform2D& translate(double dx, double dy)
  {
    *this = translation(dx, dy) * *this;
    return *this;
  }

  // Apply a scaling *before* the current transform.
  //
  // - scale(sx, sy)
  //
  //   returns
  //
  //     ⎡sx  0  0 ⎤⎡ m₁₁ m₁₂ 0 ⎤   ⎡ sx * m₁₁  sx * m₁₂  0 ⎤
  //     ⎢ 0 sy  0 ⎥⎢ m₂₁ m₂₂ 0 ⎥ = ⎢ sy * m₂₁  sy * m₂₂  0 ⎥
  //     ⎣ 0  0  1 ⎦⎣ m₃₁ m₃₂ 1 ⎦   ⎣      m₃₁       m₃₂  1 ⎦
  //
  AffineTransform2D& scale(double factor)
  {
    *this = scaling(factor) * *this;
    return *this;
  }

  // Apply a rotation *before* the current transform.
  //
  // - rotate(degrees)
  //
  //   returns
  //
  //     ⎡ cos(θ) sin(θ)  0 ⎤⎡ m₁₁ m₁₂ 0 ⎤   ⎡  cos(θ) * m₁₁ + sin(θ) * m₂₁   cos(θ) * m₁₂ + sin(θ) * m₂₂  0 ⎤
  //     ⎢-sin(θ) cos(θ)  0 ⎥⎢ m₂₁ m₂₂ 0 ⎥ = ⎢ -sin(θ) * m₁₁ + cos(θ) * m₂₁  -sin(θ) * m₁₂ + cos(θ) * m₂₂  0 ⎥
  //     ⎣ 0      0       1 ⎦⎣ m₃₁ m₃₂ 1 ⎦   ⎣  m₃₁                           m₃₂                          1 ⎦
  //
  AffineTransform2D& rotate(double degrees)
  {
    *this = rotation(degrees) * *this;
    return *this;
  }

  // - map(x, y)
  //
  //   returns
  //
  //           ⎡ m₁₁ m₁₂ 0 ⎤
  //    [x y 1]⎢ m₂₁ m₂₂ 0 ⎥ = [ x * m₁₁ + y * m₂₁ + m₃₁  x * m₁₂ + y * m₂₂ + m₃₂  1 ]
  //           ⎣ m₃₁ m₃₂ 1 ⎦
  //
  [[nodiscard]] std::pair<double, double> map_point(double x, double y) const
  {
    return {x * m11_ + y * m21_ + m31_, x * m12_ + y * m22_ + m32_};
  }

  // Same, without translation (assume m₃₁ = m₃₂ = 0).
  [[nodiscard]] std::pair<double, double> map_vector(double dx, double dy) const
  {
    return {dx * m11_ + dy * m21_, dx * m12_ + dy * m22_};
  }

  // - inverted()
  //
  //   returns
  //
  //            ⎡  m₂₂           -m₁₂            0   ⎤
  //    (1/det) ⎢ -m₂₁            m₁₁            0   ⎥
  //            ⎣  m₂₁m₃₂-m₂₂m₃₁  m₁₂m₃₁-m₁₁m₃₂  det ⎦
  //
  //   where det = m₁₁m₂₂-m₁₂m₂₁
  //
  [[nodiscard]] AffineTransform2D inverted() const
  {
    double const det = m11_ * m22_ - m12_ * m21_;
    // Don't scale with a factor of zero.
    ASSERT(det != 0.0);
    double const inv_det = 1.0 / det;

    AffineTransform2D inv;
    inv.m11_ =  m22_ * inv_det;
    inv.m12_ = -m12_ * inv_det;
    inv.m21_ = -m21_ * inv_det;
    inv.m22_ =  m11_ * inv_det;
    inv.m31_ = -(inv.m21_ * m32_ + inv.m11_ * m31_);    // (m₂₁m₃₂-m₂₂m₃₁)/det = -((-m₂₁/det) m₃₂ + (m₂₂/det) m₃₁)
    inv.m32_ = -(inv.m12_ * m31_ + inv.m22_ * m32_);    // (m₁₂m₃₁-m₁₁m₃₂)/det = -((-m₁₂/det) m₃₁ + (m₁₁/det) m₃₂)
    return inv;
  }

  //
  //     ⎡ l₁₁ l₁₂ 0 ⎤⎡ r₁₁ r₁₂ 0 ⎤   ⎡ l₁₁r₁₁+l₁₂r₂₁      l₁₁r₁₂+l₁₂r₂₂      0 ⎤
  //     ⎢ l₂₁ l₂₂ 0 ⎥⎢ r₂₁ r₂₂ 0 ⎥ = ⎢ l₂₁r₁₁+l₂₂r₂₁      l₂₁r₁₂+l₂₂r₂₂      0 ⎥
  //     ⎣ l₃₁ l₃₂ 1 ⎦⎣ r₃₁ r₃₂ 1 ⎦   ⎣ l₃₁r₁₁+l₃₂r₂₁+r₃₁  l₃₁r₁₂+l₃₂r₂₂+r₃₂  1 ⎦
  //
  friend AffineTransform2D operator*(AffineTransform2D const& lhs, AffineTransform2D const& rhs)
  {
    AffineTransform2D result;
    result.m11_ = lhs.m11_ * rhs.m11_ + lhs.m12_ * rhs.m21_;
    result.m12_ = lhs.m11_ * rhs.m12_ + lhs.m12_ * rhs.m22_;
    result.m21_ = lhs.m21_ * rhs.m11_ + lhs.m22_ * rhs.m21_;
    result.m22_ = lhs.m21_ * rhs.m12_ + lhs.m22_ * rhs.m22_;
    result.m31_ = lhs.m31_ * rhs.m11_ + lhs.m32_ * rhs.m21_ + rhs.m31_;
    result.m32_ = lhs.m31_ * rhs.m12_ + lhs.m32_ * rhs.m22_ + rhs.m32_;
    return result;
  }
};

// Verify that AffineTransform2D fullfills the concept.
static_assert(AffineTransformConcept<AffineTransform2D>);

template<CS from_cs, CS to_cs, bool inverted = false, AffineTransformConcept AffineTransformBackend = AffineTransform2D>
class Transform
{
 private:
  template<CS from_cs2, CS to_cs2, bool inverted2, AffineTransformConcept AffineTransformBackend2>
  friend class Transform;

  AffineTransformBackend m_;

 private:
  Transform(AffineTransformBackend const& m) : m_(m) { }

 public:
  Transform() = default;

  Transform& translate(TranslationVector<to_cs> const& tv);
  Transform& scale(double factor);
  Transform& rotate(double degrees);

  // The inverse converts from `to_cs` to `from_cs`!
  Transform<to_cs, from_cs, !inverted, AffineTransformBackend> const& inverse() const
  {
    return reinterpret_cast<Transform<to_cs, from_cs, !inverted, AffineTransformBackend> const&>(*this);
  }

  cairowindow::cs::Point<to_cs> multiply_from_the_right_with(cairowindow::cs::Point<from_cs> const& point) const;
  cairowindow::cs::Size<to_cs> multiply_from_the_right_with(cairowindow::cs::Size<from_cs> const& size) const;
  cairowindow::cs::Rectangle<to_cs> multiply_from_the_right_with(cairowindow::cs::Rectangle<from_cs> const& size) const;

  // Let A_M1_B be non-inverted and convert from A to B.
  // Let B_M2_C be non-inverted and convert from B to C.
  //
  // Multiplication between two non-inverted Transforms.
  // 1. A_M12_C = A_M1_B * B_M2_C
  //
  //
  // Let A_M1_B^-1 be an inverted matrix that converts from B to A, and therefore denote it as B_M1inv_A
  // Let B_M2_C^-1 be an inverted matrix that converts from C to B, and therefore denote it as C_M2inv_B
  //
  // Multiplication between two inverted Transforms.
  // 2. A_M34inv_C = A_M4inv_B * B_M3inv_C
  //
  // Note that C_M34_A = C_M3_B * B_M4_A
  //
  //
  // Let A_M5inv_B = B_M5_A^-1 be an inverted matrix that converts from A to B.
  // Let B_M56_C = B_M5_A * A_M6_C.
  //
  // Multiplication between an inverted Transform and a non-inverted Transform.
  // 3. A_M6_C = A_M5inv_B * B_M56_C
  //
  //
  // Let A_M78_B = A_M7_C * C_M8_B.
  //
  // Multiplication between a non-inverted Transform and an inverted Transform.
  // 4. A_M7_C = A_M78_B * B_M8inv_C
  //
  //
  // Then using specializations, where from_cs = A, to_cs = B and result_cs = C we'd have:
  //
  // Specialization for 1 (neither input inverted, result also not inverted).
  // std::enable_if_t<!inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, false> const& rhs) const;
  //
  // Specialization for 2 (both inputs inverted, output also inverted).
  // std::enable_if_t<inverted, Transform<from_cs, result_cs, true>> operator*(Transform<to_cs, result_cs, true> const& rhs) const;
  //
  // Specialization for 3 (only lhs input inverted, output not inverted).
  // std::enable_if_t<inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, false> const& rhs) const;
  //
  // Specialization for 4 (only rhs input inverted, output not inverted).
  //std::enable_if_t<!inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, true> const& rhs) const;
  //
  template<CS result_cs, bool rhs_inverted>
  Transform<from_cs, result_cs, inverted && rhs_inverted, AffineTransformBackend>
  operator*(Transform<to_cs, result_cs, rhs_inverted, AffineTransformBackend> const& rhs) const
  {
    // 1. Multiplication between two non-inverted Transforms.
    if constexpr (!inverted && !rhs_inverted)
    {
      return {m_ * rhs.m_};
    }
    // 2. Multiplication between two inverted Transforms.
    else if constexpr (inverted && rhs_inverted)
    {
      // A^-1 * B^-1 = (B * A)^-1
      return {rhs.m_ * m_};
    }
    // 3. Multiplication between an inverted Transform and a non-inverted Transform.
    else if constexpr (inverted && !rhs_inverted)
    {
      return {m_.inverted() * rhs.m_};
    }
    // 4. Multiplication between a non-inverted Transform and an inverted Transform.
    else if constexpr (!inverted && rhs_inverted)
    {
      return {m_ * rhs.m_.inverted()};
    }
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const
  {
    std::ostringstream prefix;
    prefix << utils::to_string(from_cs) << "_transform_" << utils::to_string(to_cs) << ":";
    int const prefix_len = std::max((int)prefix.str().length(), 24);

    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎛" << std::setw(9) << m_.m11() << " " << std::setw(9) << m_.m12() << " " << 0.0 << "⎞" << std::right;
    os << '\n' << std::right << std::setw(prefix_len) << prefix.str() <<
      std::left << "⎜" << std::setw(9) << m_.m21() << " " << std::setw(9) << m_.m22() << " " << 0.0 << "⎟" << std::right;
    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎝" << std::setw(9) << m_.m31()  << " " << std::setw(9) << m_.m32() << " " << 1.0 << "⎠" << std::right;
  }
#endif
};

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
Transform<from_cs, to_cs, inverted, AffineTransformBackend>&
Transform<from_cs, to_cs, inverted, AffineTransformBackend>::translate(TranslationVector<to_cs> const& tv)
{
  m_.translate(tv.x(), tv.y());
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
Transform<from_cs, to_cs, inverted, AffineTransformBackend>&
Transform<from_cs, to_cs, inverted, AffineTransformBackend>::scale(double factor)
{
  m_.scale(factor);
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
Transform<from_cs, to_cs, inverted, AffineTransformBackend>&
Transform<from_cs, to_cs, inverted, AffineTransformBackend>::rotate(double degrees)
{
  m_.rotate(degrees);
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
cairowindow::cs::Point<to_cs>
Transform<from_cs, to_cs, inverted, AffineTransformBackend>::multiply_from_the_right_with(cairowindow::cs::Point<from_cs> const& point) const
{
  if constexpr (!inverted)
  {
    auto const [x, y] = m_.map_point(point.x(), point.y());
    return {x, y};
  }
  else
  {
    auto const inv = m_.inverted();
    auto const [x, y] = inv.map_point(point.x(), point.y());
    return {x, y};
  }
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
cairowindow::cs::Point<to_cs>
operator*(cairowindow::cs::Point<from_cs> const& point, Transform<from_cs, to_cs, inverted, AffineTransformBackend> const& transform)
{
  return transform.multiply_from_the_right_with(point);
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
cairowindow::cs::Size<to_cs>
Transform<from_cs, to_cs, inverted, AffineTransformBackend>::multiply_from_the_right_with(cairowindow::cs::Size<from_cs> const& size) const
{
  // Just scale; scale the X and Y axis vectors by the full linear part.
  double const sx = std::hypot(m_.m11(), m_.m12());
  double const sy = std::hypot(m_.m21(), m_.m22());
  if constexpr (!inverted)
    return {size.width() * sx, size.height() * sy};
  else
    return {size.width() / sx, size.height() / sy};
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
cairowindow::cs::Size<to_cs>
operator*(cairowindow::cs::Size<from_cs> const& size, Transform<from_cs, to_cs, inverted, AffineTransformBackend> const& transform)
{
  return transform.multiply_from_the_right_with(size);
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
cairowindow::cs::Rectangle<to_cs>
Transform<from_cs, to_cs, inverted, AffineTransformBackend>::multiply_from_the_right_with(cairowindow::cs::Rectangle<from_cs> const& rectangle) const
{
  cairowindow::cs::Point<from_cs> p_from_cs{rectangle.offset_x(), rectangle.offset_y()};
  cairowindow::cs::Size<from_cs> s_from_cs{rectangle.width(), rectangle.height()};

  cairowindow::cs::Point<to_cs> p_to_cs = multiply_from_the_right_with(p_from_cs);
  cairowindow::cs::Size<to_cs> s_to_cs = multiply_from_the_right_with(s_from_cs);

  return {p_to_cs, s_to_cs};
}

template<CS from_cs, CS to_cs, bool inverted, AffineTransformConcept AffineTransformBackend>
cairowindow::cs::Rectangle<to_cs>
operator*(cairowindow::cs::Rectangle<from_cs> const& rectangle, Transform<from_cs, to_cs, inverted, AffineTransformBackend> const& transform)
{
  return transform.multiply_from_the_right_with(rectangle);
}
