#include "sys.h"
#include "utils/ulong_to_base.h"
#include <cstdint>
#include <bit>
#include <functional>
#include "debug.h"

template <typename F>
void visit_gray_cycle(unsigned n, F const& visit)
{
  // Visit takes (value, step). Example: visit(word, k).
  uint64_t const count = uint64_t{1} << n;

  uint64_t g = 0;
  visit(g, 0);

  for (uint64_t k = 1; k < count; ++k)
  {
    unsigned bit = std::countr_zero(k);        // Index of least-significant 1 in k.
    g ^= (uint64_t{1} << bit);                 // Flip exactly that bit.
    visit(g, k);
  }

  // Optional: one could check that last→first differs by one bit:
  // (g ^ 0) has exactly one bit set when k == count - 1.
}

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  constexpr int N = 7;
  visit_gray_cycle(N, [](uint64_t g, int step){
      std::cout << std::setw(2) << step << " : " << std::setw(N) << std::setfill('0') << utils::ulong_to_base(g, "01") << '\n';
    });
}
