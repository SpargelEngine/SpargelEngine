#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include "spargel/runtime/vecmath.h"

// forward declaration
namespace spargel::runtime::ui {
class Window;
}

namespace spargel::runtime::ui {
namespace detail {
template <class To, class From>
std::enable_if_t<sizeof(To) == sizeof(From), To> bit_cast(
    const From& src) noexcept {
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}
}  // namespace detail
// TODO(tianjiao): support multiple texture
struct DrawCommand {
    enum class Kind {
        fill_rect = 0,
        fill_circle,
        fill_rounded_rect,
        stroke_line,
        stroke_circle,
        sample_texture,
    };
    Kind kind;
    alignas(16) Vec4f clip;
    uint32_t data[8];
};
static_assert(sizeof(DrawCommand) == 64);
class CommandList {
public:
    CommandList() { clear(); }

    void add_line(Vec2f p1, Vec2f p2, uint32_t color);
    void add_triangle(Vec2f p1, Vec2f p2, Vec2f p3, uint32_t color);
    void add_rect(Vec2f p_min, Vec2f p_max, uint32_t color);

    void fill_circle(float x, float y, float r, uint32_t color) {
        auto transform = current_transform();
        x += transform.x;
        y += transform.y;

        push_cmd(DrawCommand::Kind::fill_circle, x * scale_, y * scale_,
                 r * scale_, color);

        auto n = (std::ceil(2.0f * r) + 1.0f) * scale_;
        estimate_slots_ += (size_t)((std::ceil(n / 8.0f) + 1.0f) *
                                    (std::ceil(n / 8.0f) + 1.0f));
    }

    void push_transform(float dx, float dy) {
        auto current = current_transform();
        transform_stack_.push_back(Vec2f{current.x + dx, current.y + dy});
    }
    void pop_transform() { transform_stack_.pop_back(); }

    void set_clip(float x, float y, float w, float h) {
        auto transform = current_transform();
        x += transform.x;
        y += transform.y;
        clip_ = Vec4f{x * scale_, y * scale_, w * scale_, h * scale_};
    }

    size_t count() const { return cmds_.size(); }
    size_t estimate_slots() const { return estimate_slots_; }

    void const* data() const { return cmds_.data(); }

    void clear() {
        cmds_.clear();
        transform_stack_.clear();
        transform_stack_.push_back(Vec2f{0.0f, 0.0f});
        estimate_slots_ = 0;
    }

private:
    template <typename... Ts>
    void push_cmd(DrawCommand::Kind cmd, Ts... ts) {
        cmds_.push_back(
            DrawCommand{cmd, clip_, {detail::bit_cast<uint32_t>(ts)...}});
    }

    Vec2f current_transform() {
        return transform_stack_[transform_stack_.size() - 1];
    }

    const float scale_ = 2.0f;

    Vec4f clip_;
    std::vector<DrawCommand> cmds_;
    std::vector<Vec2f> transform_stack_;
    size_t estimate_slots_ = 0;
};
class Painter {
public:
    static Painter* create();

    virtual ~Painter() = default;

    virtual void bind_window(Window* window) = 0;
    virtual void render(CommandList const& cmdlist) = 0;
};
}  // namespace spargel::runtime::ui
