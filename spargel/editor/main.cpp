#include "spargel/runtime/init.h"
#include "spargel/runtime/text/font_manager.h"
#include "spargel/runtime/ui/commands.h"
#include "spargel/runtime/ui/context.h"

namespace ui = spargel::ui;

namespace spargel::editor {
namespace {

class EditorApp : public ui::RenderDelegate {
public:
  EditorApp() {
    auto font_manager = text::FontManager::create();
  }

  void render() override {
    ui::begin_frame();

    ui::begin_window("test window");

    ui::fill_triangle({0, 0}, {100, 200}, {200, 100}, 0xFFFF0000);
    ui::stroke_line({0, 0}, {500, 500}, 2.0, 0xFF00FF00);
    for (int i = 1; i < 10; i++) {
      ui::stroke_line({0, 10.0f * i}, {500, 10.0f * i}, 1.0, 0xFFCCCCCC);
    }

    ui::end_window();
  }
};

}  // namespace
}  // namespace spargel::editor

int main() {
  spargel::init_runtime();
  spargel::editor::EditorApp app;
  auto& ui_ctx = ui::Context::get();
  ui_ctx.set_title("Spargel Editor");
  ui_ctx.start_with(&app);
  return 0;
}
