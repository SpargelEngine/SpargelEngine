#pragma once

#include <cstdint>

#include "spargel/runtime/math/vecmath.h"

namespace spargel::ui {

// begin a new frame
// usually called first in RenderDelegate::render()
void begin_frame();

// paint commands
void fill_triangle(math::Vec2f p1, math::Vec2f p2, math::Vec2f p3, uint32_t c);
void fill_rect(math::Vec2f orig, math::Vec2f size, uint32_t c);
void stroke_line(math::Vec2f p1, math::Vec2f p2, float thickness, uint32_t c);

bool button(math::Vec2f orig, math::Vec2f size, uint32_t c);
bool label_button(char const* text, math::Vec2f orig, math::Vec2f size, uint32_t c);

void label(char const* text, math::Vec2f orig, uint32_t c);

void begin_window(char const* name);
void end_window();

}  // namespace spargel::ui
