#include "sys.h"
#include "NiceDelta.h"
#include <vector>
#include <cstdint>
#include "debug.h"

#if CW_DEBUG
#include "utils/almost_equal.h"
#endif

namespace bruteforce {

// d = 5
//
// 5 [ 10, 15, 20, 25 ] 30
//
// d = 2
//
// 4 [ 6 8 10 12 14 16 18 20 22 24 26 28 ] 30
//
// min = 6
// max = 28
//

// Let d = s * 10^n, where n is an integer and s in S.
class D
{
 private:
  int s_;       // Let s be in S.
  int n_;

 public:
  // Construct a D that has the value 10^-9.
  D() : s_{1}, n_{-9} { }
  D(int s, int n) : s_(s), n_(n) { }

  operator double() const
  {
    return s_ * std::pow(10.0, n_);
  }

  // Go to the next allowed value of d.
  D& operator++()
  {
    // Let S be the set {1, 2, 5}.
    if (s_ == 1)
      s_ = 2;
    else if (s_ == 2)
      s_ = 5;
    else if (s_ == 5)
    {
      s_ = 1;
      ++n_;
    }
    return *this;
  }

  // Go to the previous allowed value of d.
  D& operator--()
  {
    if (s_ == 5)
      s_ = 2;
    else if (s_ == 2)
      s_ = 1;
    else
    {
      s_ = 5;
      --n_;
    }
    return *this;
  }
};

// Let k1 be the smallest possibe integer such that k1 * d >= min.
// Let k2 be the largest possible integer such that k2 * d <= max.
//
// Let m = k2 - k1 + 1.
uint64_t calculate_m(double min, double max, D const& d)
{
  uint64_t k1 = std::ceil(min / d);
  uint64_t k2 = std::floor(max / d);
  return k2 - k1 + 1;
}

double brute_force_d(double min, double max, int& m_out)
{
  // What is the smallest d such that m <= 10.
  D d;              // Start with 10^-8.
  D prev_d;
  uint64_t m = calculate_m(min, max, d);         // m gets smaller if d gets larger.
  ASSERT(m > 10);                           // We assume that the initial m is always too large.
  while (m > 10)
  {
    prev_d = d;
    ++d;                                    // Try the next larger value of d.
    m = calculate_m(min, max, d);
  }
  // If for that d-value m < 5, then just take the next smaller d (which is the largest d such that m >= 5).
  if (m < 5)
  {
    d = prev_d;
    m = calculate_m(min, max, d);
  }

  m_out = m;
  return d;
}

} // namespace bruteforce

std::array<double, 8> test_powers = {
  0.000001,
  0.001,
  0.1,
  1.0,
  10.0,
  100.0,
  1000.0,
  100000.0
};

std::array<double, 16> test_factors = {
  1.0,
  1.1,
  1.5,
  1.9,
  2.0,
  2.1,
  2.2,
  3.0,
  4.9,
  5.0,
  5.1,
  6.0,
  7.5,
  8.0,
  9.9,
  10.0
};

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  std::vector<double> min_values;
  for (double power : test_powers)
    for (double factor : test_factors)
      min_values.push_back(factor * power);

  std::vector<double> max_minus_min_values;
  for (double power : test_powers)
    for (double factor : test_factors)
      max_minus_min_values.push_back(0.1 * factor * power);

  for (double min : min_values)
    for (double max_minus_min : max_minus_min_values)
    {
      // Given the range: R = [min, max].
      double max = min + max_minus_min;

      int m_bf;
      double d_bf = bruteforce::brute_force_d(min, max, m_bf);

      NiceDelta nice_delta(min, max);
      int m = nice_delta.m();
      double d = nice_delta.value();

      if (m_bf != m)
        Dout(dc::warning, "R = [" << min << ", " << max << "] : d_bf = " << d_bf << ", d = " << d << ", m_bf = " << m_bf << ", m = " << m);
      else
        ASSERT(utils::almost_equal(d_bf, d, 10e-8));

      ASSERT(m_bf == m && utils::almost_equal(d_bf, d, 10e-8));
    }

  Dout(dc::notice, "Success!");
}
