#pragma once

#include <cstdint>

#include "spargel/runtime/vecmath.h"

namespace spargel::runtime::ui {
void begin_frame();
void fill_triangle(Vec2f p1, Vec2f p2, Vec2f p3, uint32_t c);
void stroke_line(Vec2f p1, Vec2f p2, float thickness, uint32_t c);
}  // namespace spargel::runtime::ui
