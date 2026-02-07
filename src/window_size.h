#pragma once

#include "cairowindow/cs/CS.h"
#include "math/cs/Size.h"

constexpr int window_width = 600;
constexpr int window_height = 450;

constexpr int object_width = 200;
constexpr int object_height = 100;

static constexpr math::cs::Size<math::csid::pixels> window_size(window_width, window_height);
static constexpr math::cs::Size<math::csid::pixels> half_window_size(0.5 * window_width, 0.5 * window_height);
