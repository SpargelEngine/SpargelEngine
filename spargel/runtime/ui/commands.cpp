#include "spargel/runtime/ui/commands.h"

#include "spargel/runtime/ui/context.h"

namespace spargel::runtime::ui {
namespace {
CommandList& cmdlist() {
    auto& ctx = Context::get();
    return ctx.command_list();
}
}  // namespace
void begin_frame() { cmdlist().clear(); }
void fill_triangle(Vec2f p1, Vec2f p2, Vec2f p3, uint32_t c) {
    cmdlist().fill_tri(p1, p2, p3, c);
}
void stroke_line(Vec2f p1, Vec2f p2, float thickness, uint32_t c) {
    cmdlist().stroke_line(p1, p2, thickness, c);
}
}  // namespace spargel::runtime::ui
