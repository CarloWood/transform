#pragma once

#include "cairowindow/cs/Size.h"

constexpr int window_width = 600;
constexpr int window_height = 450;

constexpr int object_width = 200;
constexpr int object_height = 100;

using CS = cairowindow::CS;

static constexpr cairowindow::cs::Size<CS::pixels> window_size(window_width, window_height);
static constexpr cairowindow::cs::Size<CS::pixels> half_window_size(0.5 * window_width, 0.5 * window_height);

