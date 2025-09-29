#pragma once

#include "TranslationVector.h"
#include <QTransform>
#include <sstream>
#include <algorithm>
#include <iomanip>

template<CS from_cs, CS to_cs, bool inverted = false>
class Transform
{
 private:
  template<CS from_cs2, CS to_cs2, bool inverted2>
  friend class Transform;

  QTransform m_;

 private:
  Transform(QTransform const& m) : m_(m) { }

 public:
  Transform() = default;

  Transform& translate(TranslationVector<to_cs> const& tv);
  Transform& scale(qreal s);
  Transform& rotate(qreal alpha);

  // The inverse converts from `to_cs` to `from_cs`!
  Transform<to_cs, from_cs, !inverted> const& inverse() const
  {
    return reinterpret_cast<Transform<to_cs, from_cs, !inverted> const&>(*this);
  }

  Point<to_cs> multiply_from_the_right_with(Point<from_cs> const& point) const;
  Size<to_cs> multiply_from_the_right_with(Size<from_cs> const& size) const;

  // Let A_M1_B be non-inverted and convert from A to B.
  // Let B_M2_C be non-inverted and convert from B to C.
  //
  // Multiplication between two non-inverted Transforms.
  // 1. A_M12_C = A_M1_B * B_M2_C
  //
  // Let A_M1_B^-1 be an inverted matrix that converts from B to A, and therefore denote it as B_M1inv_A
  // Let B_M2_C^-1 be an inverted matrix that converts from C to B, and therefore denote it as C_M2inv_B
  //
  // Multiplication between two inverted Transforms.
  // 2. A_M34inv_C = A_M4inv_B * B_M3inv_C
  //
  // Note that C_M34_A = C_M3_B * B_M4_A
  //
  // Let A_M5inv_B = B_M5_A^-1 be an inverted matrix that converts from A to B.
  // Let B_M56_C = B_M5_A * A_M6_C.
  //
  // Multiplication between an inverted Transform and a non-inverted Transform.
  // 3. A_M6_C = A_M5inv_B * B_M56_C
  //
  // Let A_M78_B = A_M7_C * C_M8_B.
  //
  // Multiplication between a non-inverted Transform and an inverted Transform.
  // 4. A_M7_C = A_M78_B * B_M8inv_C
  //
  // Then using specializations, where from_cs = A, to_cs = B and result_cs = C we'd have:
  //
  // Specialization for 1.
  // std::enable_if_t<!inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, false> const& rhs) const;
  //
  // Specialization for 2.
  // std::enable_if_t<inverted, Transform<from_cs, result_cs, true>> operator*(Transform<to_cs, result_cs, true> const& rhs) const;
  //
  // Specialization for 3.
  // std::enable_if_t<inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, false> const& rhs) const;
  //
  // Specialization for 4.
  //std::enable_if_t<!inverted, Transform<from_cs, result_cs, false>> operator*(Transform<to_cs, result_cs, true> const& rhs) const;
  //
  template<CS result_cs, bool rhs_inverted>
  Transform<from_cs, result_cs, inverted && rhs_inverted> operator*(Transform<to_cs, result_cs, rhs_inverted> const& rhs) const
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

  void print_on(std::ostream& os) const
  {
    std::ostringstream prefix;
    prefix << utils::to_string(from_cs) << "_transform_" << utils::to_string(to_cs) << ":";
    int const prefix_len = std::max((int)prefix.str().length(), 24);

    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎛" << std::setw(9) << m_.m11() << " " << std::setw(9) << m_.m12() << " " << m_.m13() << "⎞" << std::right;
    os << '\n' << std::right << std::setw(prefix_len) << prefix.str() <<
      std::left << "⎜" << std::setw(9) << m_.m21() << " " << std::setw(9) << m_.m22() << " " << m_.m23() << "⎟" << std::right;
    os << '\n' << std::setw(prefix_len) << " " <<
      std::left << "⎝" << std::setw(9) << m_.m31() << " " << std::setw(9) << m_.m32() << " " << m_.m33() << "⎠" << std::right;
  }
};

template<CS from_cs, CS to_cs, bool inverted>
Transform<from_cs, to_cs, inverted>& Transform<from_cs, to_cs, inverted>::translate(TranslationVector<to_cs> const& tv)
{
  m_.translate(tv.x(), tv.y());
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted>
Transform<from_cs, to_cs, inverted>& Transform<from_cs, to_cs, inverted>::scale(qreal s)
{
  m_.scale(s, s);
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted>
Transform<from_cs, to_cs, inverted>& Transform<from_cs, to_cs, inverted>::rotate(qreal alpha)
{
  m_.rotate(alpha);
  return *this;
}

template<CS from_cs, CS to_cs, bool inverted>
Point<to_cs> Transform<from_cs, to_cs, inverted>::multiply_from_the_right_with(Point<from_cs> const& point) const
{
  QPointF p{point.x(), point.y()};
  QPointF result;
  if constexpr (!inverted)
    result = m_.map(p);
  else
    result = m_.inverted().map(p);
  return {result.x(), result.y()};
}

template<CS from_cs, CS to_cs, bool inverted>
Point<to_cs> operator*(Point<from_cs> const& point, Transform<from_cs, to_cs, inverted> const& transform)
{
  return transform.multiply_from_the_right_with(point);
}

template<CS from_cs, CS to_cs, bool inverted>
Size<to_cs> Transform<from_cs, to_cs, inverted>::multiply_from_the_right_with(Size<from_cs> const& size) const
{
  // Just scale; scale the X and Y axis vectors by the full linear part.
  qreal const sx = std::hypot(m_.m11(), m_.m12());
  qreal const sy = std::hypot(m_.m21(), m_.m22());
  if constexpr (!inverted)
    return {size.width() * sx, size.height() * sy};
  else
    return {size.width() / sx, size.height() / sy};
}

template<CS from_cs, CS to_cs, bool inverted>
Size<to_cs> operator*(Size<from_cs> const& size, Transform<from_cs, to_cs, inverted> const& transform)
{
  return transform.multiply_from_the_right_with(size);
}
