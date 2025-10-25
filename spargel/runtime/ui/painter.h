#pragma once

#include <cstdint>
#include <vector>

#include "spargel/runtime/vecmath.h"

namespace spargel::runtime::ui {
struct DrawVertex {
    Vec2f position;
    Vec2f tex_coord;
    uint32_t color;
    uint32_t dummy;
};
static_assert(sizeof(DrawVertex) == 24);
class CommandList {
public:
    static constexpr Vec2f WHITE_PIXEL = {0.5f / 16, 0.5f / 16};

    void fill_tri(Vec2f p1, Vec2f p2, Vec2f p3, uint32_t c) {
        uint32_t i = uint32_t(vertices_.size());
        vertices_.push_back({p1, WHITE_PIXEL, c});
        vertices_.push_back({p2, WHITE_PIXEL, c});
        vertices_.push_back({p3, WHITE_PIXEL, c});
        indices_.push_back(i);
        indices_.push_back(i + 1);
        indices_.push_back(i + 2);
    }

    void stroke_line(Vec2f p1, Vec2f p2, float thickness, uint32_t c) {
        // non-anti-aliasing line rendering
        auto dx = p2.x - p1.x;
        auto dy = p2.y - p1.y;
        {
            auto l = sqrt(dx * dx + dy * dy);
            dx /= l;
            dy /= l;
        }
        dx *= thickness * 0.5;
        dy *= thickness * 0.5;
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
}  // namespace spargel::runtime::ui
