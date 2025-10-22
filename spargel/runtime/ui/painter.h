#pragma once

#include <cstdint>
#include <vector>

#include "spargel/runtime/vecmath.h"

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
            fill_rect,
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
        void add_line(Vec2f p1, Vec2f p2, uint32_t color);
        void add_triangle(Vec2f p1, Vec2f p2, Vec2f p3, uint32_t color);
        void add_rect(Vec2f p_min, Vec2f p_max, uint32_t color);

    private:
        template <typename... Ts>
        void push_cmd(DrawCommand::Kind cmd, Ts... ts) {
            cmds_.push_back(DrawCommand{
                cmd, clip_, {detail::bit_cast<Ts, uint32_t>(ts)...}});
        }

        Vec4f clip_;
        std::vector<DrawCommand> cmds_;
    };
    class Painter {
    public:
        static Painter* create();

        virtual ~Painter() = default;
    };
}  // namespace spargel::runtime::ui
