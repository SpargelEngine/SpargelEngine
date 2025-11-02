#pragma once

#include <cstdint>
#include <vector>

#include "spargel/runtime/math/vecmath.h"

namespace spargel::ui {

struct DrawVertex {
  math::Vec2f position;
  math::Vec2f tex_coord;
  uint32_t color;
  uint32_t dummy = 0;
};
static_assert(sizeof(DrawVertex) == 24);

class CommandList {
public:
  static constexpr math::Vec2f WHITE_PIXEL = {0.5f / 256, 0.5f / 256};

  void fill_tri(math::Vec2f p1, math::Vec2f p2, math::Vec2f p3, uint32_t c) {
    uint32_t i = uint32_t(vertices_.size());
    vertices_.push_back({p1, WHITE_PIXEL, c});
    vertices_.push_back({p2, WHITE_PIXEL, c});
    vertices_.push_back({p3, WHITE_PIXEL, c});
    indices_.push_back(i);
    indices_.push_back(i + 1);
    indices_.push_back(i + 2);
  }

  void stroke_line(math::Vec2f p1, math::Vec2f p2, float thickness,
                   uint32_t c) {
    // non-anti-aliasing line rendering
    auto dx = p2.x - p1.x;
    auto dy = p2.y - p1.y;
    {
      auto l = sqrt(dx * dx + dy * dy);
      dx /= l;
      dy /= l;
    }
    dx *= thickness * 0.5f;
    dy *= thickness * 0.5f;
    uint32_t i = uint32_t(vertices_.size());
    vertices_.push_back({{p1.x + dy, p1.y - dx}, WHITE_PIXEL, c});
    vertices_.push_back({{p2.x + dy, p2.y - dx}, WHITE_PIXEL, c});
    vertices_.push_back({{p2.x - dy, p2.y + dx}, WHITE_PIXEL, c});
    vertices_.push_back({{p1.x - dy, p1.y + dx}, WHITE_PIXEL, c});
    indices_.push_back(i);
    indices_.push_back(i + 1);
    indices_.push_back(i + 2);
    indices_.push_back(i);
    indices_.push_back(i + 2);
    indices_.push_back(i + 3);
  }

  void fill_text(char const* text, math::Vec2f orig, uint32_t c);
  void sample_texture(math::Vec2f orig, math::Vec2f size, math::Vec2f uv_min,
                      math::Vec2f uv_max, uint32_t c);

  size_t vertex_count() const { return vertices_.size(); }
  size_t index_count() const { return indices_.size(); }
  DrawVertex const* vertex_data() const { return vertices_.data(); }
  uint32_t const* index_data() const { return indices_.data(); }

  void clear() {
    vertices_.clear();
    indices_.clear();
  }

private:
  std::vector<DrawVertex> vertices_;
  std::vector<uint32_t> indices_;
};

}  // namespace spargel::ui
