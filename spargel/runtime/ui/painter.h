#pragma once

#include <vector>

#include "spargel/runtime/vecmath.h"

namespace spargel::runtime::ui {
    // TODO(tianjiao): support multiple texture
    struct DrawCommand {
        Vec4f clip_rect;
        uint32_t vertex_offset;
        uint32_t index_offset;
        uint32_t triangle_count;
    };
    struct DrawColor {
        uint32_t value;
    };
    struct DrawVertex {
        Vec2f pos;
        Vec2f uv;
        DrawColor color;
    };
    class CommandList {
    public:
        void add_triangle(Vec2f p1, Vec2f p2, Vec2f p3, DrawColor color);

    private:
        std::vector<uint32_t> indices_;
        std::vector<DrawVertex> vertices_;
        std::vector<DrawCommand> cmds_;
    };
    struct PaintData {
        std::vector<CommandList*> cmdlists;
    };
    class Painter {
    public:
        static Painter* create();

        virtual ~Painter() = default;
    };
}  // namespace spargel::runtime::ui
