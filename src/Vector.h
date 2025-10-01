#pragma once

#include "CS.h"
#include "Point.h"
#include "cairowindow/Vector.h"

template<CS cs>
class Vector : public cairowindow::Vector
{
 private:
  // This should not be public. We use this so we can just return calls to base class members that return a Vector by value.
  Vector(cairowindow::Vector&& base) : cairowindow::Vector(base) { }

 public:
  // Construct an uninitialized Vector.
  Vector() = default;

  // Copy constructor.
  Vector(Vector const& orig) : cairowindow::Vector(orig) { }

  // Construct a vector from its x,y coordinates.
  Vector(double x, double y) : cairowindow::Vector(x, y) { }

  // Construct a Vector that points in direction and has length.
  // Also used for automatic conversion from a Direction to a Vector.
  Vector(math::Direction direction, double length = 1.0) : cairowindow::Vector(direction, length) { }

  // Construct a Vector from two points. If the second point is not given it defaults to the origin.
  // The direction is from the second argument (or origin) to the first argument.
  Vector(Point<cs> const& from, Point<cs> const& to) : cairowindow::Vector(from, to) { }
  explicit Vector(Point<cs> const& to) : cairowindow::Vector(to) { }

  // Construct a Vector from a LinePiece, pointing from the first point to the second point.
  explicit Vector(math::LinePiece const& line_piece) : cairowindow::Vector(line_piece) { }

  // Convert the vector to a Point.
  Point<cs> as_point() const { return {x_, y_}; }

  // Call the base class implementations, but replace cairowindow::Vector with Vector.
  double dot(Vector const& v2) const { return cairowindow::Vector::dot(v2); }
  double cross(Vector const& v2) const { return cairowindow::Vector::cross(v2); }
  Vector rotate_90_degrees() const { return cairowindow::Vector::rotate_90_degrees(); }
  Vector rotate_180_degrees() const { return cairowindow::Vector::rotate_180_degrees(); }
  Vector rotate_270_degrees() const { return cairowindow::Vector::rotate_270_degrees(); }
  Vector& operator+=(Vector const& v2) { return static_cast<Vector&>(cairowindow::Vector::operator+=(v2)); }
  Vector& operator-=(Vector const& v2) { return static_cast<Vector&>(cairowindow::Vector::operator-=(v2)); }
  Vector& operator*=(double scalar) { return static_cast<Vector&>(cairowindow::Vector::operator*=(scalar)); }
  Vector& operator/=(double scalar) { return static_cast<Vector&>(cairowindow::Vector::operator/=(scalar)); }
  Vector operator/(double scalar) const { return cairowindow::Vector::operator/(scalar); }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const
  {
    os << utils::to_string(cs) << ":" << cairowindow::Vector::print_on(os);
  }
#endif
};

template<CS cs>
inline Vector<cs> operator*(double length, Vector<cs> const& v2)
{
  return math::operator*(length, v2);
}

template<CS cs>
inline Point<cs> operator+(Point<cs> const& point, Vector<cs> const& v2)
{
  return math::operator+(point, v2);
}

template<CS cs>
inline Point<cs> operator-(Point<cs> const& point, Vector<cs> const& v2)
{
  return math::operator-(point, v2);
}

template<CS cs>
inline Vector<cs> operator+(Vector<cs> const& v1, Vector<cs> const& v2)
{
  return math::operator+(v1, v2);
}

template<CS cs>
inline Vector<cs> operator-(Vector<cs> const& v1, Vector<cs> const& v2)
{
  return math::operator-(v1, v2);
}
