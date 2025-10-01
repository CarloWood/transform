#pragma once

#include <array>
#include <cmath>

class NiceDelta
{
 public:
  static constexpr int number_of_mantissa_values = 3;
  static constexpr std::array<int, number_of_mantissa_values> mantissa_values = { 1, 2, 5 };

 private:
  int mantissa_;
  int exponent_;
  int m_;

 private:
  // Construct NiceDelta from a given mantissa and exponent; this constructor accepts a mantissa in the range [-1, number_of_mantissa_values - 1].
  // Note that m_ is not initialized. This object may only be used to call value() on it.
  NiceDelta(int mantissa, int exponent) : mantissa_(mantissa), exponent_(exponent)
  {
    // Cannonicalize; turn a mantissa value of -1 into number_of_mantissa_values - 1 and decrement exponent_.
    if (mantissa_ == -1)
    {
      mantissa_ += number_of_mantissa_values;
      --exponent_;
    }
  }

  void increment()
  {
    mantissa_ = (mantissa_ + 1) % number_of_mantissa_values;
    if (mantissa_ == 0)
      ++exponent_;
  }

  static int calculate_m(double min, double max, double delta)
  {
    return static_cast<int>(std::floor(max / delta) - std::ceil(min / delta)) + 1;
  }

 public:
  NiceDelta(double min, double max)
  {
    double range_size = max - min;
    // Dividing by 9 guarantees that m() will return a value less than 10,
    // which then is guaranteed not larger than the sought for value.
    double ideal_delta = range_size / 9;

    // Initialize starting value for mantissa_ and exponent_.
    double ideal_exponent = std::log10(ideal_delta);
    exponent_ = static_cast<int>(std::floor(ideal_exponent));
    mantissa_ = 0;

    // Increment the current value until it is no longer less than ideal_delta.
    double current_value;
    for (;;)
    {
      current_value = value();

      // Exit once we found the first value that is greater or equal ideal_delta.
      if (current_value >= ideal_delta)
        break;

      // Go to the next allowed value.
      increment();
    }

    // Calculate m for the current value.
    m_ = calculate_m(min, max, current_value);

    // Try the next smaller value of the current NiceDelta.
    NiceDelta next_smaller_delta(mantissa_ - 1, exponent_);
    int next_m = calculate_m(min, max, next_smaller_delta.value());

    if (m_ < 5 || next_m <= 10)
    {
      exponent_ = next_smaller_delta.exponent_;
      mantissa_ = next_smaller_delta.mantissa_;
      m_ = next_m;
    }
  }

  double value() const
  {
    return mantissa_values[mantissa_] * std::pow(10.0, exponent_);
  }

  int m() const
  {
    return m_;
  }
};

