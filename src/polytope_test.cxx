#include "sys.h"
#include "utils/has_print_on.h"
#include <algorithm>
#include <array>
#include "debug.h"

using utils::has_print_on::operator<<;

struct Point
{
  static constexpr std::array<char const*, 3> value = { "0", "½", "1" };

  std::array<int, 3> value_index_;      // Index k = 0: x, 1: y, 2: z, content: 0: 0, 2: 1.

  friend bool operator!=(Point const& lhs, Point const& rhs)
  {
    return lhs.value_index_ != rhs.value_index_;
  }

  friend bool operator==(Point const& lhs, Point const& rhs)
  {
    return lhs.value_index_ == rhs.value_index_;
  }

  friend bool operator<(Point const& lhs, Point const& rhs)
  {
    return lhs.value_index_ < rhs.value_index_;
  }

  void print_on(std::ostream& os) const
  {
    os << '(';
    char const* separator = "";
    for (int k = 0; k < 3; ++k)
    {
      os << separator << value[value_index_[k]];
      separator = ", ";
    }
    os << ')';
  }
};

struct Corner : public Point
{
  Corner(Point const& point) : Point(point) { }

  Corner(int c)                         // c: zyx (in binary) where 0: 0, 1: 2.
  {
    for (int k = 0; k < 3; ++k)
    {
      int d = 1 << k;
      value_index_[k] = (c & d) ? 2 : 0;
    }
  }
};

struct Edge : public Point
{
  Edge() = default;
  Edge(Point const& point) : Point(point) { }

  Edge(Corner const& c1, Corner const& c2)
  {
    for (int k = 0; k < 3; ++k)
      value_index_[k] = (c1.value_index_[k] + c2.value_index_[k]) / 2;
  }

  Edge next_after(Edge const& last);
};

// For example, if the points `*this` = (0, 0, ½) and last = (0, ½, 1) exist,
// then the point on the other side of `last` can be deduced as follows:
// 1. the point `last` lays on the two faces x=0 and z=1.
// 2. the polytope edge `*this` -> `last` lays on the face x=0, therefore the next adjacent
//    edge must be on the face z=1 and we can choose between (½, 0, 1) or (½, 1, 1).
// However all three coordinates must differ from `*this` (0, 0, ½), therefore it has to be (½, 1, 1).
//
// The values of `value_index_` for the above example would be:
//
//  index \  *this  |   last    |  next
// x: 0        0         0          1 (½)   <-- *this is part of x=0, last is part of x=0 ; current_face (both values are equal)
// y: 1        0         1 (½)      2 (1)   <-- *this is part of y=0
// z: 2        1 (½)     2 (1)      2 (1)   <--                       last is part of z=1
//
Edge Edge::next_after(Edge const& last)
{
  // 3) Generate the next point from this pair.

  // Find the common face of this edge and last edge.
  int current_face = 0;
  while (value_index_[current_face] != last.value_index_[current_face])
    ++current_face;

  // Example: current_face is now 0.
  Edge next;
  next.value_index_[current_face] = 1;          // The current face coordinate always becomes ½.

  // Find both faces that `last` is a part of.
  for (int k = 0; k < 3; ++k)
  {
    if (last.value_index_[k] == 1)              // If the coordinate is ½ then k is not defining one of the two faces.
      continue;                                 // Example: skip k == 1 because last.value_index_[1] == 1.

    // `last` is on face `k` = last.value_index_[k], where k=0 means x, k=1 means y and k=2 means z; and last.value_index_ means 0 or 1 for respectively the values 0 and 2.
    // For example, the face z=1 is represented through k == 2 (coordinate is z) and last.value_index_[k] == 2 (with value 1).

    // Also skip the face that the previous polytope edge is a part of.
    if (k == current_face)
      continue;                                 // Example: skip k == 0 because the last polytope is on that face.

    // Example: this leaves the z=1 face (k = 2).
    next.value_index_[k] = last.value_index_[k];

    // The remaining coordinate then must be 0 or 1, but differ from *this.
    int remaining_coordinate = 3 - k - current_face;
    next.value_index_[remaining_coordinate] = 2 - value_index_[remaining_coordinate];
  }

  return next;
}

enum Direction
{
  left_to_right,
  right_to_left
};

struct Cycle
{
  std::array<Edge, 6> edges_;
  int len_;

  Cycle() : len_(0) { }

  void add(Edge const& edge)
  {
    edges_[len_] = edge;
    ++len_;
  }

  Direction direction() const
  {
    int lk, rk;         // Left and right coordinate where two adjacent edges have their ½ value.
    for (int k = 0; k < 3; ++k)
    {
      if (edges_[0].value_index_[k] == 1)       // [0] is left of [1].
        lk = k;
      if (edges_[1].value_index_[k] == 1)
        rk = k;
    }
    // (1, ½, 0) -> (1, 0, ½)           the ½ rotates left_to_right.
    //    lk=1            rk=2
    return lk == (rk + 1) % 3 ? right_to_left : left_to_right;
  }

  void reorder()
  {
    auto const idx = std::ranges::distance(edges_.begin(), std::ranges::min_element(edges_, {}, &Edge::value_index_));
    if (idx == 0)
      return;
    std::array<Edge, 6> reordered_edges;
    for (int i = 0, j = idx; i < len_; ++i, ++j)
      reordered_edges[i] = edges_[j % len_];
    edges_ = reordered_edges;
  }

  void reverse()
  {
    std::array<Edge, 6> reversed_edges;
    reversed_edges[0] = edges_[0];
    for (int i = 1; i < len_; ++i)
      reversed_edges[i] = edges_[len_ - i];
    edges_ = reversed_edges;
  }

  friend bool operator==(Cycle const& lhs, Cycle const& rhs)
  {
    return lhs.edges_ == rhs.edges_;
  }

  friend bool operator<(Cycle const& lhs, Cycle const& rhs)
  {
    return lhs.edges_ < rhs.edges_;
  }

  void print_on(std::ostream& os) const
  {
    char const* separator = "";
    for (Edge const& edge : edges_)
    {
      os << separator << edge;
      separator = " -> ";
    }
  }
};

void reorder(std::vector<Cycle>& cycles)
{
  for (Cycle& cycle : cycles)
    cycle.reorder();
}

void reverse(std::vector<Cycle>& cycles)
{
  for (Cycle& cycle : cycles)
    cycle.reverse();
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  // Lets [0,1]³ be the corners of a cube.

  //---------------------------------------------------------------------------
  // Generate all 12 edges.

  std::vector<Edge> edges;

  // Run over all eight corners.
  for (int c = 0; c < 8; ++c)           // Binary: zyx
  {
    Corner corner(c);
    // Find all adjacent corners by toggling one bit.
    for (int k = 0; k < 3; ++k)
    {
      int d = 1 << k;
      if ((c & d) != 0)
        continue;
      Corner adjacent_corner(c | d);    // Adjacent corner.

      // Construct a point on the edge between corner and adjacent_corner.
      edges.emplace_back(corner, adjacent_corner);
    }
  }
  //---------------------------------------------------------------------------

  std::vector<Cycle> left_to_right_cycles;
  std::vector<Cycle> right_to_left_cycles;

  // 1) Pick any of the 12 edges of the cube.
  for (int ce = 0; ce < 12; ++ce)
  {
    Edge start_edge = edges[ce];
    // 2) Pick any of the 4 adjacent edges.

    // Find out which coordinate is ½ (every "Edge" has exactly one).
    int h = 0;
    while (start_edge.value_index_[h] != 1)           // index 1 corresponds with the value ½.
      ++h;

    // Find the two corners between which this edge is.
    for (int i = 0; i <= 2; i += 2)             // 0: 0, 2: 1.
    {
      Corner corner(start_edge);                      // Not really a corner.
      corner.value_index_[h] = i;               // But now it is (we set the ½ to 0 or 1 (the value of i).

      // Change one of the other two dimensions into a ½, to find an adjacent corner.
      for (int k = 0; k < 3; ++k)
      {
        if (k == h)
          continue;

        Edge adjacent_edge(corner);             // Not really an edge.
        adjacent_edge.value_index_[k] = 1;      // But now it is (we set one of the coordinates that wasn't a ½ before to ½ now).

        // Create the next cycle.
        Cycle cycle;

        //  start_edge      edge  adjacent_edge
        //      |             |     |     .-- next_edge
        //      |             |     |     |
        //      v             v     v     v
        //      O --> ... --> O --> O --> O --> ...
        Edge edge(start_edge);
        Dout(dc::notice|continued_cf, start_edge);
        cycle.add(start_edge);
        do
        {
          Dout(dc::continued, " -> " << adjacent_edge);
          cycle.add(adjacent_edge);
          Edge next_edge = edge.next_after(adjacent_edge);
          edge = adjacent_edge;
          adjacent_edge = next_edge;
        }
        while (adjacent_edge != start_edge);
        Dout(dc::finish, "");

        if (cycle.direction() == left_to_right)
          left_to_right_cycles.push_back(cycle);
        else
          right_to_left_cycles.push_back(cycle);
      }
    }
  }

  ASSERT(left_to_right_cycles.size() == right_to_left_cycles.size());

  // Rotate the cycles so the smallest edge is first in the array.
  reorder(right_to_left_cycles);
  // Remove duplicates.
  std::sort(right_to_left_cycles.begin(), right_to_left_cycles.end());
  auto last2 = std::unique(right_to_left_cycles.begin(), right_to_left_cycles.end());
  right_to_left_cycles.erase(last2, right_to_left_cycles.end());
  // Rotate the cycles so the smallest edge is first in the array.
  reorder(left_to_right_cycles);
  // Reverse the right_to_left_cycles so that they become left_to_right_cycles! This keeps the smallest edge first in the array.
  reverse(right_to_left_cycles);
  // Append the reversed right_to_left_cycles to left_to_right_cycles.
  std::copy(right_to_left_cycles.begin(), right_to_left_cycles.end(), std::back_inserter(left_to_right_cycles));
  // Remove duplicates.
  std::sort(left_to_right_cycles.begin(), left_to_right_cycles.end());
  auto last1 = std::unique(left_to_right_cycles.begin(), left_to_right_cycles.end());
  left_to_right_cycles.erase(last1, left_to_right_cycles.end());

  Dout(dc::notice, "Left to right cycles + reversed right to left cycles:");
  for (Cycle& cycle : left_to_right_cycles)
    Dout(dc::notice, cycle);
}
