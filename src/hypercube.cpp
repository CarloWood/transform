#include "sys.h"
#include "utils/print_using.h"
#include "cairowindow/intersection_points.h"
#include <cmath>
#include "debug.h"

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  // Number of dimensions.
  constexpr int n  = 7;

  using namespace intersections;

  Printer printer(n);
  Corner corner;                // Origin.

  HyperBlock<double, n> hypercube({0, 0, 0, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 1, 1});
  HyperPlane<double, n> hyperplane({1, 1, 1, 1, 1, 1, 1}, 3.5);

  auto intersections = hypercube.intersection_points(hyperplane);
  Dout(dc::notice, "intersections = " << intersections.size());
}
