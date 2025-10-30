#include "spargel/runtime/ui/commands.h"

#include "spargel/runtime/ui/context.h"

namespace spargel::ui {
namespace {
CommandList& cmdlist() {
    auto& ctx = Context::get();
    return ctx.command_list();
}
}  // namespace
void begin_frame() { cmdlist().clear(); }
void fill_triangle(math::Vec2f p1, math::Vec2f p2, math::Vec2f p3, uint32_t c) {
    cmdlist().fill_tri(p1, p2, p3, c);
}
void stroke_line(math::Vec2f p1, math::Vec2f p2, float thickness, uint32_t c) {
    cmdlist().stroke_line(p1, p2, thickness, c);
}

void begin_window(char const* name) {
    auto& ctx = Context::get();
    auto* window = ctx.find_window(name);
}
void end_window() {}

}  // namespace spargel::ui
