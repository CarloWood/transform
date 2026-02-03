#pragma once

#include <type_traits>
#include <utility>

// Adapter that turns a Qt-like QTransform type into a Transform backend.
//
// This header intentionally does not include any Qt headers. The user is
// expected to include the appropriate Qt header (for example <QTransform>)
// before instantiating this template.
//
// QTransformLike must represent an affine transformation matrix:
//
//   ⎡ m₁₁ m₁₂ 0 ⎤
//   ⎢ m₂₁ m₂₂ 0 ⎥
//   ⎣ m₃₁ m₃₂ 1 ⎦
//
// It must have a member functions m11(), m12(), etc that return the respective mᵢⱼ matrix elements.
//
// Furthermore it must provide the following methods:
//
// translate(dx, dy)
//
//   returns
//
//     ⎡  1  0  0 ⎤⎡ m₁₁ m₁₂ 0 ⎤   ⎡ m₁₁                  m₁₂                  0 ⎤
//     ⎢  0  1  0 ⎥⎢ m₂₁ m₂₂ 0 ⎥ = ⎢ m₂₁                  m₂₂                  0 ⎥
//     ⎣ dx dy  1 ⎦⎣ m₃₁ m₃₂ 1 ⎦   ⎣ m₁₁dx + m₂₁dy + m₃₁  m₁₂dx + m₂₂dy + m₃₂  1 ⎦
//
// - scale(sx, sy)
//
//   returns
//
//     ⎡sx  0  0 ⎤⎡ m₁₁ m₁₂ 0 ⎤   ⎡ sx * m₁₁  sx * m₁₂  0 ⎤
//     ⎢ 0 sy  0 ⎥⎢ m₂₁ m₂₂ 0 ⎥ = ⎢ sy * m₂₁  sy * m₂₂  0 ⎥
//     ⎣ 0  0  1 ⎦⎣ m₃₁ m₃₂ 1 ⎦   ⎣      m₃₁       m₃₂  1 ⎦
//
// - rotate(degrees)
//
//   returns
//
//     ⎡ cos(θ) sin(θ)  0 ⎤⎡ m₁₁ m₁₂ 0 ⎤   ⎡  cos(θ) * m₁₁ + sin(θ) * m₂₁   cos(θ) * m₁₂ + sin(θ) * m₂₂  0 ⎤
//     ⎢-sin(θ) cos(θ)  0 ⎥⎢ m₂₁ m₂₂ 0 ⎥ = ⎢ -sin(θ) * m₁₁ + cos(θ) * m₂₁  -sin(θ) * m₁₂ + cos(θ) * m₂₂  0 ⎥
//     ⎣ 0      0       1 ⎦⎣ m₃₁ m₃₂ 1 ⎦   ⎣  m₃₁                           m₃₂                          1 ⎦
//
// - inverted(bool*)
//
//   returns
//
//            ⎡  m₂₂           -m₁₂            0   ⎤
//    (1/det) ⎢ -m₂₁            m₁₁            0   ⎥
//            ⎣  m₂₁m₃₂-m₂₂m₃₁  m₁₂m₃₁-m₁₁m₃₂  det ⎦
//
//   where det = m₁₁m₂₂-m₁₂m₂₁
//
//   The bool is set to true iff the det != 0.
//
// - map(x, y)
//
//   returns
//
//           ⎡ m₁₁ m₁₂ 0 ⎤
//    [x y 1]⎢ m₂₁ m₂₂ 0 ⎥ = [ x * m₁₁ + y * m₂₁ + m₃₁  x * m₁₂ + y * m₂₂ + m₃₂  1 ]
//           ⎣ m₃₁ m₃₂ 1 ⎦
//
template<typename QTransformLike>
class QTransformAffineTransform
{
 private:
  QTransformLike transform_;

  using Scalar = decltype(std::declval<QTransformLike const&>().m11());

 public:
  QTransformAffineTransform() = default;

  explicit QTransformAffineTransform(QTransformLike const& transform) : transform_(transform) { }

  // Matrix element accessors.
  double m11() const { return transform_.m11(); }
  double m21() const { return transform_.m21(); }
  double m31() const { return transform_.m31(); }
  double m12() const { return transform_.m12(); }
  double m22() const { return transform_.m22(); }
  double m32() const { return transform_.m32(); }

  QTransformAffineTransform& translate(double dx, double dy)
  {
    transform_.translate(static_cast<Scalar>(dx), static_cast<Scalar>(dy));
    return *this;
  }

  QTransformAffineTransform& scale(double factor)
  {
    transform_.scale(static_cast<Scalar>(factor), static_cast<Scalar>(factor));
    return *this;
  }

  QTransformAffineTransform& rotate(double degrees)
  {
    transform_.rotate(static_cast<Scalar>(degrees));
    return *this;
  }

  [[nodiscard]] QTransformAffineTransform inverted() const
  {
    [[maybe_unused]] bool ok = true;
    QTransformLike const inv = transform_.inverted(&ok);
    ASSERT(ok);
    return QTransformAffineTransform(inv);
  }

  [[nodiscard]] std::pair<double, double> map_point(double x, double y) const
  {
    Scalar xr, yr;
    transform_.map(static_cast<Scalar>(x), static_cast<Scalar>(y), &xr, &yr);
    return {xr, yr};
  }

  [[nodiscard]] std::pair<double, double> map_vector(double dx, double dy) const
  {
    // Affine vector mapping: translation cancels out.
    return {dx * transform_.m11() + dy * transform_.m21(), dx * transform_.m12() + dy * transform_.m22()};
  }

  friend QTransformAffineTransform operator*(QTransformAffineTransform const& lhs, QTransformAffineTransform const& rhs)
  {
    return QTransformAffineTransform(lhs.transform_ * rhs.transform_);
  }

  [[nodiscard]] QTransformLike const& underlying() const { return transform_; }
};
